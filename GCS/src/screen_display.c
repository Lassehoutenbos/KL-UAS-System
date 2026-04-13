#include "screen_display.h"
#include "screen_st7735.h"
#include "system_state.h"
#include "analog.h"     /* g_latest_adc  */
#include "digital_io.h" /* g_latest_digital */
#include "rs485.h"
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
/* Rounded-rectangle primitives (chamfered / octagonal corners)          */
/*  r = corner radius; r=0 falls back to a plain rectangle.              */
/*  Chamfer formula: row i gets indent = (r-1-i), giving a 45° bevel.   */
/* ------------------------------------------------------------------ */
static void fill_rrect(int16_t x, int16_t y, int16_t w, int16_t h,
                        int16_t r, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    if (r <= 0) { st7735_fill_rect(x, y, w, h, color); return; }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    st7735_fill_rect(x, y + r, w, h - 2 * r, color);   /* middle band */
    for (int16_t i = 0; i < r; i++) {
        int16_t ind = r - 1 - i;
        st7735_draw_hline(x + ind, y + i,         w - 2 * ind, color);
        st7735_draw_hline(x + ind, y + h - 1 - i, w - 2 * ind, color);
    }
}

static void draw_rrect(int16_t x, int16_t y, int16_t w, int16_t h,
                        int16_t r, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    if (r <= 0) {
        st7735_draw_hline(x, y,         w, color);
        st7735_draw_hline(x, y + h - 1, w, color);
        st7735_draw_vline(x,         y, h, color);
        st7735_draw_vline(x + w - 1, y, h, color);
        return;
    }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    st7735_draw_hline(x + r, y,         w - 2 * r, color);
    st7735_draw_hline(x + r, y + h - 1, w - 2 * r, color);
    st7735_draw_vline(x,         y + r, h - 2 * r, color);
    st7735_draw_vline(x + w - 1, y + r, h - 2 * r, color);
    for (int16_t i = 0; i < r; i++) {
        int16_t ind = r - 1 - i;
        st7735_draw_pixel(x + ind,         y + i,         color);
        st7735_draw_pixel(x + w - 1 - ind, y + i,         color);
        st7735_draw_pixel(x + ind,         y + h - 1 - i, color);
        st7735_draw_pixel(x + w - 1 - ind, y + h - 1 - i, color);
    }
}

/* ------------------------------------------------------------------ */
/* Internal state                                                         */
/* ------------------------------------------------------------------ */
static volatile uint8_t s_mode      = SCREEN_MODE_AUTO;
static          uint8_t s_last_mode = 0xFF; /* forces first render */
static          uint8_t s_wait_frame = 0;
static          uint8_t s_batwarning_frame = 0;

/* Snapshot of last rendered main-screen data — redraw only on change */
static uint16_t    s_last_bat_raw  = 0xFFFF;
static uint16_t    s_last_ext_raw  = 0xFFFF;
static uint8_t     s_last_porta    = 0xFF;
static uint8_t     s_last_portb    = 0xFF;
static sys_state_t s_last_sysstate = 0xFF;

/* ------------------------------------------------------------------ */
/* Peripheral screen state                                               */
/* ------------------------------------------------------------------ */
#define PERIPH_MAX_DISPLAY  8
#define PERIPH_DETAIL_BUF   32

