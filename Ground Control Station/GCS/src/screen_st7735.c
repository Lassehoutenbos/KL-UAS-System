#include "screen_st7735.h"
#include "pins.h"
#include "analog.h"   /* g_spi1_mutex */

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* ST7735 command bytes                                                   */
/* ------------------------------------------------------------------ */
#define ST77_SWRESET    0x01
#define ST77_SLPOUT     0x11
#define ST77_INVOFF     0x20
#define ST77_INVON      0x21
#define ST77_DISPOFF    0x28
#define ST77_DISPON     0x29
#define ST77_CASET      0x2A
#define ST77_RASET      0x2B
#define ST77_RAMWR      0x2C
#define ST77_MADCTL     0x36
#define ST77_COLMOD     0x3A
#define ST77_FRMCTR1    0xB1
#define ST77_FRMCTR2    0xB2
#define ST77_FRMCTR3    0xB3
#define ST77_INVCTR     0xB4
#define ST77_PWCTR1     0xC0
#define ST77_PWCTR2     0xC1
#define ST77_PWCTR3     0xC2
#define ST77_PWCTR4     0xC3
#define ST77_PWCTR5     0xC4
#define ST77_VMCTR1     0xC5
#define ST77_GMCTRP1    0xE0
#define ST77_GMCTRN1    0xE1
#define ST77_NORON      0x13

/* GreenTab 1.44" pixel offsets */
#define XSTART  2
#define YSTART  1

