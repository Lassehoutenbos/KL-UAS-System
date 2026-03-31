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
#define BAT_DIVIDER     8.021f  /* (33k + 4.7k) / 4.7k — 6S LiPo, max 26.5 V */
#define EXT_DIVIDER     8.021f

static float adc_to_volts(uint16_t raw, float divider)
{
    return ((float)raw / ADC_MAX_COUNT) * ADC_VREF * divider;
}

/* ------------------------------------------------------------------ */
/* Additional colour constants                                            */
/* ------------------------------------------------------------------ */
#define COL_NAVY      ST7735_COLOR(  0,  32, 128)   /* dark navy blue  */
#define COL_DKBLUE    ST7735_COLOR(  0,  16,  72)   /* very dark blue  */
#define COL_TEAL      ST7735_COLOR(  0, 160, 144)   /* teal accent     */
#define COL_AMBER     ST7735_COLOR(255, 152,   0)   /* warm amber/gold */
#define COL_LTGRAY    ST7735_COLOR(176, 176, 176)   /* light grey      */
#define COL_CHARCOAL  ST7735_COLOR( 28,  32,  40)   /* dark charcoal   */
#define COL_DKRED     ST7735_COLOR(144,   0,   0)   /* dark red        */
#define COL_DKGREEN   ST7735_COLOR(  0, 100,   0)   /* dark green      */

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
    st7735_fill_screen(COL_DKBLUE);

    /* Cyan accent strip at top */
    st7735_fill_rect(0, 0, 128, 3, ST7735_CYAN);

    /* "GCS" size 3: 3 chars × 18 px = 54 px, x=(128-54)/2=37 */
    st7735_draw_string(37, 14, "GCS", ST7735_CYAN, COL_DKBLUE, 3);

    /* Underline accent below "GCS" */
    st7735_fill_rect(37, 46, 54, 2, COL_TEAL);

    /* "Ground Control Station" subtitle in white */
    st7735_draw_string(22, 54, "Ground Control", ST7735_WHITE, COL_DKBLUE, 1);
    st7735_draw_string(43, 64, "Station",        ST7735_WHITE, COL_DKBLUE, 1);

    /* Teal separator */
    st7735_draw_hline(16, 76, 96, COL_TEAL);

    /* "Booting...": 10*6=60px, x=(128-60)/2=34 */
    st7735_draw_string(34, 84, "Booting...", ST7735_GREY, COL_DKBLUE, 1);

    /* Cyan accent strip + footer bar at bottom */
    st7735_fill_rect(0, 115, 128,  2, ST7735_CYAN);
    st7735_fill_rect(0, 117, 128, 11, COL_NAVY);
    /* "KL-UAS System": 13*6=78px, x=(128-78)/2=25 */
    st7735_draw_string(25, 120, "KL-UAS System", ST7735_GREY, COL_NAVY, 1);
}

static void render_waiting(void)
{
    st7735_fill_screen(COL_CHARCOAL);

    /* Amber header */
    st7735_fill_rect(0,  0, 128, 18, COL_AMBER);
    /* "PLEASE WAIT": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 5, "PLEASE WAIT", ST7735_BLACK, COL_AMBER, 1);
    st7735_fill_rect(0, 18, 128,  2, ST7735_YELLOW);

    /* "Waiting for Pi" size 2 in yellow */
    /* "Waiting": 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 38, "Waiting", ST7735_YELLOW, COL_CHARCOAL, 2);
    /* "for Pi": 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 58, "for Pi",  ST7735_YELLOW, COL_CHARCOAL, 2);

    /* Static progress dots (3 filled, 2 dim) */
    for (int i = 0; i < 5; i++) {
        uint16_t dot_col = (i < 3) ? COL_AMBER : ST7735_DARKGREY;
        st7735_fill_rect(39 + i * 11, 82, 8, 8, dot_col);
    }

    /* Yellow accent + amber footer */
    st7735_fill_rect(0, 106, 128,  2, ST7735_YELLOW);
    st7735_fill_rect(0, 108, 128, 20, COL_AMBER);
    /* "Stand by...": 11*6=66px, x=31 */
    st7735_draw_string(31, 115, "Stand by...", ST7735_BLACK, COL_AMBER, 1);
}

