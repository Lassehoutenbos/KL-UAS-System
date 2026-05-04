#include "led_sk6812.h"
#include "protocol.h"
#include "pins.h"
#include "sk6812.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdbool.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal state                                                        */
/* ------------------------------------------------------------------ */

/*
 * Pixel buffer: 32-bit words packed as (G<<24)|(R<<16)|(B<<8)|(W<<0).
 * Brightness is applied when led_sk6812_set() is called.
 */
static uint32_t s_pixel_buf[SK6812_MAX_PIXELS];
static uint8_t  s_num_pixels = 0;
static volatile uint8_t s_brightness = 255;

static int               s_dma_chan = -1;
static SemaphoreHandle_t s_dma_sem  = NULL;

/* ------------------------------------------------------------------ */
/* Warning panel state                                                   */
/* ------------------------------------------------------------------ */

static volatile uint8_t s_warn_severity[WARN_ICON_COUNT]; /* default = WARN_OK (0) */

/* ------------------------------------------------------------------ */
/* Worklight state                                                        */
/* ------------------------------------------------------------------ */

static volatile uint8_t s_worklight_on = 0;
static volatile uint8_t s_worklight_r  = 0;
static volatile uint8_t s_worklight_g  = 0;
static volatile uint8_t s_worklight_b  = 0;

/* Pixel word format used by s_pixel_buf: (G<<24)|(R<<16)|(B<<8) */
#define COL_RED    0x00FF0000UL   /* G=0,   R=255, B=0   */
#define COL_AMBER  0x64FF0000UL   /* G=100, R=255, B=0   */
#define COL_GREEN  0xFF000000UL   /* G=255, R=0,   B=0   */
#define COL_BLUE   0x0000FF00UL   /* G=0,   R=0,   B=255 */
#define COL_OFF    0x00000000UL

static bool warn_is_gps_net(uint8_t icon)
{
    return icon == WARN_ICON_GPS_GCS || icon == WARN_ICON_NETWORK_GCS;
}

static void warn_update_pixels(void)
{
    uint32_t ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bool blink_250 = (ms / 250) & 1u;
    bool blink_500 = (ms / 500) & 1u;
    uint8_t br = s_brightness;

    uint8_t led = WARN_PANEL_LED_BASE;
    for (uint8_t i = 0; i < WARN_ICON_COUNT; i++) {
        uint8_t sev = s_warn_severity[i];
        uint32_t col;

        if (warn_is_gps_net(i)) {
            if      (sev == WARN_CRITICAL) col = COL_RED;
            else if (sev == WARN_WARNING)  col = blink_500 ? COL_BLUE : COL_OFF;
            else                           col = COL_BLUE;
        } else if (i == WARN_ICON_LOCKED) {
            if      (sev == WARN_CRITICAL) col = COL_RED;
            else if (sev == WARN_WARNING)  col = COL_AMBER;
            else                           col = COL_GREEN;
        } else {
            if      (sev == WARN_CRITICAL) col = blink_250 ? COL_RED : COL_OFF;
            else if (sev == WARN_WARNING)  col = COL_AMBER;
            else                           col = COL_GREEN;
        }

        /* Apply brightness per channel; W=0 for all warning colours */
        uint8_t g = (uint8_t)(((col >> 24) & 0xFFu) * (uint16_t)br >> 8);
        uint8_t r = (uint8_t)(((col >> 16) & 0xFFu) * (uint16_t)br >> 8);
        uint8_t b = (uint8_t)(((col >>  8) & 0xFFu) * (uint16_t)br >> 8);
        uint32_t word = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);

        s_pixel_buf[led++] = word;
        /* MAIN spans two physical LEDs */
        if (i == WARN_ICON_MAIN)
            s_pixel_buf[led++] = word;
    }
}

static void worklight_update_pixels(void)
{
    uint8_t br  = s_brightness;
    uint8_t on  = s_worklight_on;
    uint8_t g_v = on ? (uint8_t)(((uint16_t)s_worklight_g * br) >> 8) : 0;
    uint8_t r_v = on ? (uint8_t)(((uint16_t)s_worklight_r * br) >> 8) : 0;
    uint8_t b_v = on ? (uint8_t)(((uint16_t)s_worklight_b * br) >> 8) : 0;
    uint32_t word = ((uint32_t)g_v << 24) | ((uint32_t)r_v << 16) | ((uint32_t)b_v << 8);

    for (uint8_t i = 0; i < WORKLIGHT_LED_COUNT; i++)
        s_pixel_buf[WORKLIGHT_LED_BASE + i] = word;
}

