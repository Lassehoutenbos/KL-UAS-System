/**
 * test_tft.c — ST7735 display self-test
 *
 * Standalone FreeRTOS application (alternative main) that cycles through
 * every drawing primitive exported by screen_st7735.h. Flash this instead
 * of GCS.uf2 to verify the display wiring and driver without the full
 * system running.
 *
 * Build target: GCS_TFT_Test
 * Visual only — no serial output needed.
 */

#include "screen_st7735.h"
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

static void test_color_fills(void)
{
    static const uint16_t colors[] = {
        ST7735_RED, ST7735_GREEN, ST7735_BLUE, ST7735_WHITE,
        ST7735_YELLOW, ST7735_CYAN, ST7735_MAGENTA, ST7735_BLACK,
    };
    for (int i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i++) {
        st7735_fill_screen(colors[i]);
        vTaskDelay(pdMS_TO_TICKS(600));
    }
}

static void test_rectangles(void)
{
    st7735_fill_screen(ST7735_BLACK);
    st7735_fill_rect(0,  0,  32, 32, ST7735_RED);
    st7735_fill_rect(96, 0,  32, 32, ST7735_GREEN);
    st7735_fill_rect(0,  96, 32, 32, ST7735_BLUE);
    st7735_fill_rect(96, 96, 32, 32, ST7735_YELLOW);
    st7735_fill_rect(44, 44, 40, 40, ST7735_CYAN);
    vTaskDelay(pdMS_TO_TICKS(1500));
}

static void test_lines(void)
{
    st7735_fill_screen(ST7735_BLACK);
    for (int y = 0; y < ST7735_HEIGHT; y += 8) {
        uint16_t color = (y % 16 == 0) ? ST7735_WHITE : ST7735_GREY;
        st7735_draw_hline(0, y, ST7735_WIDTH, color);
    }
    for (int x = 0; x < ST7735_WIDTH; x += 8) {
        uint16_t color = (x % 16 == 0) ? ST7735_WHITE : ST7735_DARKGREY;
        st7735_draw_vline(x, 0, ST7735_HEIGHT, color);
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
}

static void test_pixels(void)
{
    st7735_fill_screen(ST7735_BLACK);
    for (int i = 0; i < ST7735_WIDTH; i++) {
        st7735_draw_pixel(i, i, ST7735_WHITE);
        st7735_draw_pixel(ST7735_WIDTH - 1 - i, i, ST7735_ORANGE);
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
}

static void test_checkerboard(void)
{
    for (int y = 0; y < ST7735_HEIGHT; y += 8) {
        for (int x = 0; x < ST7735_WIDTH; x += 8) {
            uint16_t color = ((x + y) / 8 % 2 == 0) ? ST7735_WHITE : ST7735_BLACK;
            st7735_fill_rect(x, y, 8, 8, color);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
}

static void test_text(void)
{
    st7735_fill_screen(ST7735_BLACK);
    st7735_draw_string(2,  2,  "ST7735 TEST",    ST7735_WHITE,   ST7735_BLACK, 1);
    st7735_draw_string(2,  12, "Hello, World!",  ST7735_GREEN,   ST7735_BLACK, 1);
    st7735_draw_string(2,  22, "0123456789",     ST7735_YELLOW,  ST7735_BLACK, 1);
    st7735_draw_string(2,  32, "ABCDEFGHIJKLM",  ST7735_CYAN,    ST7735_BLACK, 1);
    st7735_draw_string(2,  42, "NOPQRSTUVWXYZ",  ST7735_MAGENTA, ST7735_BLACK, 1);
    st7735_draw_string(2,  52, "abcdefghijklm",  ST7735_RED,     ST7735_BLACK, 1);
    st7735_draw_string(2,  62, "nopqrstuvwxyz",  ST7735_ORANGE,  ST7735_BLACK, 1);
    st7735_draw_string(2,  72, "!\"#$%&'()*+,-.", ST7735_WHITE,  ST7735_BLACK, 1);
    vTaskDelay(pdMS_TO_TICKS(2000));

    st7735_fill_screen(ST7735_BLACK);
    st7735_draw_string(2, 2,  "SIZE 2", ST7735_GREEN, ST7735_BLACK, 2);
    st7735_draw_string(2, 20, "TEST",   ST7735_CYAN,  ST7735_BLACK, 2);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void test_bitmap(void)
{
    st7735_fill_screen(ST7735_BLACK);

    /* 10-wide x 16-tall cross bitmap, MSB-left, 2 bytes per row */
    static const uint8_t cross_bmp[32] = {
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b11111111, 0b11000000,
        0b11111111, 0b11000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
        0b00000110, 0b00000000,
    };

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            uint16_t fg = (col == 1) ? ST7735_YELLOW : ST7735_WHITE;
            st7735_draw_mono_bitmap(col * 40 + 4, row * 40 + 4,
                                    cross_bmp, 10, 16, fg, ST7735_BLACK);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
}

/* ------------------------------------------------------------------ */
/* Test task                                                            */
/* ------------------------------------------------------------------ */

static void tft_test_task(void *param)
{
    (void)param;

    st7735_init();

    while (1) {
        test_color_fills();
        test_rectangles();
        test_lines();
        test_pixels();
        test_checkerboard();
        test_text();
        test_bitmap();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ------------------------------------------------------------------ */
/* FreeRTOS hooks                                                       */
/* ------------------------------------------------------------------ */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    while (1) {}
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    while (1) {}
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* SPI1 is now dedicated to the ST7735S — initialised inside st7735_init() */

    xTaskCreate(tft_test_task, "TFT_TEST", 1024, NULL, 1, NULL);
    vTaskStartScheduler();

    while (1) {}
}
