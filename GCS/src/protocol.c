#include "protocol.h"
#include "led_sk6812.h"
#include "led_ws2811.h"
#include "digital_io.h"
#include "system_state.h"
#include "usb_cdc.h"
#include <string.h>

QueueHandle_t g_tx_queue = NULL;

static TaskHandle_t s_sk6812_handle = NULL;
static TaskHandle_t s_ws2811_handle  = NULL;
static TaskHandle_t s_screen_handle  = NULL;

void proto_set_led_task_handles(TaskHandle_t sk6812_handle,
                                TaskHandle_t ws2811_handle)
{
    s_sk6812_handle = sk6812_handle;
    s_ws2811_handle  = ws2811_handle;
}

void proto_set_screen_task_handle(TaskHandle_t screen_handle)
{
    s_screen_handle = screen_handle;
}

/* ------------------------------------------------------------------ */
/* Serialize                                                             */
/* ------------------------------------------------------------------ */
int proto_serialize(uint8_t *buf, int buf_size,
                    uint8_t type, const uint8_t *payload, uint8_t payload_len)
{
    int total = 4 + payload_len;
    if (buf_size < total) return -1;

    buf[0] = PROTO_SOF;
    buf[1] = type;
    buf[2] = payload_len;
    if (payload_len > 0) {
        memcpy(&buf[3], payload, payload_len);
    }

    uint8_t cksum = type ^ payload_len;
    for (uint8_t i = 0; i < payload_len; i++) cksum ^= payload[i];
    buf[3 + payload_len] = cksum;

    return total;
}

/* ------------------------------------------------------------------ */
/* Parse                                                                 */
/* ------------------------------------------------------------------ */
int proto_parse(const uint8_t *buf, uint8_t buf_len,
                uint8_t *type_out, uint8_t *payload_out)
{
    if (buf_len < 4) return -1;
    if (buf[0] != PROTO_SOF) return -1;

    uint8_t type        = buf[1];
    uint8_t payload_len = buf[2];

    if (buf_len < (uint8_t)(4 + payload_len)) return -1;

    uint8_t cksum = type ^ payload_len;
    for (uint8_t i = 0; i < payload_len; i++) cksum ^= buf[3 + i];
    if (cksum != buf[3 + payload_len]) return -1;

    *type_out = type;
    if (payload_len > 0) memcpy(payload_out, &buf[3], payload_len);
    return payload_len;
}

/* ------------------------------------------------------------------ */
/* TX helpers                                                            */
/* ------------------------------------------------------------------ */
void proto_send_event(uint8_t event_id, uint16_t value)
{
    if (!g_tx_queue) return;
    event_pkt_t pkt = { .event_id = event_id, .value = value };
    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_EVENT,
                              (const uint8_t *)&pkt, sizeof(pkt));
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

void proto_send_error(uint8_t error_code)
{
    if (!g_tx_queue) return;
    error_pkt_t pkt = { .error_code = error_code };
    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_ERROR,
                              (const uint8_t *)&pkt, sizeof(pkt));
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

/* ------------------------------------------------------------------ */
/* RX state machine — variables declared here so dispatch can use them  */
/* ------------------------------------------------------------------ */
typedef enum {
    RX_WAIT_SOF,
    RX_WAIT_TYPE,
    RX_WAIT_LEN,
    RX_WAIT_PAYLOAD,
    RX_WAIT_CHECKSUM
} rx_state_t;

static rx_state_t s_rx_state = RX_WAIT_SOF;
static uint8_t    s_rx_type  = 0;
static uint8_t    s_rx_len   = 0;
static uint8_t    s_rx_buf[PROTO_MAX_PAYLOAD];
static uint8_t    s_rx_idx   = 0;

