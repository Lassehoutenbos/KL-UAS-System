/mod# Peripheral Status Screens — Implementation Guide

This document describes the exact code changes required to add peripheral overview and detail screens to the GCS firmware. It extends the RS-485 peripheral bus integration described in `RS485_PERIPHERAL_BUS.md`.

---

## Overview of changes

| File | Change |
|------|--------|
| `src/protocol.h` | Add `PROTO_TYPE_PERIPH_SCREEN` (0x0F), payload struct, and screen mode defines |
| `src/protocol.c` | Handle `PROTO_TYPE_PERIPH_SCREEN` in `proto_handle_rx()` |
| `src/screen_display.h` | Add `SCREEN_MODE_PERIPH` (5) and `SCREEN_MODE_PERIPH_DETAIL` (6) |
| `src/screen_display.c` | Add peripheral overview renderer, detail renderer, and data cache |
| `src/rs485.h` | Add `rs485_get_online_flags()` accessor and peripheral detail data struct |
| `src/rs485.c` | Expose online flags; cache last STATUS/STREAM_DATA per peripheral |

---

## 1. New CDC packet type — `src/protocol.h`

Append to the existing `PROTO_TYPE_*` defines (after `PROTO_TYPE_PERIPH_STATE 0x0E`):

```c
#define PROTO_TYPE_PERIPH_SCREEN 0x0F  /* Pi->Pico: select peripheral detail screen */
```

Add the payload struct alongside the other peripheral structs:

```c
/* Type 0x0F -- Peripheral screen select (Pi -> Pico) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* peripheral address to show on detail screen */
} periph_screen_cmd_t;
```

---

## 2. New screen mode defines — `src/screen_display.h`

Add after the existing `SCREEN_MODE_BATWARNING`:

```c
#define SCREEN_MODE_PERIPH        5   /* peripheral overview list       */
#define SCREEN_MODE_PERIPH_DETAIL 6   /* single peripheral detail view  */
```

Add a function to push peripheral data into the screen module:

```c
/**
 * Update the cached peripheral data used by the detail screen.
 * Called from rs485_task when a STATUS or STREAM_DATA frame arrives
 * for the currently selected detail peripheral.
 */
void screen_periph_set_detail_addr(uint8_t addr);
void screen_periph_update_data(uint8_t addr, uint8_t cmd,
                               const uint8_t *payload, uint8_t len);
```

---

## 3. Handle the new CDC packet — `src/protocol.c`

In `proto_handle_rx()`, inside the checksum-verified switch, add a case after `PROTO_TYPE_WARNING`:

```c
case PROTO_TYPE_PERIPH_SCREEN:
    if (s_rx_len >= 1) {
        screen_periph_set_detail_addr(s_rx_buf[0]);
        if (s_screen_handle) {
            xTaskNotify(s_screen_handle,
                        (uint32_t)SCREEN_MODE_PERIPH_DETAIL,
                        eSetValueWithOverwrite);
        }
    }
    break;
```

Also update the existing `PROTO_TYPE_SCREEN` case to accept modes 5 and 6 — no change needed since it already passes `s_rx_buf[0]` through as the mode value.

Add `#include "screen_display.h"` at the top of `protocol.c` (it already includes `screen_st7735.h`).

---

## 4. Peripheral data cache and screen state — `src/screen_display.c`

### 4a. Includes and shared state

Add at the top with the other includes:

```c
#include "rs485.h"
```

### 4b. Peripheral state cache

Add after the existing `s_last_sysstate` snapshot variables:

```c
/* ------------------------------------------------------------------ */
/* Peripheral screen state                                               */
/* ------------------------------------------------------------------ */
#define PERIPH_MAX_DISPLAY  8
#define PERIPH_DETAIL_BUF   32   /* max cached payload bytes */

/* Online flags — updated from rs485_task via notify or shared memory */
static uint8_t  s_periph_addrs[PERIPH_MAX_DISPLAY] = {
    0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00
};
static bool     s_periph_online[PERIPH_MAX_DISPLAY];
static uint8_t  s_periph_count = 4;   /* active entries in the array */

/* Detail screen state */
static uint8_t  s_detail_addr = 0x01;
static uint8_t  s_detail_cmd  = 0;
static uint8_t  s_detail_buf[PERIPH_DETAIL_BUF];
static uint8_t  s_detail_len  = 0;
static bool     s_detail_dirty = false;
```