static void render_main(void)
{
    st7735_fill_screen(ST7735_BLACK);
    char buf[24];

    /* Header: dark navy with teal accent line */
    st7735_fill_rect(0,  0, 128, 16, COL_NAVY);
    st7735_fill_rect(0, 16, 128,  1, COL_TEAL);
    /* "GCS PANEL": 9*6=54px, x=(128-54)/2=37 */
    st7735_draw_string(37, 4, "GCS PANEL", ST7735_WHITE, COL_NAVY, 1);

    /* ---- Battery ---- */
    st7735_draw_string(4, 19, "BATTERY", ST7735_GREY, ST7735_BLACK, 1);
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    snprintf(buf, sizeof(buf), "%4.1fV", (double)bat_v);
    uint16_t bat_col = (bat_v > 23.0f) ? ST7735_GREEN :
                       (bat_v > 21.0f) ? ST7735_YELLOW : ST7735_RED;
    /* size 2: 5 chars*12=60px, x=(128-60)/2=34 */
    st7735_draw_string(34, 28, buf, bat_col, ST7735_BLACK, 2);
    /* Outlined charge bar: DARKGREY border, CHARCOAL track, coloured fill */
    st7735_fill_rect(4,  46, 120,  8, ST7735_DARKGREY);
    st7735_fill_rect(5,  47, 118,  6, COL_CHARCOAL);
    /* 6S range: 18.0 V (empty) – 25.2 V (full) */
    int fill = (int)((bat_v - 18.0f) / (25.2f - 18.0f) * 116.0f);
    if (fill < 0)   fill = 0;
    if (fill > 116) fill = 116;
    st7735_fill_rect(5, 47, fill, 6, bat_col);

    /* ---- External voltage ---- */
    st7735_draw_string(4, 57, "EXTERNAL", ST7735_GREY, ST7735_BLACK, 1);
    float ext_v = adc_to_volts(g_latest_adc.ch[1], EXT_DIVIDER);
    snprintf(buf, sizeof(buf), "%4.1fV", (double)ext_v);
    st7735_draw_string(34, 66, buf, ST7735_CYAN, ST7735_BLACK, 2);

    /* ---- Two-tone section divider ---- */
    st7735_draw_hline(0, 84, 128, COL_NAVY);
    st7735_draw_hline(0, 85, 128, COL_TEAL);

    /* ---- Key status bar with shadow edge ---- */
    bool locked = !(g_latest_digital.port_a & (1u << IOEXP_A_KEY));
    uint16_t key_col    = locked ? ST7735_RED   : ST7735_GREEN;
    uint16_t key_shadow = locked ? COL_DKRED    : COL_DKGREEN;
    st7735_fill_rect(0, 86, 128, 13, key_col);
    st7735_draw_hline(0, 99, 128, key_shadow);
    /* "KEY: LOCKED"/"KEY:  OPEN ": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 90, locked ? "KEY: LOCKED" : "KEY:  OPEN ",
                       ST7735_WHITE, key_col, 1);

    /* ---- Switch states ---- */
    snprintf(buf, sizeof(buf), "SW A:%02X  B:%02X",
             g_latest_digital.port_a & 0xFF,
             g_latest_digital.port_b & 0xFF);
    st7735_draw_string(4, 104, buf, COL_LTGRAY, ST7735_BLACK, 1);

    /* ---- Pi status bar ---- */
    sys_state_t state = g_sys_state;
    bool connected = (state == SYS_CONNECTED || state == SYS_ACTIVE);
    uint16_t pi_col = connected ? ST7735_GREEN : COL_AMBER;
    st7735_fill_rect(0, 114, 128, 14, pi_col);
    /* "Pi: CONNECTED"/"Pi:  WAITING ": 13*6=78px, x=(128-78)/2=25 */
    st7735_draw_string(25, 117, connected ? "Pi: CONNECTED" : "Pi:  WAITING ",
                       ST7735_WHITE, pi_col, 1);
}

