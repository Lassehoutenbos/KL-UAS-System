#include "board.h"

#include "pico/stdlib.h"

/* v1: tracks state and exposes a power estimate. The actual SK6812 PIO
   drive is left to a follow-up — the framework integration is the point
   of this app. */

static uint8_t  s_brightness;
static uint8_t  s_mode;
static uint16_t s_colour;

void     board_init(void)                         { gpio_init(BOARD_PIN_LED); }
void     board_set_brightness(uint8_t b)          { s_brightness = b; }
uint8_t  board_get_brightness(void)               { return s_brightness; }
void     board_set_mode(uint8_t m)                { s_mode = m; }
uint8_t  board_get_mode(void)                     { return s_mode; }
void     board_set_colour_rgb16(uint16_t c)       { s_colour = c; }
uint16_t board_get_colour_rgb16(void)             { return s_colour; }
void     board_tick(void)                         { /* push pixels here */ }

uint16_t board_estimate_power_mw(void)
{
    /* Rough: ~60 mW per LED at full white, scaled by current brightness. */
    uint32_t mw = (uint32_t)BOARD_NUM_PIXELS * 60u * s_brightness / 255u;
    return mw > 0xFFFFu ? 0xFFFFu : (uint16_t)mw;
}