### 4c. Public API implementations

```c
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
```

### 4d. Peripheral name lookup

```c
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
```

### 4e. Render: peripheral overview screen

```c
static void render_periph_overview(void)
{
    st7735_fill_screen(ST7735_BLACK);
    char buf[24];

    /* Header */
    st7735_fill_rect(0, 0, 128, 16, COL_NAVY);
    st7735_fill_rect(0, 16, 128, 1, COL_TEAL);
    /* "PERIPHERALS": 11*6=66px, x=(128-66)/2=31 */
    st7735_draw_string(31, 4, "PERIPHERALS", ST7735_WHITE, COL_NAVY, 1);

    /* Device rows — 12 px per row, starting at y=20 */
    uint8_t online_count = 0;
    for (int i = 0; i < s_periph_count && i < PERIPH_MAX_DISPLAY; i++) {
        uint8_t addr = s_periph_addrs[i];
        if (!addr) continue;

        int16_t row_y = 20 + i * 12;
        bool online = s_periph_online[i];
        if (online) online_count++;

        /* Address */
        snprintf(buf, sizeof(buf), "%02X", addr);
        st7735_draw_string(4, row_y, buf, COL_LTGRAY, ST7735_BLACK, 1);

        /* Name */
        st7735_draw_string(22, row_y, periph_name(addr),
                           ST7735_WHITE, ST7735_BLACK, 1);

        /* Status pill */
        uint16_t pill_col = online ? ST7735_GREEN : ST7735_RED;
        const char *lbl   = online ? "ON"         : "OFF";
        fill_rrect(97, row_y - 1, 27, 10, 3, pill_col);
        st7735_draw_string(100, row_y, lbl, ST7735_WHITE, pill_col, 1);
    }

    /* Footer */
    st7735_fill_rect(0, 114, 128, 1, COL_TEAL);
    st7735_fill_rect(0, 115, 128, 13, COL_NAVY);
    snprintf(buf, sizeof(buf), "%d devs  %d online",
             s_periph_count, online_count);
    /* Centre the text: estimate width = strlen*6 */
    int16_t tw = (int16_t)(strlen(buf) * 6);
    st7735_draw_string((128 - tw) / 2, 118, buf, COL_TEAL, COL_NAVY, 1);
}
```

### 4f. Render: peripheral detail screen

The detail renderer interprets the cached payload based on the device address and response command. Each peripheral type has its own layout.

