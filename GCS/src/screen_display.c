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

/* Snapshot of last rendered main-screen data — redraw only on change */
static uint16_t    s_last_bat_raw  = 0xFFFF;
static uint16_t    s_last_ext_raw  = 0xFFFF;
static uint8_t     s_last_porta    = 0xFF;
static uint8_t     s_last_portb    = 0xFF;
static sys_state_t s_last_sysstate = 0xFF;

static bool main_needs_redraw(void)
{
    uint16_t    bat = g_latest_adc.ch[ADC_CH_BAT_VIN];
    uint16_t    ext = g_latest_adc.ch[ADC_CH_EXT_VIN];
    uint8_t     pa  = g_latest_digital.port_a;
    uint8_t     pb  = g_latest_digital.port_b;
    sys_state_t st  = g_sys_state;

    if (bat == s_last_bat_raw && ext == s_last_ext_raw &&
        pa == s_last_porta && pb == s_last_portb && st == s_last_sysstate)
        return false;

    s_last_bat_raw  = bat;
    s_last_ext_raw  = ext;
    s_last_porta    = pa;
    s_last_portb    = pb;
    s_last_sysstate = st;
    return true;
}

/* ------------------------------------------------------------------ */
/* Render helpers                                                         */
/* ------------------------------------------------------------------ */

static void render_boot(void)
{
    st7735_fill_screen(ST7735_BLACK);
    /* "GCS" size 3: 3*18=54px, x=(128-54)/2=37 */
    st7735_draw_string(37, 20, "GCS", ST7735_WHITE, ST7735_BLACK, 3);
    /* "Ground Control": 14*6=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 56, "Ground Control", ST7735_GREY, ST7735_BLACK, 1);
    /* "Station": 7*6=42px, x=(128-42)/2=43 */
    st7735_draw_string(43, 66, "Station", ST7735_GREY, ST7735_BLACK, 1);
    st7735_draw_hline(8, 80, 112, ST7735_DARKGREY);
    /* "Booting...": 10*6=60px, x=(128-60)/2=34 */
    st7735_draw_string(34, 88, "Booting...", ST7735_GREY, ST7735_BLACK, 1);
    st7735_fill_rect(0, 118, 128, 10, ST7735_DARKGREY);
    /* "KL-UAS System": 13*6=78px, x=(128-78)/2=25 */
    st7735_draw_string(25, 120, "KL-UAS System", ST7735_GREY, ST7735_DARKGREY, 1);
}

static void render_waiting(void)
{
    st7735_fill_screen(ST7735_YELLOW);
    st7735_fill_rect(0, 0, 128, 18, ST7735_ORANGE);
    /* "PLEASE WAIT": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 5, "PLEASE WAIT", ST7735_BLACK, ST7735_ORANGE, 1);
    /* "Waiting" size 2: 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 40, "Waiting", ST7735_BLACK, ST7735_YELLOW, 2);
    /* "for Pi" size 2: 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 60, "for Pi", ST7735_BLACK, ST7735_YELLOW, 2);
    st7735_draw_hline(8, 88, 112, ST7735_ORANGE);
    st7735_fill_rect(0, 108, 128, 20, ST7735_ORANGE);
    /* "Stand by...": 11*6=66px, x=31 */
    st7735_draw_string(31, 113, "Stand by...", ST7735_BLACK, ST7735_ORANGE, 1);
}

