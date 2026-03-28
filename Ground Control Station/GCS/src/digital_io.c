#include "digital_io.h"
#include "pins.h"
#include "protocol.h"
#include "system_state.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Shared state                                                          */
/* ------------------------------------------------------------------ */
volatile digital_packet_t g_latest_digital = {0};

/* ------------------------------------------------------------------ */
/* Internal state                                                        */
/* ------------------------------------------------------------------ */

/* Shadow of port A output bits */
static uint8_t s_porta_shadow = 0x00;

/* Debounce: previous raw sample for 2-consecutive-sample filter */
static uint8_t s_prev_raw_a = 0xFF;
static uint8_t s_prev_raw_b = 0xFF;

/* Stable debounced state */
static uint8_t s_stable_a = 0xFF;
static uint8_t s_stable_b = 0xFF;

/* LED request queue */
typedef struct {
    uint8_t mask;    /* 0xFF = single-bit mode (led_bit in low 3 bits) */
    uint8_t value;   /* for single-bit: bool; for mask mode: state bitmask */
    bool    is_mask; /* true = mask+state mode, false = bit+on mode */
} led_request_t;

#define LED_REQ_QUEUE_DEPTH 8
static QueueHandle_t s_led_req_queue = NULL;

/* ------------------------------------------------------------------ */
/* MCP23017 helpers                                                      */
/* ------------------------------------------------------------------ */

static void mcp_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(MCP23017_I2C_INST, MCP23017_ADDR, buf, 2, false);
}

static void mcp_read_ports(uint8_t *porta, uint8_t *portb)
{
    uint8_t reg = MCP_GPIOA;
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(MCP23017_I2C_INST, MCP23017_ADDR, &reg, 1, true);
    i2c_read_blocking(MCP23017_I2C_INST, MCP23017_ADDR, buf, 2, false);
    *porta = buf[0];
    *portb = buf[1];
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void digital_io_init(void)
{
    i2c_init(MCP23017_I2C_INST, MCP23017_I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    mcp_write_reg(MCP_IODIRA, MCP_IODIRA_VAL);
    mcp_write_reg(MCP_IODIRB, MCP_IODIRB_VAL);
    mcp_write_reg(MCP_GPPUA,  MCP_GPPUA_VAL);
    mcp_write_reg(MCP_GPPUB,  MCP_GPPUB_VAL);

    s_porta_shadow = 0x00;
    mcp_write_reg(MCP_GPIOA, s_porta_shadow);

    s_led_req_queue = xQueueCreate(LED_REQ_QUEUE_DEPTH, sizeof(led_request_t));
}

void digital_io_set_led(uint8_t led_bit, bool on)
{
    if (on) {
        s_porta_shadow |= (uint8_t)(1u << led_bit);
    } else {
        s_porta_shadow &= (uint8_t)~(1u << led_bit);
    }
    mcp_write_reg(MCP_GPIOA, s_porta_shadow);
}

void digital_io_set_led_async(uint8_t led_bit, bool on)
{
    led_request_t req = { .mask = led_bit, .value = (uint8_t)on, .is_mask = false };
    if (s_led_req_queue) xQueueSend(s_led_req_queue, &req, 0);
}

void digital_io_set_leds_async(uint8_t mask, uint8_t state)
{
    led_request_t req = { .mask = mask, .value = state, .is_mask = true };
    if (s_led_req_queue) xQueueSend(s_led_req_queue, &req, 0);
}

void digital_io_task(void *param)
{
    (void)param;

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);  /* 50 Hz */

    uint8_t serial_buf[TX_BUF_SIZE];
    led_request_t req;

    while (1) {
        /* -- Service pending LED requests -------------------------------- */
        while (xQueueReceive(s_led_req_queue, &req, 0) == pdTRUE) {
            if (req.is_mask) {
                /* Mask-based: apply state bits where mask bits are set */
                s_porta_shadow = (s_porta_shadow & (uint8_t)~req.mask) |
                                 (req.value & req.mask);
            } else {
                /* Single-bit mode */
                if (req.value) {
                    s_porta_shadow |= (uint8_t)(1u << req.mask);
                } else {
                    s_porta_shadow &= (uint8_t)~(1u << req.mask);
                }
            }
            mcp_write_reg(MCP_GPIOA, s_porta_shadow);
        }

        /* -- Read raw GPIO state ----------------------------------------- */
        uint8_t raw_a, raw_b;
        mcp_read_ports(&raw_a, &raw_b);

        /* -- 2-sample debounce: update stable state when raw == prev_raw -- */
        uint8_t new_stable_a = s_stable_a;
        uint8_t new_stable_b = s_stable_b;

        if (raw_a == s_prev_raw_a) new_stable_a = raw_a;
        if (raw_b == s_prev_raw_b) new_stable_b = raw_b;

        s_prev_raw_a = raw_a;
        s_prev_raw_b = raw_b;

        /* -- Edge detection & event emission ----------------------------- */
        /* Only emit events in connected/active states */
        sys_state_t state = g_sys_state;
        bool emit = (state == SYS_CONNECTED || state == SYS_ACTIVE ||
                     state == SYS_LOCKED);

        if (emit) {
            uint8_t changed_a = new_stable_a ^ s_stable_a;
            uint8_t changed_b = new_stable_b ^ s_stable_b;

            if (changed_a || changed_b) {
                /* Send full state change event */
                uint16_t val = (uint16_t)(((uint16_t)new_stable_a << 8) |
                                          new_stable_b);
                proto_send_event(EVT_SWITCH_CHANGED, val);

                /* Key switch: bit IOEXP_A_KEY of port A (input, active-low) */
                if (changed_a & (1u << IOEXP_A_KEY)) {
                    bool unlocked = !(new_stable_a & (1u << IOEXP_A_KEY));
                    proto_send_event(EVT_KEY_LOCK_CHANGED, unlocked ? 1u : 0u);
                    sys_state_set(unlocked ? SYS_ACTIVE : SYS_LOCKED);
                }

                /* Individual button press/release on port B (inputs, active-low) */
                for (uint8_t bit = 0; bit < 8; bit++) {
                    if (changed_b & (1u << bit)) {
                        bool pressed = !(new_stable_b & (1u << bit));
                        proto_send_event(pressed ? EVT_BUTTON_PRESSED
                                                 : EVT_BUTTON_RELEASED,
                                         (uint16_t)bit);
                    }
                }
            }
        }

        /* Apply debounced state */
        s_stable_a = new_stable_a;
        s_stable_b = new_stable_b;

        /* -- Update shared latest state ---------------------------------- */
        g_latest_digital.port_a = s_stable_a;
        g_latest_digital.port_b = s_stable_b;
        g_latest_digital.ts_ms  = (uint16_t)(xTaskGetTickCount() & 0xFFFF);

        /* -- Serialize and enqueue full state packet --------------------- */
        int len = proto_serialize(serial_buf, sizeof(serial_buf),
                                  PROTO_TYPE_DIGITAL,
                                  (const uint8_t *)&g_latest_digital,
                                  sizeof(digital_packet_t));
        if (len > 0 && g_tx_queue) {
            tx_item_t item;
            item.len = (uint8_t)len;
            memcpy(item.buf, serial_buf, (size_t)len);
            xQueueSend(g_tx_queue, &item, 0);
        }

        vTaskDelayUntil(&last_wake, period);
    }
}