```c
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

    /* Body — depends on device type */
    if (s_detail_len == 0) {
        st7735_draw_string(16, 50, "No data yet", COL_LTGRAY, ST7735_BLACK, 1);
    } else {
        switch (s_detail_addr) {
            case 0x01:  /* Searchlight: brightness(u8), temp_C(i8), faults(u8) */
                if (s_detail_len >= 3) {
                    uint8_t brightness = s_detail_buf[0];
                    int8_t  temp_c     = (int8_t)s_detail_buf[1];
                    uint8_t faults     = s_detail_buf[2];

                    /* Brightness label + bar */
                    st7735_draw_string(4, 22, "Brightness", COL_LTGRAY, ST7735_BLACK, 1);
                    int pct = (brightness * 100) / 255;
                    snprintf(buf, sizeof(buf), "%3d%%", pct);
                    st7735_draw_string(88, 22, buf, ST7735_WHITE, ST7735_BLACK, 1);
                    fill_rrect(4, 32, 120, 6, 3, COL_CHARCOAL);
                    int bw = (brightness * 120) / 255;
                    if (bw > 0) fill_rrect(4, 32, bw, 6, 3, ST7735_CYAN);

                    /* Temperature */
                    st7735_draw_string(4, 44, "Temp", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%d C", temp_c);
                    uint16_t t_col = (temp_c < 60) ? ST7735_GREEN :
                                     (temp_c < 75) ? COL_AMBER : ST7735_RED;
                    st7735_draw_string(60, 44, buf, t_col, ST7735_BLACK, 2);

                    /* Faults */
                    st7735_draw_string(4, 68, "Faults", COL_LTGRAY, ST7735_BLACK, 1);
                    if (faults == 0) {
                        st7735_draw_string(60, 68, "NONE", ST7735_GREEN, ST7735_BLACK, 1);
                    } else {
                        if (faults & 0x01) st7735_draw_string(4, 80, "Overcurrent", ST7735_RED, ST7735_BLACK, 1);
                        if (faults & 0x02) st7735_draw_string(4, 90, "Overtemp",    ST7735_RED, ST7735_BLACK, 1);
                        if (faults & 0x04) st7735_draw_string(4, 100,"Undervolt",   ST7735_RED, ST7735_BLACK, 1);
                    }
                }
                break;

            case 0x02:  /* Radar: distance_mm(u16), signal(u8), status(u8) */
                if (s_detail_len >= 4) {
                    uint16_t dist_mm = (uint16_t)s_detail_buf[0]
                                     | ((uint16_t)s_detail_buf[1] << 8);
                    uint8_t  signal  = s_detail_buf[2];
                    uint8_t  status  = s_detail_buf[3];

                    st7735_draw_string(4, 22, "Distance", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%u mm", dist_mm);
                    st7735_draw_string(16, 34, buf, ST7735_CYAN, ST7735_BLACK, 2);

                    /* Signal strength bar */
                    st7735_draw_string(4, 58, "Signal", COL_LTGRAY, ST7735_BLACK, 1);
                    fill_rrect(4, 68, 120, 6, 3, COL_CHARCOAL);
                    int sw = (signal * 120) / 255;
                    if (sw > 0) fill_rrect(4, 68, sw, 6, 3, ST7735_GREEN);

                    /* Status */
                    st7735_draw_string(4, 80, "Status", COL_LTGRAY, ST7735_BLACK, 1);
                    st7735_draw_string(60, 80, status == 0 ? "OK" : "ERR",
                                       status == 0 ? ST7735_GREEN : ST7735_RED,
                                       ST7735_BLACK, 1);
                }
                break;

            case 0x03:  /* Pan-Tilt: pan_deg(u8), tilt_deg(u8), moving(u8) */
                if (s_detail_len >= 3) {
                    uint8_t pan  = s_detail_buf[0];
                    uint8_t tilt = s_detail_buf[1];
                    uint8_t moving = s_detail_buf[2];

                    st7735_draw_string(4, 22, "Pan", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%3d deg", pan);
                    st7735_draw_string(50, 22, buf, ST7735_CYAN, ST7735_BLACK, 2);

                    st7735_draw_string(4, 48, "Tilt", COL_LTGRAY, ST7735_BLACK, 1);
                    snprintf(buf, sizeof(buf), "%3d deg", tilt);
                    st7735_draw_string(50, 48, buf, ST7735_CYAN, ST7735_BLACK, 2);

                    st7735_draw_string(4, 76, "Motion", COL_LTGRAY, ST7735_BLACK, 1);
                    fill_rrect(60, 75, 40, 12, 4,
                               moving ? COL_AMBER : ST7735_GREEN);
                    st7735_draw_string(64, 78, moving ? "MOVE" : "IDLE",
                                       ST7735_WHITE,
                                       moving ? COL_AMBER : ST7735_GREEN, 1);
                }
                break;

            default:
                /* Generic hex dump for unknown peripheral types */
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

    /* Look up online status */
    bool online = false;
    for (int i = 0; i < PERIPH_MAX_DISPLAY; i++) {
        if (s_periph_addrs[i] == s_detail_addr) {
            online = s_periph_online[i];
            break;
        }
    }
    snprintf(buf, sizeof(buf), "0x%02X  %s", s_detail_addr,
             online ? "ONLINE" : "OFFLINE");
    tw = (int16_t)(strlen(buf) * 6);
    uint16_t st_col = online ? ST7735_GREEN : ST7735_RED;
    st7735_draw_string((128 - tw) / 2, 118, buf, st_col, COL_NAVY, 1);
}
```

### 4g. Hook renderers into the screen task loop

In `screen_task()`, add the new modes to both the mode-changed switch and the per-tick update logic.

**In the mode-changed switch** (where `render_main()`, `render_warning()`, etc. are called):

```c
case SCREEN_MODE_PERIPH:        render_periph_overview(); break;
case SCREEN_MODE_PERIPH_DETAIL: render_periph_detail();   break;
```

**In the per-tick update section** (after the `else if (effective == SCREEN_MODE_BATWARNING)` block):

```c
} else if (effective == SCREEN_MODE_PERIPH_DETAIL && s_detail_dirty) {
    s_detail_dirty = false;
    render_periph_detail();
}
```

