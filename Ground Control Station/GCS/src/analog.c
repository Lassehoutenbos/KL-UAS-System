#include "analog.h"
#include "pins.h"
#include "protocol.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Shared state                                                          */
/* ------------------------------------------------------------------ */

SemaphoreHandle_t    g_spi1_mutex  = NULL;
volatile adc_packet_t g_latest_adc = {0};

/* ------------------------------------------------------------------ */
/* MCP3208 helpers                                                       */
/* ------------------------------------------------------------------ */

/*
 * Read one channel from the MCP3208 in single-ended mode.
 * Caller must hold g_spi1_mutex and have set SPI baud to MCP3208_SPI_BAUD.
 */
static uint16_t mcp3208_read(uint8_t channel)
{
    uint8_t tx[3] = {
        (uint8_t)(0x06 | (channel >> 2)),
        (uint8_t)((channel & 0x03) << 6),
        0x00
    };
    uint8_t rx[3] = {0, 0, 0};

    gpio_put(PIN_SPI_CS, 0);
    spi_write_read_blocking(MCP3208_SPI_INST, tx, rx, 3);
    gpio_put(PIN_SPI_CS, 1);

    return (uint16_t)(((rx[1] & 0x0F) << 8) | rx[2]);
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void analog_init(void)
{
    /* SPI1 hardware lines — set by whichever caller wins init first.
     * Both MCP3208 and ST7735 use CPOL=0 CPHA=0; baud rate is set per-transaction. */
    spi_init(MCP3208_SPI_INST, MCP3208_SPI_BAUD);
    spi_set_format(MCP3208_SPI_INST, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);  /* MCP3208 CS idle high */
}

void adc_task(void *param)
{
    (void)param;

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10);  /* 100 Hz */

    uint8_t serial_buf[TX_BUF_SIZE];

    while (1) {
        adc_packet_t pkt;

        /* Acquire SPI1 bus and set MCP3208 baud rate */
        xSemaphoreTake(g_spi1_mutex, portMAX_DELAY);
        spi_set_baudrate(MCP3208_SPI_INST, MCP3208_SPI_BAUD);

        for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
            pkt.ch[ch] = mcp3208_read((uint8_t)ch);
        }

        xSemaphoreGive(g_spi1_mutex);

        pkt.ts_ms = (uint16_t)(xTaskGetTickCount() & 0xFFFF);

        /* Update shared latest sample for screen task */
        memcpy((void *)&g_latest_adc, &pkt, sizeof(pkt));

        int len = proto_serialize(serial_buf, sizeof(serial_buf),
                                  PROTO_TYPE_ADC,
                                  (const uint8_t *)&pkt, sizeof(pkt));
        if (len > 0 && g_tx_queue) {
            tx_item_t item;
            item.len = (uint8_t)len;
            memcpy(item.buf, serial_buf, (size_t)len);
            xQueueSend(g_tx_queue, &item, 0);
        }

        vTaskDelayUntil(&last_wake, period);
    }
}