/* ------------------------------------------------------------------ */
/* Standard Adafruit 5×7 font (95 printable ASCII, 0x20–0x7E)           */
/* Each entry is 5 bytes: left column first, each byte = 7 row bits,    */
/* bit0 = top row.                                                       */
/* ------------------------------------------------------------------ */
static const uint8_t s_font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 0x20 ' '  */
    {0x00,0x00,0x5F,0x00,0x00}, /* 0x21 '!'  */
    {0x00,0x07,0x00,0x07,0x00}, /* 0x22 '"'  */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 0x23 '#'  */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 0x24 '$'  */
    {0x23,0x13,0x08,0x64,0x62}, /* 0x25 '%'  */
    {0x36,0x49,0x55,0x22,0x50}, /* 0x26 '&'  */
    {0x00,0x05,0x03,0x00,0x00}, /* 0x27 '\'' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 0x28 '('  */
    {0x00,0x41,0x22,0x1C,0x00}, /* 0x29 ')'  */
    {0x14,0x08,0x3E,0x08,0x14}, /* 0x2A '*'  */
    {0x08,0x08,0x3E,0x08,0x08}, /* 0x2B '+'  */
    {0x00,0x50,0x30,0x00,0x00}, /* 0x2C ','  */
    {0x08,0x08,0x08,0x08,0x08}, /* 0x2D '-'  */
    {0x00,0x60,0x60,0x00,0x00}, /* 0x2E '.'  */
    {0x20,0x10,0x08,0x04,0x02}, /* 0x2F '/'  */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0x30 '0'  */
    {0x00,0x42,0x7F,0x40,0x00}, /* 0x31 '1'  */
    {0x42,0x61,0x51,0x49,0x46}, /* 0x32 '2'  */
    {0x21,0x41,0x45,0x4B,0x31}, /* 0x33 '3'  */
    {0x18,0x14,0x12,0x7F,0x10}, /* 0x34 '4'  */
    {0x27,0x45,0x45,0x45,0x39}, /* 0x35 '5'  */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 0x36 '6'  */
    {0x01,0x71,0x09,0x05,0x03}, /* 0x37 '7'  */
    {0x36,0x49,0x49,0x49,0x36}, /* 0x38 '8'  */
    {0x06,0x49,0x49,0x29,0x1E}, /* 0x39 '9'  */
    {0x00,0x36,0x36,0x00,0x00}, /* 0x3A ':'  */
    {0x00,0x56,0x36,0x00,0x00}, /* 0x3B ';'  */
    {0x08,0x14,0x22,0x41,0x00}, /* 0x3C '<'  */
    {0x14,0x14,0x14,0x14,0x14}, /* 0x3D '='  */
    {0x00,0x41,0x22,0x14,0x08}, /* 0x3E '>'  */
    {0x02,0x01,0x51,0x09,0x06}, /* 0x3F '?'  */
    {0x32,0x49,0x79,0x41,0x3E}, /* 0x40 '@'  */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 0x41 'A'  */
    {0x7F,0x49,0x49,0x49,0x36}, /* 0x42 'B'  */
    {0x3E,0x41,0x41,0x41,0x22}, /* 0x43 'C'  */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 0x44 'D'  */
    {0x7F,0x49,0x49,0x49,0x41}, /* 0x45 'E'  */
    {0x7F,0x09,0x09,0x09,0x01}, /* 0x46 'F'  */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 0x47 'G'  */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 0x48 'H'  */
    {0x00,0x41,0x7F,0x41,0x00}, /* 0x49 'I'  */
    {0x20,0x40,0x41,0x3F,0x01}, /* 0x4A 'J'  */
    {0x7F,0x08,0x14,0x22,0x41}, /* 0x4B 'K'  */
    {0x7F,0x40,0x40,0x40,0x40}, /* 0x4C 'L'  */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 0x4D 'M'  */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 0x4E 'N'  */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 0x4F 'O'  */
    {0x7F,0x09,0x09,0x09,0x06}, /* 0x50 'P'  */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 0x51 'Q'  */
    {0x7F,0x09,0x19,0x29,0x46}, /* 0x52 'R'  */
    {0x46,0x49,0x49,0x49,0x31}, /* 0x53 'S'  */
    {0x01,0x01,0x7F,0x01,0x01}, /* 0x54 'T'  */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 0x55 'U'  */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 0x56 'V'  */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 0x57 'W'  */
    {0x63,0x14,0x08,0x14,0x63}, /* 0x58 'X'  */
    {0x07,0x08,0x70,0x08,0x07}, /* 0x59 'Y'  */
    {0x61,0x51,0x49,0x45,0x43}, /* 0x5A 'Z'  */
    {0x00,0x7F,0x41,0x41,0x00}, /* 0x5B '['  */
    {0x02,0x04,0x08,0x10,0x20}, /* 0x5C '\\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* 0x5D ']'  */
    {0x04,0x02,0x01,0x02,0x04}, /* 0x5E '^'  */
    {0x40,0x40,0x40,0x40,0x40}, /* 0x5F '_'  */
    {0x00,0x01,0x02,0x04,0x00}, /* 0x60 '`'  */
    {0x20,0x54,0x54,0x54,0x78}, /* 0x61 'a'  */
    {0x7F,0x48,0x44,0x44,0x38}, /* 0x62 'b'  */
    {0x38,0x44,0x44,0x44,0x20}, /* 0x63 'c'  */
    {0x38,0x44,0x44,0x48,0x7F}, /* 0x64 'd'  */
    {0x38,0x54,0x54,0x54,0x18}, /* 0x65 'e'  */
    {0x08,0x7E,0x09,0x01,0x02}, /* 0x66 'f'  */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 0x67 'g'  */
    {0x7F,0x08,0x04,0x04,0x78}, /* 0x68 'h'  */
    {0x00,0x44,0x7D,0x40,0x00}, /* 0x69 'i'  */
    {0x20,0x40,0x44,0x3D,0x00}, /* 0x6A 'j'  */
    {0x7F,0x10,0x28,0x44,0x00}, /* 0x6B 'k'  */
    {0x00,0x41,0x7F,0x40,0x00}, /* 0x6C 'l'  */
    {0x7C,0x04,0x18,0x04,0x78}, /* 0x6D 'm'  */
    {0x7C,0x08,0x04,0x04,0x78}, /* 0x6E 'n'  */
    {0x38,0x44,0x44,0x44,0x38}, /* 0x6F 'o'  */
    {0x7C,0x14,0x14,0x14,0x08}, /* 0x70 'p'  */
    {0x08,0x14,0x14,0x18,0x7C}, /* 0x71 'q'  */
    {0x7C,0x08,0x04,0x04,0x08}, /* 0x72 'r'  */
    {0x48,0x54,0x54,0x54,0x20}, /* 0x73 's'  */
    {0x04,0x3F,0x44,0x40,0x20}, /* 0x74 't'  */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 0x75 'u'  */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 0x76 'v'  */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 0x77 'w'  */
    {0x44,0x28,0x10,0x28,0x44}, /* 0x78 'x'  */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 0x79 'y'  */
    {0x44,0x64,0x54,0x4C,0x44}, /* 0x7A 'z'  */
    {0x00,0x08,0x36,0x41,0x00}, /* 0x7B '{'  */
    {0x00,0x00,0x7F,0x00,0x00}, /* 0x7C '|'  */
    {0x00,0x41,0x36,0x08,0x00}, /* 0x7D '}'  */
    {0x10,0x08,0x08,0x10,0x08}, /* 0x7E '~'  */
};

