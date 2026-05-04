#include "led_ws2811.h"
#include "protocol.h"   /* LED_ANIM_* constants */
#include "pins.h"
#include "ws2811.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Per-button animation state                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint8_t r;
    volatile uint8_t g;
    volatile uint8_t b;
    volatile uint8_t anim;
    uint8_t phase;    /* animation tick counter (not volatile: task-only) */
} btn_state_t;

static btn_state_t s_btn[WS2811_NUM_PIXELS];
static volatile uint8_t s_brightness = 255;

/* ------------------------------------------------------------------ */
/* DMA / PIO state                                                       */
/* ------------------------------------------------------------------ */
static uint32_t          s_pixel_buf[WS2811_NUM_PIXELS];
static int               s_dma_chan = -1;
static SemaphoreHandle_t s_dma_sem  = NULL;

/* ------------------------------------------------------------------ */
/* DMA interrupt handler                                                 */
/* ------------------------------------------------------------------ */
static void __isr ws2811_dma_isr(void)
{
    if (dma_channel_get_irq1_status(s_dma_chan)) {
        dma_channel_acknowledge_irq1(s_dma_chan);
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(s_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */
void led_ws2811_init(void)
{
    uint offset = pio_add_program(pio1, &ws2811_program);
    ws2811_program_init(pio1, 0, offset, PIN_WS2811_DATA, 800000.0f);

    s_dma_chan = dma_claim_unused_channel(true);
    s_dma_sem  = xSemaphoreCreateBinary();

    dma_channel_config cfg = dma_channel_get_default_config(s_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, DREQ_PIO1_TX0);
    dma_channel_set_config(s_dma_chan, &cfg, false);
    dma_channel_set_write_addr(s_dma_chan, &pio1->txf[0], false);

    dma_channel_set_irq1_enabled(s_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_1, ws2811_dma_isr);
    irq_set_enabled(DMA_IRQ_1, true);

    memset(s_pixel_buf, 0, sizeof(s_pixel_buf));
    for (int i = 0; i < WS2811_NUM_PIXELS; i++) {
        s_btn[i].r    = 0;
        s_btn[i].g    = 0;
        s_btn[i].b    = 0;
        s_btn[i].anim = LED_ANIM_OFF;
        s_btn[i].phase = 0;
    }

    /* TEMP DIAGNOSTIC: force non-zero pixels so GP1 shows continuous
     * 50 ms bit patterns on a scope. Remove once PIO output is confirmed. */
    s_btn[0].r = 255; s_btn[0].anim = LED_ANIM_ON;
    s_btn[1].g = 255; s_btn[1].anim = LED_ANIM_ON;
}

void led_ws2811_set_button(uint8_t button_id,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t anim)
{
    if (button_id >= WS2811_NUM_PIXELS) return;
    s_btn[button_id].r    = r;
    s_btn[button_id].g    = g;
    s_btn[button_id].b    = b;
    s_btn[button_id].anim = anim;
    /* phase reset for clean animation restart */
    s_btn[button_id].phase = 0;
}

void led_ws2811_set_brightness(uint8_t level)
{
    s_brightness = level;
}

/* ------------------------------------------------------------------ */
/* Animation helpers                                                     */
/* ------------------------------------------------------------------ */

/* Apply brightness scale to a single channel (0-255) */
static inline uint8_t scale(uint8_t v, uint8_t bright)
{
    return (uint8_t)(((uint16_t)v * bright) >> 8);
}

/* Compute one pixel word for a button given its current animation phase.
 * Returns a 32-bit word packed as (R<<24)|(G<<16)|(B<<8) for the PIO. */
static uint32_t compute_pixel(const btn_state_t *btn)
{
    uint8_t r = btn->r;
    uint8_t g = btn->g;
    uint8_t b = btn->b;
    uint8_t br = s_brightness;
    uint8_t phase = btn->phase;

    switch (btn->anim) {
        case LED_ANIM_OFF:
            r = g = b = 0;
            break;

        case LED_ANIM_ON:
            r = scale(r, br);
            g = scale(g, br);
            b = scale(b, br);
            break;

        case LED_ANIM_BLINK_SLOW:
            /* 500 ms on, 500 ms off — 10 ticks per half at 50 ms/tick */
            if ((phase % 20) < 10) {
                r = scale(r, br);
                g = scale(g, br);
                b = scale(b, br);
            } else {
                r = g = b = 0;
            }
            break;

        case LED_ANIM_BLINK_FAST:
            /* 100 ms on, 100 ms off — 2 ticks per half */
            if ((phase % 4) < 2) {
                r = scale(r, br);
                g = scale(g, br);
                b = scale(b, br);
            } else {
                r = g = b = 0;
            }
            break;

        case LED_ANIM_PULSE: {
            /* Triangle wave over 40 ticks (2 second period) */
            uint8_t p = (uint8_t)(phase % 40);
            uint8_t intensity = (p < 20) ? (uint8_t)(p * 12)
                                         : (uint8_t)((40 - p) * 12);
            uint8_t bscale = scale(br, intensity);
            r = scale(r, bscale);
            g = scale(g, bscale);
            b = scale(b, bscale);
            break;
        }

        default:
            r = g = b = 0;
            break;
    }

    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8);
}

/* ------------------------------------------------------------------ */
/* FreeRTOS task                                                          */
/* ------------------------------------------------------------------ */
void ws2811_task(void *param)
{
    (void)param;

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50);

    /* Prime the DMA semaphore (starts as "available") */
    xSemaphoreGive(s_dma_sem);

    while (1) {
        /* Wake on notification (new command) OR after 50 ms tick */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));

        /* Build pixel buffer from current animation state */
        for (int i = 0; i < WS2811_NUM_PIXELS; i++) {
            s_pixel_buf[i] = compute_pixel(&s_btn[i]);
            s_btn[i].phase++;
        }

        /* Wait for previous DMA to finish, then kick off new transfer */
        xSemaphoreTake(s_dma_sem, portMAX_DELAY);
        dma_channel_set_read_addr(s_dma_chan, s_pixel_buf, false);
        dma_channel_set_trans_count(s_dma_chan, WS2811_NUM_PIXELS, true);

        /* Align to period (prevents drift when woken by notification) */
        vTaskDelayUntil(&last_wake, period);
    }
}