The overview screen re-renders on mode change only. The detail screen re-renders whenever `screen_periph_update_data()` sets `s_detail_dirty = true`.

---

## 5. Feed data from RS-485 task to screen — `src/rs485.c`

In the RS-485 task, after forwarding a response to the Pi via `forward_to_pi()`, also push the data to the screen module if the response came from the currently-selected detail peripheral.

Add `#include "screen_display.h"` at the top of `rs485.c`.

In `rs485_task()`, after each `forward_to_pi(resp_addr, resp_cmd, resp_buf, (uint8_t)r)` call:

```c
/* Also feed to display for live detail view */
screen_periph_update_data(resp_addr, resp_cmd, resp_buf, (uint8_t)r);
```

In the PING sweep section, after updating `s_online[i]` and calling `notify_state()`, also update the screen module's cached online flags. The simplest approach is to expose online state via a public accessor:

**In `rs485.h`:**

```c
/**
 * Copy current online flags into caller-provided arrays.
 * Returns the number of entries written (up to max_entries).
 */
uint8_t rs485_get_peripherals(uint8_t *addrs, bool *online, uint8_t max_entries);
```

**In `rs485.c`:**

```c
uint8_t rs485_get_peripherals(uint8_t *addrs, bool *online, uint8_t max_entries)
{
    uint8_t n = 0;
    for (int i = 0; i < RS485_MAX_PERIPHERALS && n < max_entries; i++) {
        if (!s_known_addrs[i]) continue;
        addrs[n]  = s_known_addrs[i];
        online[n] = s_online[i];
        n++;
    }
    return n;
}
```

Then in `render_periph_overview()`, call this before drawing the rows:

```c
s_periph_count = rs485_get_peripherals(s_periph_addrs, s_periph_online,
                                        PERIPH_MAX_DISPLAY);
```

This pulls fresh online/offline data every time the overview screen renders.

---

## 6. Summary of all new/changed code

| File | Lines added | Description |
|------|-------------|-------------|
| `src/protocol.h` | ~6 | `PROTO_TYPE_PERIPH_SCREEN` define + `periph_screen_cmd_t` struct |
| `src/protocol.c` | ~10 | `case PROTO_TYPE_PERIPH_SCREEN` in RX switch + include |
| `src/screen_display.h` | ~8 | Two new `SCREEN_MODE_*` defines + two new function declarations |
| `src/screen_display.c` | ~180 | Peripheral cache, name lookup, overview renderer, detail renderer, update hooks |
| `src/rs485.h` | ~4 | `rs485_get_peripherals()` declaration |
| `src/rs485.c` | ~15 | `rs485_get_peripherals()` implementation + `screen_periph_update_data()` calls |

No changes to `main.c` or `CMakeLists.txt` — the screen task and RS-485 task are already registered.

---

## 7. Pi-side usage (Python reference)

### Show peripheral overview

```python
# Send screen mode command: mode = 5 (SCREEN_MODE_PERIPH)
send_packet(type=0x04, payload=bytes([5]))
```

### Show detail view for a specific peripheral

```python
# Send peripheral screen select: addr = 0x01 (searchlight)
send_packet(type=0x0F, payload=bytes([0x01]))
```

The Pico will automatically switch to the detail screen and begin rendering live data from that peripheral. Subsequent `STATUS` or `STREAM_DATA` responses from the selected address will update the display at the screen task's 5 Hz rate.

### Return to main screen

```python
send_packet(type=0x04, payload=bytes([1]))   # mode 1 = SCREEN_MODE_MAIN
```

---

## 8. Design notes

- **128×128 px constraint:** The overview fits up to 8 device rows at 12 px per row. The detail view uses size-2 font (10×14 px) for primary readings and size-1 (5×7 px) for labels.
- **No blocking:** `screen_periph_update_data()` is a simple memcpy into static buffers — safe to call from the RS-485 task context without blocking.
- **Dirty flag pattern:** The detail screen only redraws when new data arrives (`s_detail_dirty`), avoiding unnecessary SPI traffic to the display.
- **Extensible:** New peripheral types are added by adding a `case` block in `render_periph_detail()` with the appropriate payload parsing. Unknown peripherals fall through to the hex dump view.
- **Style consistency:** All new screens follow the existing Navy/Teal/Charcoal theme with rounded-rectangle pills for status indicators.