/* ------------------------------------------------------------------ */
/* Low-level SPI helpers (caller must hold g_spi1_mutex)                */
/* ------------------------------------------------------------------ */

static inline void dc_cmd(void) { gpio_put(PIN_TFT_DC, 0); }
static inline void dc_data(void){ gpio_put(PIN_TFT_DC, 1); }
static inline void cs_low(void) { gpio_put(PIN_TFT_CS, 0); }
static inline void cs_high(void){ gpio_put(PIN_TFT_CS, 1); }

static void write_cmd(uint8_t cmd)
{
    dc_cmd(); cs_low();
    spi_write_blocking(ST7735_SPI_INST, &cmd, 1);
    cs_high();
}

static void write_data(const uint8_t *data, size_t len)
{
    dc_data(); cs_low();
    spi_write_blocking(ST7735_SPI_INST, data, len);
    cs_high();
}

static void write_data_byte(uint8_t d)
{
    write_data(&d, 1);
}

/* Set address window (applies GreenTab x+2, y+1 offsets) */
static void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t caset[4] = { 0x00, (uint8_t)(x0 + XSTART),
                         0x00, (uint8_t)(x1 + XSTART) };
    uint8_t raset[4] = { 0x00, (uint8_t)(y0 + YSTART),
                         0x00, (uint8_t)(y1 + YSTART) };

    write_cmd(ST77_CASET); write_data(caset, 4);
    write_cmd(ST77_RASET); write_data(raset, 4);
    write_cmd(ST77_RAMWR);
}

/* Write n pixels of the same 16-bit colour (RGB565, big-endian) */
static void flood_colour(uint16_t color, uint32_t n)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    dc_data(); cs_low();
    while (n--) {
        spi_write_blocking(ST7735_SPI_INST, &hi, 1);
        spi_write_blocking(ST7735_SPI_INST, &lo, 1);
    }
    cs_high();
}