static uint8_t  s_detail_addr  = 0x01;
static uint8_t  s_detail_cmd   = 0;
static uint8_t  s_detail_buf[PERIPH_DETAIL_BUF];
static uint8_t  s_detail_len   = 0;
static bool     s_detail_dirty = false;

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

    /* Rounded content card */
    fill_rrect(10, 7, 108, 76, 6, COL_NAVY);

    /* "GCS" size 3: 3 chars × 18 px = 54 px, x=(128-54)/2=37 */
    st7735_draw_string(37, 15, "GCS", ST7735_CYAN, COL_NAVY, 3);

    /* Underline accent below "GCS": same width/x as text */
    fill_rrect(37, 47, 54, 2, 1, COL_TEAL);

    /* "Ground Control Station" subtitle in white */
    st7735_draw_string(22, 55, "Ground Control", ST7735_WHITE, COL_NAVY, 1);
    st7735_draw_string(43, 65, "Station",        ST7735_WHITE, COL_NAVY, 1);

    /* Teal separator inside card */
    st7735_draw_hline(20, 76, 88, COL_TEAL);

    /* "Booting..." below card: 10*6=60px, x=(128-60)/2=34 */
    st7735_draw_string(34, 88, "Booting...", ST7735_GREY, COL_DKBLUE, 1);

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

    /* Rounded content panel */
    fill_rrect(8, 28, 112, 54, 6, COL_NAVY);

    /* "Waiting for Pi" size 2 inside panel */
    /* "Waiting": 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 36, "Waiting", ST7735_YELLOW, COL_NAVY, 2);
    /* "for Pi": 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 56, "for Pi",  ST7735_YELLOW, COL_NAVY, 2);

    /* Rounded progress dots (3 lit, 2 dim) */
    for (int i = 0; i < 5; i++) {
        uint16_t dot_col = (i < 3) ? COL_AMBER : ST7735_DARKGREY;
        fill_rrect(39 + i * 11, 90, 8, 8, 2, dot_col);
    }

    /* Yellow accent + amber footer */
    st7735_fill_rect(0, 106, 128,  2, ST7735_YELLOW);
    st7735_fill_rect(0, 108, 128, 20, COL_AMBER);
    /* "Stand by...": 11*6=66px, x=31 */
    st7735_draw_string(31, 115, "Stand by...", ST7735_BLACK, COL_AMBER, 1);
}

static void update_waiting_dots(void)
{
    uint8_t lit = s_wait_frame % 5;
    for (int i = 0; i < 5; i++) {
        uint16_t dot_col = (i == lit) ? COL_AMBER : ST7735_DARKGREY;
        fill_rrect(39 + i * 11, 90, 8, 8, 2, dot_col);
    }
    s_wait_frame++;
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
    /* Rounded charge bar: border → track → coloured fill */
    fill_rrect(4,  46, 120, 8, 4, ST7735_DARKGREY);   /* border */
    fill_rrect(5,  47, 118, 6, 3, COL_CHARCOAL);       /* track  */
    /* 6S range: 18.0 V (empty) – 25.2 V (full) */
    int fill = (int)((bat_v - 18.0f) / (25.2f - 18.0f) * 116.0f);
    if (fill < 0)   fill = 0;
    if (fill > 116) fill = 116;
    if (fill > 0) fill_rrect(5, 47, fill, 6, 3, bat_col);

    /* ---- External voltage ---- */
    st7735_draw_string(4, 57, "EXTERNAL", ST7735_GREY, ST7735_BLACK, 1);
    float ext_v = adc_to_volts(g_latest_adc.ch[1], EXT_DIVIDER);
    snprintf(buf, sizeof(buf), "%4.1fV", (double)ext_v);
    st7735_draw_string(34, 66, buf, ST7735_CYAN, ST7735_BLACK, 2);

    /* ---- Centered teal accent divider ---- */
    fill_rrect(24, 83, 80, 3, 1, COL_TEAL);

    /* ---- Key status — inset rounded pill ---- */
    bool locked = !(g_latest_digital.port_a & (1u << IOEXP_A_KEY));
    uint16_t key_col = locked ? ST7735_RED : ST7735_GREEN;
    fill_rrect(4, 88, 120, 12, 4, key_col);
    /* "KEY: LOCKED"/"KEY:  OPEN ": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 91, locked ? "KEY: LOCKED" : "KEY:  OPEN ",
                       ST7735_WHITE, key_col, 1);

    /* ---- Switch states ---- */
    snprintf(buf, sizeof(buf), "SW A:%02X  B:%02X",
             g_latest_digital.port_a & 0xFF,
             g_latest_digital.port_b & 0xFF);
    st7735_draw_string(4, 103, buf, COL_LTGRAY, ST7735_BLACK, 1);

    /* ---- Pi status — inset rounded pill ---- */
    sys_state_t state = g_sys_state;
    bool connected = (state == SYS_CONNECTED || state == SYS_ACTIVE);
    uint16_t pi_col = connected ? ST7735_GREEN : COL_AMBER;
    fill_rrect(4, 114, 120, 12, 4, pi_col);
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

    /* Short rounded orange accent pill */
    fill_rrect(14, 101, 100, 4, 2, ST7735_ORANGE);
    /* Rounded footer panel */
    fill_rrect(4, 105, 120, 23, 4, ST7735_DARKGREY);
    /* "WARNING" size 2: 7*12=84px, x=(128-84)/2=22 */
    st7735_draw_string(22, 109, "WARNING", ST7735_RED, ST7735_DARKGREY, 2);
}