/* ------------------------------------------------------------------ */
/* LED command dispatcher                                                */
/* ------------------------------------------------------------------ */
static void dispatch_led_command(void)
{
    if (s_rx_len < 1) return;
    uint8_t chain = s_rx_buf[0];

    if (chain == 0x00) {
        /* SK6812 — raw GRB pixels: [chain][num_pixels][G R B ...] */
        if (s_rx_len < 2) return;
        uint8_t num_pixels = s_rx_buf[1];
        uint8_t data_len   = (uint8_t)(s_rx_len - 2);
        if (num_pixels == 0 || data_len < (uint8_t)(num_pixels * 3)) return;
        led_sk6812_set(&s_rx_buf[2], num_pixels);
        if (s_sk6812_handle) xTaskNotifyGive(s_sk6812_handle);

    } else if (chain == 0x01) {
        /* WS2811 RGB buttons — [chain][button_id][R][G][B][anim_mode] */
        if (s_rx_len < 6) return;
        uint8_t button_id = s_rx_buf[1];
        uint8_t r         = s_rx_buf[2];
        uint8_t g_c       = s_rx_buf[3];
        uint8_t b         = s_rx_buf[4];
        uint8_t anim      = s_rx_buf[5];
        led_ws2811_set_button(button_id, r, g_c, b, anim);
        /* ws2811_task wakes on its own 50 ms tick; notify for immediate update */
        if (s_ws2811_handle) xTaskNotifyGive(s_ws2811_handle);

    } else if (chain == 0x02) {
        /* MCP23017 indicator LEDs — [chain][led_mask][led_state] */
        if (s_rx_len < 3) return;
        digital_io_set_leds_async(s_rx_buf[1], s_rx_buf[2]);
    }
}

/* ------------------------------------------------------------------ */
/* RX entry point (called from CDC task context)                         */
/* ------------------------------------------------------------------ */
void proto_handle_rx(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        switch (s_rx_state) {
            case RX_WAIT_SOF:
                if (byte == PROTO_SOF) s_rx_state = RX_WAIT_TYPE;
                break;

            case RX_WAIT_TYPE:
                s_rx_type  = byte;
                s_rx_state = RX_WAIT_LEN;
                break;

            case RX_WAIT_LEN:
                s_rx_len = byte;
                s_rx_idx = 0;
                if (s_rx_len == 0) {
                    s_rx_state = RX_WAIT_CHECKSUM;
                } else if (s_rx_len > PROTO_MAX_PAYLOAD) {
                    s_rx_state = RX_WAIT_SOF;
                } else {
                    s_rx_state = RX_WAIT_PAYLOAD;
                }
                break;

            case RX_WAIT_PAYLOAD:
                s_rx_buf[s_rx_idx++] = byte;
                if (s_rx_idx >= s_rx_len) s_rx_state = RX_WAIT_CHECKSUM;
                break;

            case RX_WAIT_CHECKSUM: {
                uint8_t cksum = s_rx_type ^ s_rx_len;
                for (uint8_t j = 0; j < s_rx_len; j++) cksum ^= s_rx_buf[j];

                if (cksum == byte) {
                    switch (s_rx_type) {

                        case PROTO_TYPE_LED:
                            dispatch_led_command();
                            break;

                        case PROTO_TYPE_SCREEN:
                            if (s_screen_handle && s_rx_len >= 1) {
                                xTaskNotify(s_screen_handle,
                                            (uint32_t)s_rx_buf[0],
                                            eSetValueWithOverwrite);
                            }
                            break;

                        case PROTO_TYPE_HEARTBEAT:
                            /* Update RX timestamp (task context) */
                            g_last_heartbeat_rx_tick = xTaskGetTickCount();
                            /* Echo heartbeat back to host */
                            if (g_tx_queue && s_rx_len >= 1) {
                                tx_item_t item;
                                int hlen = proto_serialize(
                                    item.buf, sizeof(item.buf),
                                    PROTO_TYPE_HEARTBEAT, s_rx_buf, 1);
                                if (hlen > 0) {
                                    item.len = (uint8_t)hlen;
                                    xQueueSend(g_tx_queue, &item, 0);
                                }
                            }
                            break;

                        case PROTO_TYPE_BRIGHTNESS:
                            if (s_rx_len >= 2) {
                                if (s_rx_buf[0] == 0x00) {
                                    led_sk6812_set_brightness(s_rx_buf[1]);
                                } else if (s_rx_buf[0] == 0x01) {
                                    led_ws2811_set_brightness(s_rx_buf[1]);
                                }
                            }
                            break;

                        case PROTO_TYPE_MODE:
                            if (s_rx_len >= 1) {
                                sys_state_set((sys_state_t)s_rx_buf[0]);
                            }
                            break;

                        case PROTO_TYPE_WARNING:
                            if (s_rx_len >= WARN_ICON_COUNT) {
                                for (uint8_t i = 0; i < WARN_ICON_COUNT; i++) {
                                    led_sk6812_set_warning_state(i, s_rx_buf[i]);
                                }
                                /* Wake sk6812_task for an immediate visual update */
                                if (s_sk6812_handle) xTaskNotifyGive(s_sk6812_handle);
                            }
                            break;

                        default:
                            break;
                    }
                }
                s_rx_state = RX_WAIT_SOF;
                break;
            }
        }
    }
}
