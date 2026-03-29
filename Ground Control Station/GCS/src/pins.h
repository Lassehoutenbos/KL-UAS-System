#ifndef PINS_H
#define PINS_H

#include "hardware/i2c.h"
#include "hardware/spi.h"

/* ------------------------------------------------------------------ */
/* PIO / LED data lines                                                 */
/* ------------------------------------------------------------------ */
#define PIN_SK6812_DATA     0   /* GP0  — PIO0 SM0, 800 Kbps NRZ → SK6812 chain */
#define PIN_WS2811_DATA     1   /* GP1  — PIO0 SM1, 400 Kbps NRZ → WS2811 chain */

/* ------------------------------------------------------------------ */
/* I2C0 — MCP23017 GPIO expander                                        */
/* ------------------------------------------------------------------ */
#define PIN_I2C_SDA         4   /* GP4  */
#define PIN_I2C_SCL         5   /* GP5  */
#define MCP23017_I2C_INST   i2c0
#define MCP23017_I2C_BAUD   400000
#define MCP23017_ADDR       0x20  /* A2/A1/A0 all tied to GND */

/* ------------------------------------------------------------------ */
/* SPI1 — MCP3208 8-channel ADC                                         */
/* GP16-GP19 map to spi1 on RP2040                                      */
/* ------------------------------------------------------------------ */
#define PIN_SPI_MISO        16  /* GP16 — spi1 RX  */
#define PIN_SPI_CS          17  /* GP17 — software CS (active low) */
#define PIN_SPI_SCK         18  /* GP18 — spi1 SCK */
#define PIN_SPI_MOSI        19  /* GP19 — spi1 TX  */
#define MCP3208_SPI_INST    spi1
#define MCP3208_SPI_BAUD    1000000  /* 1 MHz — MCP3208 max 2 MHz at 3.3 V */

/* ------------------------------------------------------------------ */
/* MCP23017 register addresses (IOCON.BANK = 0, default)                */
/* ------------------------------------------------------------------ */
#define MCP_IODIRA      0x00
#define MCP_IODIRB      0x01
#define MCP_GPPUA       0x0C
#define MCP_GPPUB       0x0D
#define MCP_GPIOA       0x12
#define MCP_GPIOB       0x13

/* ------------------------------------------------------------------ */
/* MCP23017 Port A bit assignments                                      */
/* ------------------------------------------------------------------ */
#define IOEXP_A_LED2        0   /* output — indicator LED 2 */
#define IOEXP_A_LED3        1   /* output — indicator LED 3 */
#define IOEXP_A_LED4        2   /* output — indicator LED 4 */
/* bits 3-4 unused on port A */
#define IOEXP_A_KEY         5   /* input  — key switch */
#define IOEXP_A_SW3_3       6   /* input  — switch 3, contact 3 */
#define IOEXP_A_SW3_4       7   /* input  — switch 3, contact 4 */

/* IODIRA value: bits 0-2 output (0), bits 3-7 input (1) */
#define MCP_IODIRA_VAL      0b11111000
/* GPPUA value: pull-ups on inputs only (bits 3-7) */
#define MCP_GPPUA_VAL       0b11111000

/* ------------------------------------------------------------------ */
/* MCP23017 Port B bit assignments (all inputs)                         */
/* ------------------------------------------------------------------ */
#define IOEXP_B_SW1_1       0   /* switch 1, contact 1 */
#define IOEXP_B_SW1_2       1   /* switch 1, contact 2 */
#define IOEXP_B_SW1_3       2   /* switch 1, contact 3 */
#define IOEXP_B_SW2_1       3   /* switch 2, contact 1 */
#define IOEXP_B_SW2_2       4   /* switch 2, contact 2 */
#define IOEXP_B_SW2_3       5   /* switch 2, contact 3 */
#define IOEXP_B_SW3_1       6   /* switch 3, contact 1 */
#define IOEXP_B_SW3_2       7   /* switch 3, contact 2 */

/* IODIRB value: all inputs */
#define MCP_IODIRB_VAL      0xFF
/* GPPUB value: pull-ups on all B inputs */
#define MCP_GPPUB_VAL       0xFF

/* ------------------------------------------------------------------ */
/* SPI1 — ST7735 TFT display (shares SPI1 bus with MCP3208)           */
/* Use separate CS/DC/RST; bus protected by g_spi1_mutex               */
/* ------------------------------------------------------------------ */
#define PIN_TFT_CS          20  /* GP20 — software CS (active low)   */
#define PIN_TFT_DC          21  /* GP21 — data(1) / command(0)        */
#define PIN_TFT_RST         22  /* GP22 — hardware reset (active low) */
#define ST7735_SPI_INST     spi1
#define ST7735_SPI_BAUD     4000000   /* 4 MHz — reduce if display stays white */

/* ------------------------------------------------------------------ */
/* MCP3208 channel assignments                                          */
/* ------------------------------------------------------------------ */
#define ADC_CH_BAT_VIN      0   /* battery voltage monitor */
#define ADC_CH_EXT_VIN      1   /* external voltage input monitor */
#define ADC_CH_SENS2        2
#define ADC_CH_SENS3        3
#define ADC_CH_SENS4        4
#define ADC_CH_SENS5        5
#define ADC_NUM_CHANNELS    6   /* channels 6-7 reserved */

#endif /* PINS_H */
