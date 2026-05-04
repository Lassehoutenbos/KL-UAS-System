#ifndef LIGHTBAR_BOARD_H
#define LIGHTBAR_BOARD_H

#include <stdint.h>

#define BOARD_ADDR        0x04

#define BOARD_PIN_LED     16              /* SK6812 data line */
#define BOARD_NUM_PIXELS  32

/* Lighting modes (matches RS485_PERIPHERAL_BUS.md §LightBar SET_PARAM) */
#define LB_MODE_SOLID     0
#define LB_MODE_FLASH     1
#define LB_MODE_BREATHE   2

#define LB_PARAM_MODE     0x01
#define LB_PARAM_COLOUR   0x02

/* Hardware-agnostic API. Per-MCU impl in src/board_<mcu>.c. */
void     board_init(void);
void     board_set_brightness(uint8_t b);
uint8_t  board_get_brightness(void);
void     board_set_mode(uint8_t mode);
uint8_t  board_get_mode(void);
void     board_set_colour_rgb16(uint16_t rgb16);   /* RRRRRGGGGGGBBBBB */
uint16_t board_get_colour_rgb16(void);
void     board_tick(void);                          /* called from main loop */
uint16_t board_estimate_power_mw(void);

#endif