static void render_warning(void)
{
    st7735_fill_screen(COL_DKRED);

    /* Outer orange "glow" triangle: apex (64,5), base y=100, half-width=57 */
    for (int y = 5; y <= 100; y++) {
        int half = (int)((float)(y - 5) / 95.0f * 57.0f);
        st7735_draw_hline(64 - half, y, 2 * half + 1, ST7735_ORANGE);
    }

    /* Inner yellow triangle: apex (64,8), base y=98, half-width=54 */
    for (int y = 8; y <= 98; y++) {
        int half = (int)((float)(y - 8) / 90.0f * 54.0f);
        st7735_draw_hline(64 - half, y, 2 * half + 1, ST7735_YELLOW);
    }

    /* Black exclamation mark centred inside triangle */
    st7735_fill_rect(60, 44, 8, 28, ST7735_BLACK);  /* shaft */
    st7735_fill_rect(60, 79, 8,  8, ST7735_BLACK);  /* dot   */

    /* Label bar with orange accent line at top */
    st7735_fill_rect(0, 104, 128,  3, ST7735_ORANGE);
    st7735_fill_rect(0, 107, 128, 21, ST7735_DARKGREY);
    /* "WARNING" size 2: 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 111, "WARNING", ST7735_RED, ST7735_DARKGREY, 2);
}

static void render_lock(void)
{
    st7735_fill_screen(COL_CHARCOAL);

    /* Shackle in light grey */
    st7735_fill_rect(46, 22,  8, 38, COL_LTGRAY);  /* left leg  */
    st7735_fill_rect(74, 22,  8, 38, COL_LTGRAY);  /* right leg */
    st7735_fill_rect(46, 22, 36,  8, COL_LTGRAY);  /* top bar   */

    /* Body in amber/gold with yellow border */
    st7735_fill_rect(32, 58, 64, 44, COL_AMBER);
    st7735_draw_hline(32,  58, 64, ST7735_YELLOW);   /* top edge    */
    st7735_draw_hline(32, 101, 64, ST7735_YELLOW);   /* bottom edge */
    st7735_draw_vline(32,  58, 44, ST7735_YELLOW);   /* left edge   */
    st7735_draw_vline(95,  58, 44, ST7735_YELLOW);   /* right edge  */

    /* Keyhole centred in body: x=(128-12)/2=58 */
    st7735_fill_rect(58, 66, 12, 22, COL_CHARCOAL);

    /* "LOCKED" size 2: 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 108, "LOCKED", ST7735_RED, COL_CHARCOAL, 2);
}

static void render_batwarning(void)
{
    st7735_fill_screen(COL_CHARCOAL);

    /* Red header with amber accent line */
    st7735_fill_rect(0,  0, 128, 18, ST7735_RED);
    /* "LOW BATTERY": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 5, "LOW BATTERY", ST7735_WHITE, ST7735_RED, 1);
    st7735_fill_rect(0, 18, 128,  2, COL_AMBER);

    /* Battery icon */
    st7735_fill_rect( 8, 26,  96, 48, ST7735_DARKGREY);  /* outer border   */
    st7735_fill_rect(10, 28,  92, 44, COL_CHARCOAL);     /* interior       */
    st7735_fill_rect(104, 40, 12, 20, ST7735_DARKGREY);  /* terminal nub   */
    st7735_fill_rect(106, 42,  8, 16, COL_CHARCOAL);     /* terminal inner */
    st7735_fill_rect(10,  28,  14, 44, ST7735_RED);      /* ~15% red fill  */
    st7735_draw_vline(24, 28, 44, COL_AMBER);            /* charge marker  */

    /* Voltage size 2: 5 chars*12=60px, x=(128-60)/2=34 */
    float bat_v = adc_to_volts(g_latest_adc.ch[0], BAT_DIVIDER);
    char buf[16];
    snprintf(buf, sizeof(buf), "%4.1fV", (double)bat_v);
    st7735_draw_string(34, 82, buf, ST7735_YELLOW, COL_CHARCOAL, 2);

    /* Amber accent + red footer */
    st7735_fill_rect(0, 104, 128,  2, COL_AMBER);
    st7735_fill_rect(0, 106, 128, 22, ST7735_RED);
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
