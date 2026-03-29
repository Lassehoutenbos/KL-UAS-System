#include "veml7700.h"
#include "pins.h"
#include "protocol.h"

#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* VEML7700 register map                                                */
/* ------------------------------------------------------------------ */
#define VEML7700_REG_ALS_CONF   0x00  /* configuration register        */
#define VEML7700_REG_ALS        0x04  /* ALS high-resolution output     */
#define VEML7700_REG_WHITE      0x05  /* white channel output           */

/* ALS_CONF bit fields (16-bit, written LSB first)
 *
 *  [15:13] reserved
 *  [12:11] ALS_GAIN  : 00=×1, 01=×2, 10=×1/8, 11=×1/4
 *  [9:6]   ALS_IT    : 0000=100ms, 0001=200ms, 0010=400ms, 0011=800ms,
 *                      1000=50ms,  1100=25ms
 *  [5:4]   ALS_PERS  : 00=1, 01=2, 10=4, 11=8 (interrupt persistence)
 *  [1]     ALS_INT_EN: 0=disabled
 *  [0]     ALS_SD    : 0=power on, 1=shutdown
 *
 * Chosen: GAIN=×1/8 (bits[12:11]=10), IT=100 ms (bits[9:6]=0000)
 * → max ~120 klux; lux resolution = 0.0576 lux/count
 * CONF word = 0b0001_0000_0000_0000 = 0x1000
 */
#define VEML7700_CONF_GAIN_1_8  (0x02u << 11)
#define VEML7700_CONF_IT_100MS  (0x00u <<  6)
#define VEML7700_CONF_WORD      (VEML7700_CONF_GAIN_1_8 | VEML7700_CONF_IT_100MS)

/* Lux resolution for GAIN=1/8, IT=100ms: 0.0576 lux/count = 57600 millilux/count
 * Represented as a fixed-point multiplier × 1000 to stay in integer arithmetic. */
#define VEML7700_RESOLUTION_MILLILUX_PER_COUNT  57600u  /* 57.600 lux / count × 1000 */

/* ------------------------------------------------------------------ */
/* Shared state                                                          */
/* ------------------------------------------------------------------ */
volatile als_packet_t g_latest_als = {0};

/* ------------------------------------------------------------------ */
/* I2C helpers                                                          */
/* ------------------------------------------------------------------ */

static void veml7700_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {
        reg,
        (uint8_t)(value & 0xFF),        /* LSB first */
        (uint8_t)((value >> 8) & 0xFF)  /* MSB */
    };
    i2c_write_blocking(MCP23017_I2C_INST, VEML7700_ADDR, buf, 3, false);
}

static uint16_t veml7700_read_reg(uint8_t reg)
{
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(MCP23017_I2C_INST, VEML7700_ADDR, &reg, 1, true);
    i2c_read_blocking(MCP23017_I2C_INST, VEML7700_ADDR, buf, 2, false);
    return (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void veml7700_init(void)
{
    /* I2C0 GPIO already configured by digital_io_init() */
    veml7700_write_reg(VEML7700_REG_ALS_CONF, VEML7700_CONF_WORD);
}

void veml7700_task(void *param)
{
    (void)param;

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(500);

    uint8_t serial_buf[TX_BUF_SIZE];

    while (1) {
        als_packet_t pkt;

        pkt.als_raw   = veml7700_read_reg(VEML7700_REG_ALS);
        pkt.white_raw = veml7700_read_reg(VEML7700_REG_WHITE);

        /* Convert to millilux (integer, avoids float on the wire) */
        pkt.lux_milli = (uint32_t)pkt.als_raw *
                        VEML7700_RESOLUTION_MILLILUX_PER_COUNT;

        pkt.ts_ms = (uint16_t)(xTaskGetTickCount() & 0xFFFF);

        /* Update shared latest reading for other tasks (e.g. screen) */
        memcpy((void *)&g_latest_als, &pkt, sizeof(pkt));

        /* Enqueue Pico→Pi ALS packet */
        int len = proto_serialize(serial_buf, sizeof(serial_buf),
                                  PROTO_TYPE_ALS,
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