static void render_lock(void)
{
    st7735_fill_screen(COL_CHARCOAL);

    /* Shackle: pill-shaped arch top + two straight legs */
    /* Arch (r = h/2 → full stadium shape): x=46, y=18, w=36, h=14, r=7 */
    fill_rrect(46, 18, 36, 14, 7, COL_LTGRAY);
    /* Legs connect at y=24 (where arch reaches full width) */
    st7735_fill_rect(46, 24, 8, 34, COL_LTGRAY);   /* left leg  y=24–57 */
    st7735_fill_rect(74, 24, 8, 34, COL_LTGRAY);   /* right leg y=24–57 */

    /* Body: rounded amber rectangle; body overdraws bottom of legs */
    fill_rrect(32, 56, 64, 44, 6, COL_AMBER);
    draw_rrect(32, 56, 64, 44, 6, ST7735_YELLOW);  /* yellow border */

    /* Keyhole centred in body: x=(128-12)/2=58 */
    fill_rrect(58, 64, 12, 22, 3, COL_CHARCOAL);

    /* "LOCKED" size 2: 6*12=72px, x=(128-72)/2=28 */
    st7735_draw_string(28, 106, "LOCKED", ST7735_RED, COL_CHARCOAL, 2);
}

static void update_batwarning(void)
{
    bool flash = (s_batwarning_frame & 1);

    /* Repaint battery interior: toggle red fill on/off */
    fill_rrect(10, 28, 92, 44, 4, COL_CHARCOAL);
    if (!flash) {
        fill_rrect(10, 28, 14, 44, 4, ST7735_RED);
        st7735_draw_vline(24, 28, 44, COL_AMBER);
    }

    /* Flash "CHARGE NOW!" between white and yellow */
    uint16_t txt_col = flash ? ST7735_YELLOW : ST7735_WHITE;
    st7735_draw_string(31, 113, "CHARGE NOW!", txt_col, ST7735_RED, 1);

    s_batwarning_frame++;
}

