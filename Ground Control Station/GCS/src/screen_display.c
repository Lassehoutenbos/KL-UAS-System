#include "screen_display.h"
#include "screen_st7735.h"
#include "system_state.h"
#include "analog.h"     /* g_latest_adc  */
#include "digital_io.h" /* g_latest_digital */
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Voltage conversion                                                     */
/* MCP3208 12-bit, Vref = 3.3 V                                          */
/* Adjust DIVIDER ratios to match actual hardware resistor dividers.     */
/* ------------------------------------------------------------------ */
#define ADC_VREF        3.3f
#define ADC_MAX_COUNT   4095.0f
#define BAT_DIVIDER     4.0f    /* e.g. 4:1 voltage divider on battery rail */
#define EXT_DIVIDER     4.0f

static float adc_to_volts(uint16_t raw, float divider)
{
    return ((float)raw / ADC_MAX_COUNT) * ADC_VREF * divider;
}

/* ------------------------------------------------------------------ */
/* Internal state                                                         */
/* ------------------------------------------------------------------ */
static volatile uint8_t s_mode      = SCREEN_MODE_AUTO;
static          uint8_t s_last_mode = 0xFF; /* forces first render */

/* ------------------------------------------------------------------ */
/* Render helpers                                                         */
/* ------------------------------------------------------------------ */

static void render_boot(void)
{
    st7735_fill_screen(ST7735_BLACK);
    st7735_draw_string(10, 50, "GCS", ST7735_WHITE, ST7735_BLACK, 2);
    st7735_draw_string(10, 74, "Booting...", ST7735_GREY, ST7735_BLACK, 1);
}

static void render_waiting(void)
{
    st7735_fill_screen(ST7735_YELLOW);
    st7735_draw_string(4, 44, "Waiting", ST7735_BLACK, ST7735_YELLOW, 1);
    st7735_draw_string(4, 56, "for Pi...", ST7735_BLACK, ST7735_YELLOW, 1);
}

static void render_main(void)
{
    /* Background */
    st7735_fill_screen(ST7735_BLACK);

    /* Title bar */
    st7735_fill_rect(0, 0, ST7735_WIDTH, 14, ST7735_BLUE);
    st7735_draw_string(4, 3, "GCS Panel", ST7735_WHITE, ST7735_BLUE, 1);

    /* Battery voltage */
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    char buf[24];
    snprintf(buf, sizeof(buf), "BAT: %4.1fV", (double)bat_v);
    st7735_draw_string(4, 24, buf, ST7735_GREEN, ST7735_BLACK, 1);

    /* External voltage */
    float ext_v = adc_to_volts(g_latest_adc.ch[1], EXT_DIVIDER);
    snprintf(buf, sizeof(buf), "EXT: %4.1fV", (double)ext_v);
    st7735_draw_string(4, 36, buf, ST7735_CYAN, ST7735_BLACK, 1);

    /* Key/lock status */
    bool locked = !(g_latest_digital.port_a & (1u << IOEXP_A_KEY));
    st7735_draw_string(4, 52, locked ? "KEY: LOCKED" : "KEY: OPEN  ",
                       locked ? ST7735_RED : ST7735_GREEN, ST7735_BLACK, 1);

    /* Switch summary from port B */
    snprintf(buf, sizeof(buf), "SW: %02X %02X",
             g_latest_digital.port_a & 0xFF,
             g_latest_digital.port_b & 0xFF);
    st7735_draw_string(4, 64, buf, ST7735_WHITE, ST7735_BLACK, 1);

    /* Divider */
    st7735_draw_hline(0, 78, ST7735_WIDTH, ST7735_DARKGREY);

    /* Pi connected indicator */
    sys_state_t state = g_sys_state;
    const char *con = (state == SYS_CONNECTED || state == SYS_ACTIVE) ?
                      "Pi: CONNECTED" : "Pi: WAITING  ";
    uint16_t con_color = (state == SYS_CONNECTED || state == SYS_ACTIVE) ?
                         ST7735_GREEN : ST7735_ORANGE;
    st7735_draw_string(4, 82, con, con_color, ST7735_BLACK, 1);
}

static void render_warning(void)
{
    st7735_fill_screen(ST7735_RED);
    /* Draw simple warning triangle outline */
    for (int i = 0; i < 60; i++) {
        st7735_draw_pixel(64, 24 + i,               ST7735_WHITE);
        st7735_draw_pixel(64 - i, 24 + i + 40,      ST7735_WHITE);
        st7735_draw_pixel(64 + i, 24 + i + 40 > 127
                          ? 127 : 24 + i + 40,      ST7735_WHITE);
    }
    /* Exclamation mark */
    st7735_fill_rect(61, 50, 6, 20, ST7735_WHITE);
    st7735_fill_rect(61, 76, 6,  6, ST7735_WHITE);
    st7735_draw_string(16, 100, "! WARNING !", ST7735_WHITE, ST7735_RED, 1);
}