/* ------------------------------------------------------------------ */
/* Initialisation                                                         */
/* ------------------------------------------------------------------ */
void st7735_init(void)
{
    /* Configure CS, DC, RST as GPIO outputs */
    gpio_init(PIN_TFT_CS);  gpio_set_dir(PIN_TFT_CS,  GPIO_OUT); gpio_put(PIN_TFT_CS,  1);
    gpio_init(PIN_TFT_DC);  gpio_set_dir(PIN_TFT_DC,  GPIO_OUT); gpio_put(PIN_TFT_DC,  1);
    gpio_init(PIN_TFT_RST); gpio_set_dir(PIN_TFT_RST, GPIO_OUT); gpio_put(PIN_TFT_RST, 1);

    /* Hardware reset */
    gpio_put(PIN_TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_put(PIN_TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    xSemaphoreTake(g_spi1_mutex, portMAX_DELAY);
    spi_set_baudrate(ST7735_SPI_INST, ST7735_SPI_BAUD);

    write_cmd(ST77_SWRESET); vTaskDelay(pdMS_TO_TICKS(150));
    write_cmd(ST77_SLPOUT);  vTaskDelay(pdMS_TO_TICKS(500));

    /* Frame rate */
    write_cmd(ST77_FRMCTR1);
    write_data_byte(0x01); write_data_byte(0x2C); write_data_byte(0x2D);

    write_cmd(ST77_FRMCTR2);
    write_data_byte(0x01); write_data_byte(0x2C); write_data_byte(0x2D);

    write_cmd(ST77_FRMCTR3);
    write_data_byte(0x01); write_data_byte(0x2C); write_data_byte(0x2D);
    write_data_byte(0x01); write_data_byte(0x2C); write_data_byte(0x2D);

    write_cmd(ST77_INVCTR);  write_data_byte(0x07);

    write_cmd(ST77_PWCTR1);
    write_data_byte(0xA2); write_data_byte(0x02); write_data_byte(0x84);
    write_cmd(ST77_PWCTR2);  write_data_byte(0xC5);
    write_cmd(ST77_PWCTR3);
    write_data_byte(0x0A); write_data_byte(0x00);
    write_cmd(ST77_PWCTR4);
    write_data_byte(0x8A); write_data_byte(0x2A);
    write_cmd(ST77_PWCTR5);
    write_data_byte(0x8A); write_data_byte(0xEE);

    write_cmd(ST77_VMCTR1);  write_data_byte(0x0E);

    write_cmd(ST77_INVOFF);

    /* Memory access control: rotation 3 (MX+MY) */
    write_cmd(ST77_MADCTL);  write_data_byte(0xC0);

    /* 16-bit colour */
    write_cmd(ST77_COLMOD);  write_data_byte(0x05);

    /* Gamma */
    write_cmd(ST77_GMCTRP1);
    {
        static const uint8_t g[] = {
            0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
            0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10
        };
        write_data(g, 16);
    }
    write_cmd(ST77_GMCTRN1);
    {
        static const uint8_t g[] = {
            0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
            0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10
        };
        write_data(g, 16);
    }

    write_cmd(ST77_INVON);                        /* required for GreenTab */
    write_cmd(ST77_NORON);  vTaskDelay(pdMS_TO_TICKS(10));
    write_cmd(ST77_DISPON); vTaskDelay(pdMS_TO_TICKS(100));

    xSemaphoreGive(g_spi1_mutex);

    st7735_fill_screen(ST7735_BLACK);
}

/* ------------------------------------------------------------------ */
/* Drawing primitives                                                     */
/* ------------------------------------------------------------------ */

void st7735_fill_screen(uint16_t color)
{
    st7735_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void st7735_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;
    if (x + w > ST7735_WIDTH)  w = ST7735_WIDTH  - x;
    if (y + h > ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    xSemaphoreTake(g_spi1_mutex, portMAX_DELAY);
    spi_set_baudrate(ST7735_SPI_INST, ST7735_SPI_BAUD);
    set_addr_window((uint8_t)x, (uint8_t)y,
                    (uint8_t)(x + w - 1), (uint8_t)(y + h - 1));
    flood_colour(color, (uint32_t)w * (uint32_t)h);
    xSemaphoreGive(g_spi1_mutex);
}

void st7735_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= ST7735_WIDTH || y < 0 || y >= ST7735_HEIGHT) return;

    xSemaphoreTake(g_spi1_mutex, portMAX_DELAY);
    spi_set_baudrate(ST7735_SPI_INST, ST7735_SPI_BAUD);
    set_addr_window((uint8_t)x, (uint8_t)y, (uint8_t)x, (uint8_t)y);
    uint8_t pix[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    write_data(pix, 2);
    xSemaphoreGive(g_spi1_mutex);
}

void st7735_draw_hline(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    st7735_fill_rect(x, y, len, 1, color);
}

void st7735_draw_vline(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    st7735_fill_rect(x, y, 1, len, color);
}

void st7735_draw_char(int16_t x, int16_t y, char c,
                      uint16_t fg, uint16_t bg, uint8_t size)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *glyph = s_font5x7[(uint8_t)(c - 0x20)];

    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            uint16_t color = (line & 1) ? fg : bg;
            if (size == 1) {
                st7735_draw_pixel(x + col, y + row, color);
            } else {
                st7735_fill_rect(x + col * size, y + row * size,
                                 size, size, color);
            }
            line >>= 1;
        }
    }
    /* One-pixel spacer column */
    if (size == 1) {
        for (int row = 0; row < 7; row++) {
            st7735_draw_pixel(x + 5, y + row, bg);
        }
    } else {
        st7735_fill_rect(x + 5 * size, y, size, 7 * size, bg);
    }
}

void st7735_draw_string(int16_t x, int16_t y, const char *s,
                        uint16_t fg, uint16_t bg, uint8_t size)
{
    int16_t cx = x;
    while (*s) {
        if (*s == '\n') {
            cx = x;
            y += (int16_t)(8 * size);
        } else {
            st7735_draw_char(cx, y, *s, fg, bg, size);
            cx += (int16_t)(6 * size);
        }
        s++;
    }
}

void st7735_draw_mono_bitmap(int16_t x, int16_t y,
                             const uint8_t *bmp, int16_t w, int16_t h,
                             uint16_t fg, uint16_t bg)
{
    int16_t bytes_per_row = (w + 7) / 8;
    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            uint8_t byte = bmp[(row * bytes_per_row) + (col / 8)];
            uint16_t color = (byte & (0x80u >> (col & 7))) ? fg : bg;
            st7735_draw_pixel(x + col, y + row, color);
        }
    }
}
