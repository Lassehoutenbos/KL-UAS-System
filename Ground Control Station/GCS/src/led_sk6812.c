#include "led_sk6812.h"
#include "pins.h"
#include "sk6812.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal state                                                        */
/* ------------------------------------------------------------------ */

/*
 * Pixel buffer: 32-bit words packed as (G<<24)|(R<<16)|(B<<8).
 * Brightness is applied when led_sk6812_set() is called.
 */
static uint32_t s_pixel_buf[SK6812_MAX_PIXELS];
static uint8_t  s_num_pixels = 0;
static volatile uint8_t s_brightness = 255;

static int               s_dma_chan = -1;
static SemaphoreHandle_t s_dma_sem  = NULL;

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
}

void led_sk6812_set_brightness(uint8_t level)
{
    s_brightness = level;
}

void led_sk6812_set(const uint8_t *pixel_data, uint8_t num_pixels)
{
    if (num_pixels > SK6812_MAX_PIXELS) num_pixels = SK6812_MAX_PIXELS;
    s_num_pixels = num_pixels;

    uint8_t br = s_brightness;

    for (uint8_t i = 0; i < num_pixels; i++) {
        /* Input is GRB order; apply brightness per channel */
        uint8_t g = (uint8_t)(((uint16_t)pixel_data[i * 3 + 0] * br) >> 8);
        uint8_t r = (uint8_t)(((uint16_t)pixel_data[i * 3 + 1] * br) >> 8);
        uint8_t b = (uint8_t)(((uint16_t)pixel_data[i * 3 + 2] * br) >> 8);
        s_pixel_buf[i] = ((uint32_t)g << 24) |
                         ((uint32_t)r << 16) |
                         ((uint32_t)b <<  8);
    }
}

void sk6812_task(void *param)
{
    (void)param;

    /* Prime semaphore so first DMA transfer can proceed immediately */
    xSemaphoreGive(s_dma_sem);

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_num_pixels == 0) continue;

        xSemaphoreTake(s_dma_sem, portMAX_DELAY);
        dma_channel_set_read_addr(s_dma_chan, s_pixel_buf, false);
        dma_channel_set_trans_count(s_dma_chan, s_num_pixels, true);
    }
}