static void render_lock(void)
{
    st7735_fill_screen(ST7735_WHITE);
    /* Draw simple padlock shape */
    st7735_fill_rect(44, 60, 40, 32, ST7735_DARKGREY);    /* body */
    st7735_draw_hline(50, 54, 12, ST7735_DARKGREY);
    st7735_draw_hline(66, 54, 12, ST7735_DARKGREY);
    st7735_draw_vline(50, 42, 12, ST7735_DARKGREY);
    st7735_draw_vline(78, 42, 12, ST7735_DARKGREY);
    st7735_draw_hline(50, 42, 28, ST7735_DARKGREY);
    /* Keyhole */
    st7735_fill_rect(60, 70, 8, 12, ST7735_WHITE);
    st7735_draw_string(30, 100, "  LOCKED  ", ST7735_BLACK, ST7735_WHITE, 1);
}

static void render_batwarning(void)
{
    st7735_fill_screen(ST7735_YELLOW);
    /* Draw simple battery shape */
    st7735_fill_rect(34, 40, 60, 32, ST7735_BLACK);       /* body */
    st7735_fill_rect(94, 50, 6, 12, ST7735_BLACK);        /* terminal */
    /* Low-battery fill (red strip) */
    st7735_fill_rect(36, 42, 16, 28, ST7735_RED);
    st7735_draw_string(16, 84, "LOW BATTERY!", ST7735_BLACK, ST7735_YELLOW, 1);
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    char buf[16];
    snprintf(buf, sizeof(buf), "%4.1fV", (double)bat_v);
    st7735_draw_string(44, 96, buf, ST7735_BLACK, ST7735_YELLOW, 1);
}

/* ------------------------------------------------------------------ */
/* Public API                                                             */
/* ------------------------------------------------------------------ */

void screen_display_init(void)
{
    /* GPIO direction for TFT pins — safe to call before scheduler starts.
     * The full st7735_init() (which uses vTaskDelay) is called from
     * screen_task after the scheduler has started. */
    gpio_init(PIN_TFT_CS);  gpio_set_dir(PIN_TFT_CS,  GPIO_OUT); gpio_put(PIN_TFT_CS,  1);
    gpio_init(PIN_TFT_DC);  gpio_set_dir(PIN_TFT_DC,  GPIO_OUT); gpio_put(PIN_TFT_DC,  1);
    gpio_init(PIN_TFT_RST); gpio_set_dir(PIN_TFT_RST, GPIO_OUT); gpio_put(PIN_TFT_RST, 1);
}

void screen_task(void *param)
{
    (void)param;

    /* Full hardware init here — uses vTaskDelay, needs scheduler running */
    st7735_init();
    render_boot();

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(200);

    while (1) {
        /* Check for host-override notification (non-blocking) */
        uint32_t notified_mode = 0;
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notified_mode, 0) == pdTRUE) {
            s_mode = (uint8_t)notified_mode;
        }

        /* Determine effective render mode */
        uint8_t effective;
        if (s_mode == SCREEN_MODE_AUTO) {
            /* Map system state to a render mode */
            switch (g_sys_state) {
                case SYS_BOOT:
                case SYS_INIT:
                    effective = 0xFF; /* boot splash — rendered by init */
                    break;
                case SYS_WAITING_FOR_PI:
                    effective = SCREEN_MODE_AUTO + 10; /* special "waiting" */
                    break;
                case SYS_LOCKED:
                    effective = SCREEN_MODE_LOCK;
                    break;
                case SYS_ERROR:
                    effective = SCREEN_MODE_WARNING;
                    break;
                default:
                    effective = SCREEN_MODE_MAIN;
                    break;
            }
        } else {
            effective = s_mode;
        }

        if (effective != s_last_mode) {
            s_last_mode = effective;
            switch (effective) {
                case SCREEN_MODE_MAIN:     render_main();       break;
                case SCREEN_MODE_WARNING:  render_warning();    break;
                case SCREEN_MODE_LOCK:     render_lock();       break;
                case SCREEN_MODE_BATWARNING: render_batwarning(); break;
                case SCREEN_MODE_AUTO + 10: render_waiting();   break;
                case 0xFF:                               break; /* boot — skip */
                default:                  render_main();       break;
            }
        } else if (effective == SCREEN_MODE_MAIN) {
            /* Refresh live data on main screen even if mode hasn't changed */
            render_main();
        }

        vTaskDelayUntil(&last_wake, period);
    }
}