static void render_main(void)
{
    st7735_fill_screen(ST7735_BLACK);
    char buf[24];

    /* Header bar */
    st7735_fill_rect(0, 0, 128, 16, ST7735_BLUE);
    /* "GCS PANEL": 9*6=54px, x=(128-54)/2=37 */
    st7735_draw_string(37, 4, "GCS PANEL", ST7735_WHITE, ST7735_BLUE, 1);

    /* Battery section */
    st7735_draw_string(4, 19, "BATTERY", ST7735_GREY, ST7735_BLACK, 1);
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    snprintf(buf, sizeof(buf), "%4.1fV", (double)bat_v);
    uint16_t bat_col = (bat_v > 11.5f) ? ST7735_GREEN :
                       (bat_v > 10.5f) ? ST7735_YELLOW : ST7735_RED;
    /* size 2, 5 chars*12=60px, x=(128-60)/2=34 */
    st7735_draw_string(34, 28, buf, bat_col, ST7735_BLACK, 2);
    /* Charge bar */
    st7735_fill_rect(4, 46, 120, 8, ST7735_DARKGREY);
    int fill = (int)((bat_v - 10.0f) / (13.0f - 10.0f) * 118.0f);
    if (fill < 0)   fill = 0;
    if (fill > 118) fill = 118;
    st7735_fill_rect(5, 47, fill, 6, bat_col);

    /* External section */
    st7735_draw_string(4, 57, "EXTERNAL", ST7735_GREY, ST7735_BLACK, 1);
    float ext_v = adc_to_volts(g_latest_adc.ch[1], EXT_DIVIDER);
    snprintf(buf, sizeof(buf), "%4.1fV", (double)ext_v);
    st7735_draw_string(34, 66, buf, ST7735_CYAN, ST7735_BLACK, 2);

    /* Divider */
    st7735_draw_hline(0, 85, 128, ST7735_DARKGREY);

    /* Key status bar */
    bool locked = !(g_latest_digital.port_a & (1u << IOEXP_A_KEY));
    uint16_t key_col = locked ? ST7735_RED : ST7735_GREEN;
    st7735_fill_rect(0, 87, 128, 14, key_col);
    /* "KEY: LOCKED"/"KEY:  OPEN ": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 90, locked ? "KEY: LOCKED" : "KEY:  OPEN ",
                       ST7735_WHITE, key_col, 1);

    /* Switch states */
    snprintf(buf, sizeof(buf), "SW A:%02X  B:%02X",
             g_latest_digital.port_a & 0xFF,
             g_latest_digital.port_b & 0xFF);
    st7735_draw_string(4, 104, buf, ST7735_WHITE, ST7735_BLACK, 1);

    /* Pi status bar */
    sys_state_t state = g_sys_state;
    bool connected = (state == SYS_CONNECTED || state == SYS_ACTIVE);
    uint16_t pi_col = connected ? ST7735_GREEN : ST7735_ORANGE;
    st7735_fill_rect(0, 114, 128, 14, pi_col);
    /* "Pi: CONNECTED"/"Pi:  WAITING ": 13*6=78px, x=(128-78)/2=25 */
    st7735_draw_string(25, 117, connected ? "Pi: CONNECTED" : "Pi:  WAITING ",
                       ST7735_WHITE, pi_col, 1);
}

static void render_warning(void)
{
    st7735_fill_screen(ST7735_RED);

    /* Filled yellow warning triangle: apex (64,8), base y=98, half-width=54 */
    for (int y = 8; y <= 98; y++) {
        int half = (int)((float)(y - 8) / 90.0f * 54.0f);
        st7735_draw_hline(64 - half, y, 2 * half + 1, ST7735_YELLOW);
    }

    /* Black exclamation mark centred inside triangle */
    st7735_fill_rect(60, 44, 8, 28, ST7735_BLACK);  /* shaft */
    st7735_fill_rect(60, 79, 8,  8, ST7735_BLACK);  /* dot   */

    /* Label bar */
    st7735_fill_rect(0, 106, 128, 22, ST7735_DARKGREY);
    /* "WARNING" size 2: 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 110, "WARNING", ST7735_RED, ST7735_DARKGREY, 2);
}

static void render_lock(void)
{
    st7735_fill_screen(ST7735_WHITE);

    /* Shackle: left leg, right leg, top bar */
    st7735_fill_rect(46, 22, 8, 38, ST7735_DARKGREY);  /* left leg  */
    st7735_fill_rect(74, 22, 8, 38, ST7735_DARKGREY);  /* right leg */
    st7735_fill_rect(46, 22, 36, 8, ST7735_DARKGREY);  /* top bar   */

    /* Body: x=32, y=58, w=64, h=44 */
    st7735_fill_rect(32, 58, 64, 44, ST7735_DARKGREY);

    /* Keyhole centred in body */
    st7735_fill_rect(58, 66, 12, 22, ST7735_WHITE);

    /* "LOCKED" size 2: 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 108, "LOCKED", ST7735_RED, ST7735_WHITE, 2);
}

static void render_batwarning(void)
{
    st7735_fill_screen(ST7735_YELLOW);

    /* Header bar */
    st7735_fill_rect(0, 0, 128, 18, ST7735_RED);
    /* "LOW BATTERY": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 5, "LOW BATTERY", ST7735_WHITE, ST7735_RED, 1);

    /* Battery icon: body x=8, y=26, w=100, h=50 */
    st7735_fill_rect(8,  26, 100, 50, ST7735_BLACK);   /* border     */
    st7735_fill_rect(10, 28,  96, 46, ST7735_YELLOW);  /* interior   */
    st7735_fill_rect(108, 40, 12, 22, ST7735_BLACK);   /* terminal   */
    st7735_fill_rect(10,  28,  14, 46, ST7735_RED);    /* ~15% fill  */

    /* Voltage size 2: 5 chars*12=60px, x=(128-60)/2=34 */
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    char buf[16];
    snprintf(buf, sizeof(buf), "%4.1fV", (double)bat_v);
    st7735_draw_string(34, 82, buf, ST7735_BLACK, ST7735_YELLOW, 2);

    /* Footer bar */
    st7735_fill_rect(0, 108, 128, 20, ST7735_RED);
    /* "CHARGE NOW!": 11*6=66px, x=31 */
    st7735_draw_string(31, 113, "CHARGE NOW!", ST7735_WHITE, ST7735_RED, 1);
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
        } else if (effective == SCREEN_MODE_MAIN && main_needs_redraw()) {
            render_main();
        }

        vTaskDelayUntil(&last_wake, period);
    }
}