/* ------------------------------------------------------------------ */
/* DMA interrupt handler                                                 */
/* ------------------------------------------------------------------ */

static void __isr sk6812_dma_isr(void)
{
    if (dma_channel_get_irq0_status(s_dma_chan)) {
        dma_channel_acknowledge_irq0(s_dma_chan);
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(s_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void led_sk6812_init(void)
{
    uint offset = pio_add_program(pio0, &sk6812_program);
    sk6812_program_init(pio0, 0, offset, PIN_SK6812_DATA, 800000.0f);

    s_dma_chan = dma_claim_unused_channel(true);
    s_dma_sem  = xSemaphoreCreateBinary();

    dma_channel_config cfg = dma_channel_get_default_config(s_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX0);
    dma_channel_set_config(s_dma_chan, &cfg, false);
    dma_channel_set_write_addr(s_dma_chan, &pio0->txf[0], false);

    dma_channel_set_irq0_enabled(s_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, sk6812_dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    memset(s_pixel_buf, 0, sizeof(s_pixel_buf));

    /* Cover both the warning panel and the worklight from boot, so neither
     * region is silently dropped from DMA when only one is commanded. */
    uint8_t warn_end = WARN_PANEL_LED_BASE + WARN_PANEL_LED_COUNT;
    uint8_t work_end = WORKLIGHT_LED_BASE + WORKLIGHT_LED_COUNT;
    s_num_pixels = warn_end > work_end ? warn_end : work_end;
}

void led_sk6812_set_brightness(uint8_t level)
{
    s_brightness = level;
}

void led_sk6812_set_warning_state(uint8_t icon, uint8_t severity)
{
    if (icon >= WARN_ICON_COUNT) return;
    s_warn_severity[icon] = severity;
    /* Ensure the DMA transfer always covers the full warning panel */
    uint8_t needed = WARN_PANEL_LED_BASE + WARN_PANEL_LED_COUNT;
    if (s_num_pixels < needed)
        s_num_pixels = needed;
}

void led_sk6812_set_worklight(uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    s_worklight_on = on;
    s_worklight_r  = r;
    s_worklight_g  = g;
    s_worklight_b  = b;
    uint8_t needed = WORKLIGHT_LED_BASE + WORKLIGHT_LED_COUNT;
    if (s_num_pixels < needed)
        s_num_pixels = needed;
}

void led_sk6812_set(const uint8_t *pixel_data, uint8_t num_pixels)
{
    if (num_pixels > SK6812_MAX_PIXELS) num_pixels = SK6812_MAX_PIXELS;

    uint8_t br = s_brightness;

    for (uint8_t i = 0; i < num_pixels; i++) {
        /* Input is GRBW order; apply brightness per channel */
        uint8_t g = (uint8_t)(((uint16_t)pixel_data[i * 4 + 0] * br) >> 8);
        uint8_t r = (uint8_t)(((uint16_t)pixel_data[i * 4 + 1] * br) >> 8);
        uint8_t b = (uint8_t)(((uint16_t)pixel_data[i * 4 + 2] * br) >> 8);
        uint8_t w = (uint8_t)(((uint16_t)pixel_data[i * 4 + 3] * br) >> 8);
        s_pixel_buf[i] = ((uint32_t)g << 24) |
                         ((uint32_t)r << 16) |
                         ((uint32_t)b <<  8) |
                         ((uint32_t)w      );
    }

    /* Always cover the warning panel so it is included in every DMA transfer */
    uint8_t needed = WARN_PANEL_LED_BASE + WARN_PANEL_LED_COUNT;
    if (num_pixels < needed) {
        /* Zero-fill any gap between the last Pi pixel and the panel */
        memset(&s_pixel_buf[num_pixels], 0,
               (needed - num_pixels) * sizeof(uint32_t));
        s_num_pixels = needed;
    } else {
        s_num_pixels = num_pixels;
    }
}

void sk6812_task(void *param)
{
    (void)param;

    /* Prime semaphore so first DMA transfer can proceed immediately */
    xSemaphoreGive(s_dma_sem);

    while (1) {
        /* Wake immediately on notification (new pixel data from Pi) or
         * after 50 ms to animate the warning panel blink states. */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));

        warn_update_pixels();
        worklight_update_pixels();

        uint8_t n = s_num_pixels;
        if (n == 0) continue;

        xSemaphoreTake(s_dma_sem, portMAX_DELAY);
        dma_channel_set_read_addr(s_dma_chan, s_pixel_buf, false);
        dma_channel_set_trans_count(s_dma_chan, n, true);
    }
}
