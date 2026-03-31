#ifndef SCREEN_ST7735_H
#define SCREEN_ST7735_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Color helpers (RGB888 → RGB565)                                       */
/* ------------------------------------------------------------------ */
#define ST7735_COLOR(r,g,b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define ST7735_BLACK    0x0000
#define ST7735_WHITE    0xFFFF
#define ST7735_RED      0xF800
#define ST7735_GREEN    0x07E0
#define ST7735_BLUE     0x001F
#define ST7735_YELLOW   0xFFE0
#define ST7735_CYAN     0x07FF
#define ST7735_MAGENTA  0xF81F
#define ST7735_GREY     0x8410
#define ST7735_DARKGREY 0x4208
#define ST7735_ORANGE   0xFC00

/* Screen dimensions (GreenTab 1.44") */
#define ST7735_WIDTH    128
#define ST7735_HEIGHT   128

/**
 * Initialise the ST7735 display and clear it to black.
 * Must be called from a FreeRTOS task context (uses vTaskDelay).
 */
void st7735_init(void);

/** Fill entire screen with a colour. */
void st7735_fill_screen(uint16_t color);

/** Fill a rectangle. Clips to screen bounds. */
void st7735_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t color);

/** Draw a single pixel. No-op if out of bounds. */
void st7735_draw_pixel(int16_t x, int16_t y, uint16_t color);

/** Draw one character of the built-in 5×7 font.
 *  size=1 → 5×7 px, size=2 → 10×14 px, etc. */
void st7735_draw_char(int16_t x, int16_t y, char c,
                      uint16_t fg, uint16_t bg, uint8_t size);

/** Draw a null-terminated string using the built-in font. */
void st7735_draw_string(int16_t x, int16_t y, const char *s,
                        uint16_t fg, uint16_t bg, uint8_t size);

/** Draw a monochrome 1-bit-per-pixel bitmap (MSB of each byte = leftmost pixel).
 *  fg = foreground colour (1 bits), bg = background colour (0 bits). */
void st7735_draw_mono_bitmap(int16_t x, int16_t y,
                             const uint8_t *bmp, int16_t w, int16_t h,
                             uint16_t fg, uint16_t bg);

/** Draw a horizontal line. */
void st7735_draw_hline(int16_t x, int16_t y, int16_t len, uint16_t color);

/** Draw a vertical line. */
void st7735_draw_vline(int16_t x, int16_t y, int16_t len, uint16_t color);

#endif /* SCREEN_ST7735_H */