static void render_batwarning(void)
{
    s_batwarning_frame = 0;
    st7735_fill_screen(COL_CHARCOAL);

    /* Red header with amber accent line */
    st7735_fill_rect(0,  0, 128, 18, ST7735_RED);
    /* "LOW BATTERY": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 5, "LOW BATTERY", ST7735_WHITE, ST7735_RED, 1);
    st7735_fill_rect(0, 18, 128,  2, COL_AMBER);

    /* Battery icon with rounded body */
    fill_rrect( 8, 26,  96, 48, 5, ST7735_DARKGREY);  /* outer border   */
    fill_rrect(10, 28,  92, 44, 4, COL_CHARCOAL);     /* interior       */
    st7735_fill_rect(104, 40, 10, 18, ST7735_DARKGREY); /* terminal nub   */
    st7735_fill_rect(106, 42,  6, 14, COL_CHARCOAL);  /* terminal inner */
    fill_rrect(10, 28,  14, 44, 4, ST7735_RED);        /* ~15% red fill  */
    st7735_draw_vline(24, 28, 44, COL_AMBER);          /* charge marker  */

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
/* Peripheral helper                                                      */
/* ------------------------------------------------------------------ */

static const char *periph_name(uint8_t addr)
{
    switch (addr) {
        case 0x01: return "Searchlight";
        case 0x02: return "Radar";
        case 0x03: return "Pan-Tilt";
        case 0x04: return "Light Bar";
        default:   return "Device";
    }
}

/* ------------------------------------------------------------------ */
/* Peripheral overview screen (mode 5)                                   */
/* ------------------------------------------------------------------ */

static void render_periph_overview(void)
{
    st7735_fill_screen(ST7735_BLACK);
    char buf[24];

    /* Header */
    st7735_fill_rect(0, 0, 128, 16, COL_NAVY);
    st7735_fill_rect(0, 16, 128, 1, COL_TEAL);
    /* "PERIPHERALS": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 4, "PERIPHERALS", ST7735_WHITE, COL_NAVY, 1);

    /* Pull fresh data from RS-485 layer */
    uint8_t addrs[PERIPH_MAX_DISPLAY];
    bool    online[PERIPH_MAX_DISPLAY];
    uint8_t count = rs485_get_peripherals(addrs, online, PERIPH_MAX_DISPLAY);

    uint8_t online_count = 0;
    for (int i = 0; i < count; i++) {
        int16_t row_y = 20 + i * 12;
        bool    is_on = online[i];
        if (is_on) online_count++;

        /* Address */
        snprintf(buf, sizeof(buf), "%02X", addrs[i]);
        st7735_draw_string(4, row_y, buf, COL_LTGRAY, ST7735_BLACK, 1);

        /* Name */
        st7735_draw_string(22, row_y, periph_name(addrs[i]),
                           ST7735_WHITE, ST7735_BLACK, 1);

        /* Status pill */
        uint16_t pill_col = is_on ? ST7735_GREEN : ST7735_RED;
        fill_rrect(97, row_y - 1, 27, 10, 3, pill_col);
        st7735_draw_string(100, row_y, is_on ? "ON " : "OFF",
                           ST7735_WHITE, pill_col, 1);
    }

    /* Footer summary */
    st7735_fill_rect(0, 114, 128, 1, COL_TEAL);
    st7735_fill_rect(0, 115, 128, 13, COL_NAVY);
    snprintf(buf, sizeof(buf), "%d devs  %d online", count, online_count);
    int16_t tw = (int16_t)(strlen(buf) * 6);
    st7735_draw_string((128 - tw) / 2, 118, buf, COL_TEAL, COL_NAVY, 1);
}

/* ------------------------------------------------------------------ */
/* Peripheral detail screen (mode 6)                                     */
/* ------------------------------------------------------------------ */

static void render_periph_detail(void)
{
    st7735_fill_screen(ST7735_BLACK);
    char buf[24];

    /* Header with device name */
    st7735_fill_rect(0, 0, 128, 16, COL_NAVY);
    st7735_fill_rect(0, 16, 128, 1, COL_TEAL);
    const char *name = periph_name(s_detail_addr);
    int16_t tw = (int16_t)(strlen(name) * 6);
    st7735_draw_string((128 - tw) / 2, 4, name, ST7735_WHITE, COL_NAVY, 1);

    /* Body — layout depends on peripheral type */
    if (s_detail_len == 0) {
        /* "No data yet": 11*6=66px, x=31 */
        st7735_draw_string(31, 50, "No data yet", COL_LTGRAY, ST7735_BLACK, 1);
    } else {
        switch (s_detail_addr) {

            case 0x01: /* Searchlight: brightness(u8), temp_C(i8), faults(u8) */
                if (s_detail_len >= 3) {
                    uint8_t brightness = s_detail_buf[0];
                    int8_t  temp_c     = (int8_t)s_detail_buf[1];
                    uint8_t faults     = s_detail_buf[2];

                    st7735_draw_string(4, 22, "Brightness", COL_LTGRAY, ST7735_BLACK, 1);
                    int pct = (int)((brightness * 100u) / 255u);
                    snprintf(buf, sizeof(buf), "%3d%%", pct);
                    st7735_draw_string(88, 22, buf, ST7735_WHITE, ST7735_BLACK, 1);
                    fill_rrect(4, 32, 120, 6, 3, COL_CHARCOAL);
                    int bw = (int)((brightness * 120u) / 255u);
                    if (bw > 0) fill_rrect(4, 32, bw, 6, 3, ST7735_CYAN);

                    st7735_draw_string(4, 44, "Temp", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%d C", (int)temp_c);
                    uint16_t t_col = (temp_c < 60) ? ST7735_GREEN :
                                     (temp_c < 75) ? COL_AMBER : ST7735_RED;
                    st7735_draw_string(60, 44, buf, t_col, ST7735_BLACK, 2);

                    st7735_draw_string(4, 68, "Faults", COL_LTGRAY, ST7735_BLACK, 1);
                    if (faults == 0) {
                        st7735_draw_string(60, 68, "NONE", ST7735_GREEN, ST7735_BLACK, 1);
                    } else {
                        if (faults & 0x01) st7735_draw_string(4, 80, "Overcurrent",
                                               ST7735_RED, ST7735_BLACK, 1);
                        if (faults & 0x02) st7735_draw_string(4, 90, "Overtemp",
                                               ST7735_RED, ST7735_BLACK, 1);
                        if (faults & 0x04) st7735_draw_string(4, 100, "Undervolt",
                                               ST7735_RED, ST7735_BLACK, 1);
                    }
                }
                break;

            case 0x02: /* Radar: distance_mm(u16 LE), signal(u8), status(u8) */
                if (s_detail_len >= 4) {
                    uint16_t dist_mm = (uint16_t)s_detail_buf[0]
                                     | ((uint16_t)s_detail_buf[1] << 8);
                    uint8_t  signal  = s_detail_buf[2];
                    uint8_t  status  = s_detail_buf[3];

                    st7735_draw_string(4, 22, "Distance", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%u mm", (unsigned)dist_mm);
                    int16_t dw = (int16_t)(strlen(buf) * 12);
                    st7735_draw_string((128 - dw) / 2, 34, buf,
                                       ST7735_CYAN, ST7735_BLACK, 2);

                    st7735_draw_string(4, 58, "Signal", COL_LTGRAY, ST7735_BLACK, 1);
                    fill_rrect(4, 68, 120, 6, 3, COL_CHARCOAL);
                    int sw = (int)((signal * 120u) / 255u);
                    if (sw > 0) fill_rrect(4, 68, sw, 6, 3, ST7735_GREEN);

                    st7735_draw_string(4, 80, "Status", COL_LTGRAY, ST7735_BLACK, 1);
                    st7735_draw_string(60, 80, status == 0 ? "OK " : "ERR",
                                       status == 0 ? ST7735_GREEN : ST7735_RED,
                                       ST7735_BLACK, 1);
                }
                break;

            case 0x03: /* Pan-Tilt: pan_deg(u8), tilt_deg(u8), moving(u8) */
                if (s_detail_len >= 3) {
                    uint8_t pan    = s_detail_buf[0];
                    uint8_t tilt   = s_detail_buf[1];
                    uint8_t moving = s_detail_buf[2];

                    st7735_draw_string(4, 22, "Pan", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%3d deg", (int)pan);
                    st7735_draw_string(50, 22, buf, ST7735_CYAN, ST7735_BLACK, 2);

                    st7735_draw_string(4, 48, "Tilt", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%3d deg", (int)tilt);
                    st7735_draw_string(50, 48, buf, ST7735_CYAN, ST7735_BLACK, 2);

                    st7735_draw_string(4, 76, "Motion", COL_LTGRAY, ST7735_BLACK, 1);
                    uint16_t m_col = moving ? COL_AMBER : ST7735_GREEN;
                    fill_rrect(60, 74, 40, 12, 4, m_col);
                    st7735_draw_string(64, 77, moving ? "MOVE" : "IDLE",
                                       ST7735_WHITE, m_col, 1);
                }
                break;

            default: /* Generic: hex dump of first 8 bytes */
                st7735_draw_string(4, 22, "Raw data:", COL_LTGRAY, ST7735_BLACK, 1);
                for (int i = 0; i < s_detail_len && i < 8; i++) {
                    snprintf(buf, sizeof(buf), "%02X", s_detail_buf[i]);
                    st7735_draw_string(4 + i * 18, 36, buf,
                                       ST7735_WHITE, ST7735_BLACK, 1);
                }
                break;
        }
    }

    /* Footer: address + online status */
    st7735_fill_rect(0, 114, 128, 1, COL_TEAL);
    st7735_fill_rect(0, 115, 128, 13, COL_NAVY);

    uint8_t addrs[PERIPH_MAX_DISPLAY];
    bool    online_flags[PERIPH_MAX_DISPLAY];
    uint8_t cnt = rs485_get_peripherals(addrs, online_flags, PERIPH_MAX_DISPLAY);
    bool is_on = false;
    for (int i = 0; i < cnt; i++) {
        if (addrs[i] == s_detail_addr) { is_on = online_flags[i]; break; }
    }

    snprintf(buf, sizeof(buf), "0x%02X  %s",
             s_detail_addr, is_on ? "ONLINE" : "OFFLINE");
    tw = (int16_t)(strlen(buf) * 6);
    st7735_draw_string((128 - tw) / 2, 118, buf,
                       is_on ? ST7735_GREEN : ST7735_RED, COL_NAVY, 1);
}

/* ------------------------------------------------------------------ */
/* Peripheral screen public API                                           */
/* ------------------------------------------------------------------ */

void screen_periph_set_detail_addr(uint8_t addr)
{
    s_detail_addr  = addr;
    s_detail_len   = 0;
    s_detail_dirty = true;
}

void screen_periph_update_data(uint8_t addr, uint8_t cmd,
                               const uint8_t *payload, uint8_t len)
{
    if (addr != s_detail_addr) return;
    s_detail_cmd = cmd;
    if (len > PERIPH_DETAIL_BUF) len = PERIPH_DETAIL_BUF;
    memcpy(s_detail_buf, payload, len);
    s_detail_len   = len;
    s_detail_dirty = true;
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
    vTaskDelay(pdMS_TO_TICKS(5000));   /* hold boot screen for 5 s */

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
                case SCREEN_MODE_MAIN:          render_main();           break;
                case SCREEN_MODE_WARNING:       render_warning();        break;
                case SCREEN_MODE_LOCK:          render_lock();           break;
                case SCREEN_MODE_BATWARNING:    render_batwarning();     break;
                case SCREEN_MODE_PERIPH:        render_periph_overview(); break;
                case SCREEN_MODE_PERIPH_DETAIL: render_periph_detail();  break;
                case SCREEN_MODE_AUTO + 10: s_wait_frame = 0; render_waiting(); break;
                case 0xFF:                                   break; /* boot — skip */
                default:                        render_main();           break;
            }
        } else if (effective == SCREEN_MODE_MAIN && main_needs_redraw()) {
            render_main();
        } else if (effective == SCREEN_MODE_AUTO + 10) {
            update_waiting_dots();
        } else if (effective == SCREEN_MODE_BATWARNING) {
            update_batwarning();
        } else if (effective == SCREEN_MODE_PERIPH_DETAIL && s_detail_dirty) {
            s_detail_dirty = false;
            render_periph_detail();
        }

        vTaskDelayUntil(&last_wake, period);
    }
}
