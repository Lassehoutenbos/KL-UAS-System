# GCS Screen UI Design Plan
**Platform:** Raspberry Pi · Qt/QML · Two 1024×600 7" Touchscreens
**Status:** Design draft

---

## 1. Physical Setup

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│                                                                                  │
│  ┌──────────────────────┐  ┌──────────────┐  ┌──────────────────────┐           │
│  │                      │  │   WARNING    │  │                      │           │
│  │    SCREEN LEFT       │  │    PANEL     │  │    SCREEN RIGHT      │           │
│  │    1024 × 600        │  │  (9 icons)   │  │    1024 × 600        │           │
│  │    touchscreen       │  │              │  │    touchscreen       │           │
│  │                      │  │  SW1 ×3      │  │                      │           │
│  │                      │  │  SW2 ×3      │  │                      │           │
│  │                      │  │  SW3 ×2      │  │                      │           │
│  │                      │  │  KEY         │  │                      │           │
│  │                      │  │  BTN ×2 RGB  │  │                      │           │
│  └──────────────────────┘  └──────────────┘  └──────────────────────┘           │
│                                                                                  │
└──────────────────────────────────────────────────────────────────────────────────┘
```

Both screens are **independent** — each runs the full menu system and can show any page simultaneously. They do not mirror each other. The operator can have different pages on each screen at the same time.

---

## 2. Color System

Defined in `gcs_ui_design_system.md`. Summary for quick reference:

| Token | Hex | Use |
|---|---|---|
| `bg-primary` | `#0b0d10` | Main background |
| `bg-secondary` | `#12161b` | Cards, panels |
| `bg-elevated` | `#1a2027` | Buttons idle, raised surfaces |
| `text-primary` | `#e8edf2` | Values, headings |
| `text-secondary` | `#8f9baa` | Labels, units |
| `text-disabled` | `#5c6673` | Inactive |
| `accent-yellow` | `#e3d049` | Active button, selected tab |
| `accent-blue` | `#7fd6ff` | Telemetry data, graphs |
| `status-ok` | `#62d48f` | Connected, armed-safe |
| `status-warn` | `#ffb347` | Warning |
| `status-crit` | `#ff5b5b` | Error, critical, ARM |
| `border` | `#2a313a` | Borders, dividers |

---

## 3. Typography

Font: **Inter** or **Roboto Condensed** — both render cleanly at small sizes on 170 DPI.

| Role | Size | Weight | Color |
|---|---|---|---|
| Page title | 13px | 600 | `text-secondary` |
| Section label | 11px | 500 | `text-secondary` |
| Data value (large) | 32px | 700 | `text-primary` |
| Data value (medium) | 20px | 600 | `text-primary` |
| Data value (small) | 14px | 500 | `text-primary` |
| Button label | 13px | 600 | `text-primary` |
| Unit label | 11px | 400 | `text-secondary` |

Use **uppercase** for all labels and button text. Values always in the largest size that fits.

---

## 4. Screen Layout Structure

Every page uses the same three-zone structure:

```
┌──────────────────────────────── 1024px ─────────────────────────────────┐
│  STATUS BAR                                                    40px      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  PAGE CONTENT                                                  488px     │
│                                                                          │
├─────────────────────────────────────────────────────────────────────────┤
│  NAV BAR                                                       72px      │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.1 Status Bar (40px, always visible)

```
┌──────────────────────────────────────────────────────────────────────────┐
│  ● LINK  │  GPS 14  FIX-3D  │  BAT 22.4V ████░  │  ALT 045m  │  16:42  │
└──────────────────────────────────────────────────────────────────────────┘
```

| Segment | Content | Notes |
|---|---|---|
| Link status | `● CONNECTED` / `○ NO LINK` | Color: ok/crit |
| GPS | `GPS 14  FIX-3D` | Satellites + fix type |
| Battery | Voltage + 5-segment bar | From ADC CH0 |
| Altitude | MSL from MAVLink | |
| Clock | Local time HH:MM | |

All segments separated by `│` dividers. Right-side items are GCS-local state; left side is drone state.

### 4.2 Nav Bar (72px, always visible)

8 tabs, each 128×72px. Active tab uses `accent-yellow` label and bottom border (4px).

```
┌──────┬──────┬────────┬──────┬─────────┬────────┬────────┬──────┐
│      │      │        │      │         │        │        │      │
│ DASH │DRONE │ CAMERA │ MAP  │ MISSION │ PARAMS │ PERIPH │ CASE │
│  ⬡   │  ✈   │   ◉    │  ⊞   │   ◎     │   ≡    │   ⬡    │  ⚙   │
└──────┴──────┴────────┴──────┴─────────┴────────┴────────┴──────┘
```

Icon above label, both centered. Active tab: `accent-yellow` on both. Inactive: `text-secondary`.

---

## 5. Pages

### 5.1 DASH — Dashboard

Primary status overview. This is the default page on startup.

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├───────────────────────┬────────────────────────────┬─────────────────────┤
│  DRONE                │                            │  GCS                │
│  ─────────────────    │       ATTITUDE             │  ─────────────────  │
│  ALT    045.2 m       │      INDICATOR             │  LINK   ● OK        │
│  SPEED  12.4 m/s      │        (HUD)               │  GPS    14  3D-FIX  │
│  HDG    214°          │                            │  ALS    840 lux     │
│  VSPD   +0.3 m/s      │   ┌────────────────┐      │  TEMP   32°C        │
│                       │   │  ╱─────────────│      │                     │
│  ─────────────────    │   │ ╱   horizon    │      │  ─────────────────  │
│  MODE   LOITER        │   │────────────────│      │  WARNINGS           │
│  ARMED  ● ARMED       │   │ \              │      │  TEMP   ● OK        │
│                       │   └────────────────┘      │  SIG    ● OK        │
│  ─────────────────    │                            │  DRONE  ● OK        │
│  BAT    22.4V  89%    │   PITCH  +2.1°             │  GPS    ● OK        │
│  EXT    12.1V         │   ROLL   -0.8°             │  LINK   ● OK        │
│                       │   YAW    214°              │  NET    ● WARN      │
└───────────────────────┴────────────────────────────┴─────────────────────┘
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Left column (280px):** Drone quick stats in labeled value pairs. Section dividers as thin lines (`#2a313a`). Values in `text-primary`, labels in `text-secondary`.

**Center (464px):** Artificial horizon / HUD widget. Pitch ladder, roll arc, heading tape at bottom. All drawn in `accent-blue` on dark. Numeric readouts below the HUD widget.

**Right column (280px):** GCS hardware state. Warning panel icons mirrored here as colored dots with labels.

---

### 5.2 DRONE — MAVLink Telemetry & Control

Full drone state with controls for mode, arm, and flight parameters.

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────────────────────┬───────────────────────────────────┤
│  TELEMETRY                           │  CONTROL                          │
│                                      │                                   │
│  ┌──────────┐  ┌──────────┐          │  FLIGHT MODE                      │
│  │ ALT      │  │ SPEED    │          │  ┌────────┐ ┌────────┐ ┌────────┐ │
│  │ 045.2    │  │  12.4    │          │  │STABILIZE│ │ LOITER │ │  AUTO  │ │
│  │    m     │  │   m/s    │          │  └────────┘ └────────┘ └────────┘ │
│  └──────────┘  └──────────┘          │  ┌────────┐ ┌────────┐ ┌────────┐ │
│  ┌──────────┐  ┌──────────┐          │  │  ALT   │ │  RTL   │ │ GUIDED │ │
│  │ HDG      │  │ VSPD     │          │  └────────┘ └────────┘ └────────┘ │
│  │  214°    │  │ +0.3     │          │                                   │
│  │          │  │   m/s    │          │  ─────────────────────────────    │
│  └──────────┘  └──────────┘          │                                   │
│  ┌──────────┐  ┌──────────┐          │  ┌───────────────────────────┐    │
│  │ SAT      │  │ HDOP     │          │  │         ARM / DISARM       │    │
│  │   14     │  │  0.8     │          │  │     (KEY REQUIRED)         │    │
│  └──────────┘  └──────────┘          │  └───────────────────────────┘    │
│  ┌──────────────────────────┐         │                                   │
│  │ BATTERY    22.4V  89%    │         │  SYSTEM MESSAGE                   │
│  │ ████████████████████░░░  │         │  > Loiter mode engaged            │
│  └──────────────────────────┘         │  > GPS 3D fix acquired            │
└──────────────────────────────────────┴───────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Left (520px):** Telemetry cards, 2 columns. Each card: label top-left small, value center large, unit bottom-right small. Background `bg-secondary`, border `border`.

**Right (504px):** Mode buttons — 6 in a 3×2 grid, 156×72px each. Selected mode uses `accent-yellow` border + label. ARM button is 100% wide, 72px tall, `status-crit` background when armed, `bg-elevated` when disarmed. Requires key-switch unlock — grayed out with `LOCKED` overlay if key is locked. System message log: last 3 MAVLink status messages.

---

### 5.3 CAMERA — Video Feed

Source: USB capture card connected to Pi. Displayed via Qt Multimedia `VideoOutput`.

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │                                                                    │  │
│  │   ALT 045m                                          HDG 214°       │  │
│  │                                                                    │  │
│  │                                                                    │  │
│  │                      VIDEO FEED                                    │  │
│  │                  (USB capture card)                                │  │
│  │                                                                    │  │
│  │   SPD 12.4m/s     [+]    [●]    [⊡]         BAT 22.4V            │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│  ZOOM ──────●─────────── │  REC ● 00:04:32  │  SNAP  │  OVERLAY ON   │  │
├──────────────────────────────────────────────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Video area (1024×420px):** Full-width video stream from USB capture card. HUD overlay elements pinned to corners and edges. Semi-transparent `rgba(0,0,0,0.35)` backing behind each HUD element to preserve readability.

**HUD overlay elements:**
- Top-left: `ALT` altitude
- Top-right: `HDG` heading
- Bottom-left: `SPD` ground speed
- Bottom-center: zoom in/out `[+][-]`, record `[●]`, snapshot `[⊡]`
- Bottom-right: `BAT` voltage

**Bottom controls strip (68px):** Zoom slider, record state + timer, snapshot button, overlay toggle. All 68px tall — fat touch targets.

---

### 5.4 MAP — Live Map

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────┬───────────────────────────────────────────────────┤
│  WAYPOINTS     ×3    │                                                   │
│  ─────────────────   │                                                   │
│  WP1  TAKEOFF        │                                                   │
│       0m  0°         │                  MAP                              │
│  WP2  WAYPOINT       │            (full tile area)                       │
│       450m  214°     │                                                   │
│  WP3  LAND           │         ✈ drone position                          │
│       0m   30°       │           with heading chevron                    │
│                      │                                                   │
│  ─────────────────   │                                                   │
│  DIST  1.24 km       │                                                   │
│  ETE   04:12         │                                                   │
│                      │                                                   │
│  [CENTER DRONE]      │                                                   │
│  [ZOOM IN ]          │                                                   │
│  [ZOOM OUT]          │                                                   │
└──────────────────────┴───────────────────────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Left sidebar (220px):** Waypoint list, scrollable. Each WP: number, type, distance, bearing. Total distance and ETE below. Buttons: `CENTER DRONE`, `ZOOM IN`, `ZOOM OUT` — 220×56px each.

**Map (804px × 488px):** Tile-based. Online source: OpenStreetMap. Offline source: pre-downloaded MBTiles file. Toggle in quick-settings panel (swipe-down). Drone position as directional chevron icon in `accent-yellow`. Waypoints as numbered circles. Lines connecting waypoints in `accent-blue`. Tap waypoint to inspect.

---

### 5.5 MISSION — Mission Planner

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────────────────────────────┬───────────────────────────┤
│  MAP (editable)                              │  MISSION EDITOR           │
│                                              │                           │
│  [ tap to add waypoint ]                     │  ┌─────────────────────┐  │
│                                              │  │ WP1  TAKEOFF   15m  │  │
│           ●WP1                               │  ├─────────────────────┤  │
│            │                                 │  │ WP2  WP    450m 30° │  │
│           ●WP2──────●WP3                     │  ├─────────────────────┤  │
│                      │                       │  │ WP3  WP    200m 90° │  │
│                     ●WP4 (land)              │  ├─────────────────────┤  │
│                                              │  │ WP4  LAND           │  │
│                                              │  └─────────────────────┘  │
│                                              │                           │
│                                              │  [+ ADD WP]  [- DEL WP]   │
│                                              │                           │
│                                              │  ─────────────────────    │
│                                              │  [  UPLOAD TO DRONE  ]    │
│                                              │  [ DOWNLOAD FROM DRONE]   │
│                                              │  [   CLEAR MISSION   ]    │
└──────────────────────────────────────────────┴───────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Map (640px):** Interactive, tap to place waypoint. Drag existing waypoints to reposition. Active waypoint highlighted in `accent-yellow`.

**Editor (384px):** Scrollable waypoint list. Tap a row to select and edit parameters (altitude, speed, action) in an inline expand. ADD/DEL buttons 184×56px. UPLOAD/DOWNLOAD/CLEAR full-width 72px tall.

---

### 5.6 PARAMS — Flight Parameters

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├────────────────────┬─────────────────────────────────────────────────────┤
│  SEARCH            │  PARAMETERS                                         │
│  [______________]  │                                                     │
│                    │  ┌─────────────────────────────────────────────┐    │
│  GROUPS            │  │  ATC_ANG_RLL_P       4.50     [  -  ][ + ]  │    │
│  ─────────────     │  ├─────────────────────────────────────────────┤    │
│  ● ATTITUDE        │  │  ATC_ANG_PIT_P       4.50     [  -  ][ + ]  │    │
│  ○ TUNING          │  ├─────────────────────────────────────────────┤    │
│  ○ BATTERY         │  │  ATC_ANG_YAW_P       4.50     [  -  ][ + ]  │    │
│  ○ GPS             │  ├─────────────────────────────────────────────┤    │
│  ○ COMPASS         │  │  ATC_RATE_RLL_P      0.135    [  -  ][ + ]  │    │
│  ○ FAILSAFE        │  ├─────────────────────────────────────────────┤    │
│  ○ RADIO           │  │  ATC_RATE_PIT_P      0.135    [  -  ][ + ]  │    │
│  ○ MOTORS          │  └─────────────────────────────────────────────┘    │
│                    │                                                     │
│                    │  [  WRITE TO DRONE  ]    [  REFRESH FROM DRONE  ]   │
└────────────────────┴─────────────────────────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Left sidebar (220px):** Search field at top. Parameter group list below — tap to filter. Active group in `accent-yellow`.

**Right (804px):** Scrollable parameter list. Each row: parameter name left, current value center, decrement/increment buttons right (64×56px each). Changed-but-not-written values shown in `status-warn`. WRITE and REFRESH buttons at bottom, full width, 64px tall.

---

### 5.7 PERIPH — RS-485 Peripherals

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────────┬───────────────────────────────────────────────┤
│  DEVICES          4/4    │  SEARCHLIGHT          ● ONLINE  0x01          │
│  ─────────────────────   │  ────────────────────────────────────────     │
│  ● 0x01  SEARCHLIGHT     │  BRIGHTNESS                                   │
│  ● 0x02  RADAR           │  ┌──────────────────────────────────────┐     │
│  ○ 0x03  PAN-TILT        │  │  ────────────●──────────────────     │     │
│  ● 0x04  LIGHT BAR       │  └──────────────────────────────────────┘     │
│                          │   OFF ──────────────────────────────── FULL   │
│                          │   Current:  78%                                │
│                          │                                               │
│                          │  TEMPERATURE       42°C    ● OK               │
│                          │  FAULTS            NONE                        │
│                          │                                               │
│                          │  ─────────────────────────────────────────    │
│                          │  [ STREAM ON  100ms ]     [ GET STATUS ]       │
│                          │  [ SET PARAM... ]         [ PING ]             │
└──────────────────────────┴───────────────────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**Left sidebar (260px):** Device list. Each row: online/offline dot (green/red), address, name. Tap to select. Offline rows dimmed. Below the list, a `[ VIEW ON TFT ]` button (full sidebar width, 56px) sends `PROTO_TYPE_PERIPH_SCREEN` with the selected peripheral's address. The TFT switches to `SCREEN_MODE_PERIPH_DETAIL` (mode 6) for that device. Button is disabled (grayed) if no device is selected or the selected device is offline.

An additional `[ TFT: OVERVIEW ]` button (full sidebar width, 56px) directly below sends `PROTO_TYPE_SCREEN` mode 5 (`SCREEN_MODE_PERIPH`) to show the overview list of all peripherals on the TFT instead of a single detail view.

```
│  DEVICES          4/4    │
│  ─────────────────────   │
│  ● 0x01  SEARCHLIGHT     │  ← selected (accent-yellow border)
│  ● 0x02  RADAR           │
│  ○ 0x03  PAN-TILT        │
│  ● 0x04  LIGHT BAR       │
│                          │
│  [ VIEW ON TFT  ]        │  ← sends PERIPH_SCREEN for selected addr
│  [ TFT: OVERVIEW ]       │  ← sends SCREEN mode 5 (all peripherals)
```

The active TFT selection is shown as a small tag next to the device name: `[TFT]` in `accent-blue` when that device is currently shown on the TFT detail screen. Only one device can hold this tag at a time. The tag clears when TFT mode changes away from PERIPH_DETAIL.

**Right (764px):** Detail panel for selected device. Layout adapts per peripheral type:
- **Searchlight:** brightness slider + temp + faults
- **Radar:** distance readout (large) + signal strength bar + status
- **Pan-Tilt:** two sliders (pan + tilt) + position readback
- **Light Bar:** brightness + mode selector (solid/flash/breathe) + colour

Command buttons at bottom: STREAM ON/OFF, GET STATUS, SET PARAM, PING — all 180×56px.

---

### 5.8 CASE — GCS Case Settings

Software control panel for everything not handled by hardware switches. Three sections: Brightness, Temperatures, TFT Control.

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                              │
├──────────────────────────────────────────────────────────────────────────┤
│  BRIGHTNESS                                                              │
│  ────────────────────────────────────────────────────────────────────   │
│                                                                          │
│  MASTER    ───────────────────────────●──────────  80%                  │
│            (scales all outputs below proportionally)                    │
│                                                                          │
│  SCREENS   ───────────────────────────●──────────  80%   [LINK  ■]      │
│  LED STRIP ─────────────────────●────────────────  60%                  │
│  TFT BL    ──────────────────────────────●───────  90%                  │
│  BTN LEDS  ──────────────────────────────●───────  90%                  │
│                                                                          │
│  AMBIENT LIGHT AUTO-BRIGHTNESS   [ ENABLED  ▶ ]                         │
│  ALS sensor adjusts all outputs automatically based on lux reading.     │
│                                                                          │
│  ────────────────────────────────────────────────────────────────────   │
│  TEMPERATURES                                                            │
│                                                                          │
│  PI CPU         52°C   ●OK    │  CASE CH2   38°C   ●OK                  │
│  CASE CH3       41°C   ●OK    │  CASE CH4   --      ●--                 │
│  SEARCHLIGHT    42°C   ●OK    │  CASE CH5   --      ●--                 │
│                                                                          │
│  ────────────────────────────────────────────────────────────────────   │
│  TFT SCREEN MODE                                                         │
│  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌────────┐          │
│  │ AUTO  │ │ MAIN  │ │ WARN  │ │ LOCK  │ │  BAT  │ │ PERIPH │          │
│  └───────┘ └───────┘ └───────┘ └───────┘ └───────┘ └────────┘          │
├──────────────────────────────────────────────────────────────────────────┤
│  NAV BAR                                                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

**MASTER slider:** Single slider at top, 0–100%. Moving it scales all brightness outputs proportionally (screens, LED strip, TFT backlight, button LEDs). Individual sliders below still allow trimming the ratio between outputs.

**SCREENS `[LINK ■]` toggle:** When LINK is active, Screen L and Screen R move together. Tap to unlink and adjust independently (toggle becomes `[UNLINKED □]`, a second SCREEN R slider appears).

**AMBIENT LIGHT toggle:** Enables/disables VEML7700-driven auto-brightness. When enabled, the MASTER slider shows the current ALS-computed level (read-only, grayed handle). When disabled, MASTER is user-controlled. Sends `PROTO_TYPE_BRIGHTNESS` on each ALS update cycle when enabled.

**TEMPERATURES:** Source mapping:
- `PI CPU` — Linux `/sys/class/thermal/thermal_zone0/temp`
- `CASE CH2–CH5` — ADC CH2–CH5 via NTC thermistor (if populated; shows `--` if channel reads rail voltage = no sensor)
- `SEARCHLIGHT` — live from RS-485 `GET_STATUS` response; shown only if peripheral is online
- Color coding: `●OK` = `status-ok` (<60°C), `●WARN` = `status-warn` (60–80°C), `●CRIT` = `status-crit` (>80°C)

**TFT SCREEN MODE:** Fully automatic — the Pi drives the TFT mode based on system state. No manual override needed.

| Condition | TFT Mode |
|---|---|
| Normal operation | `MAIN` (mode 1) |
| Any `WARN_WARNING` active | `WARNING` (mode 2) |
| Key switch locked | `LOCK` (mode 3) |
| Battery below threshold | `BAT_WARNING` (mode 4) |
| Any peripheral online | `PERIPH` (mode 5) — cycles through connected devices |

Priority: CRIT battery > WARN > LOCK > PERIPH > MAIN. The CASE page shows the **current active TFT mode** as a read-only status label only.

**INDICATOR LEDS (LED2, LED3, LED4 on MCP23017 PA0–PA2):** Fully automatic — driven by system state, not manually toggled.

| LED | Condition ON |
|---|---|
| LED2 | MAVLink connected (`status-ok` solid) |
| LED3 | Drone armed |
| LED4 | Any active warning (`WARN_WARNING` or above) — blinks at `LED_ANIM_BLINK_SLOW` |

These are never shown as controls in the UI.

---

## 6. Swipe-Down Quick Panel

Triggered by swiping down anywhere on the status bar. Slides in from the top with a fast spring animation (150ms ease-out). Dismissed by swiping up, tapping the scrim, or a dedicated close button. Independent per screen — opening it on Screen L does not affect Screen R.

```
┌──────────────────────────────────────────────────────────────────────────┐
│  STATUS BAR  (still visible above the panel)                             │
├──────────────────────────────────────────────────────────────────────────┤
│  ╔════════════════════════════════════════════════════════════════════╗   │
│  ║  QUICK PANEL                                          [ ✕ CLOSE ] ║   │
│  ╠════════════════════════════════════════════════════════════════════╣   │
│  ║  QUICK TOGGLES                                                     ║   │
│  ║  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────┐  ║   │
│  ║  │  MAP: ONLINE │ │  ALS: ON     │ │ READING LIGHT│ │  OVERLAY │  ║   │
│  ║  │   ● ONLINE   │ │  ● ENABLED   │ │     OFF      │ │   HUD ON │  ║   │
│  ║  └──────────────┘ └──────────────┘ └──────────────┘ └──────────┘  ║   │
│  ╠════════════════════════════════════════════════════════════════════╣   │
│  ║  SYSTEM                                                             ║   │
│  ║  PI CPU     52°C  ●OK  │  UPTIME   01:24:38  │  MEM   34%          ║   │
│  ║  CASE CH2   38°C  ●OK  │  DISK     12%       │  PICO  ● CONNECTED  ║   │
│  ╠════════════════════════════════════════════════════════════════════╣   │
│  ║  LINK QUALITY                                                       ║   │
│  ║  MAVLINK  ████████████████████░░   RSSI  -72 dBm   SNR  18 dB      ║   │
│  ║  PICO CDC ████████████████████████  HB 1000ms seq 124              ║   │
│  ║  ALS      840 lux   GAIN AUTO   INT 100ms                          ║   │
│  ╚════════════════════════════════════════════════════════════════════╝   │
│                                                                          │
│  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ SCRIM ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   │
└──────────────────────────────────────────────────────────────────────────┘
```

**Panel background:** `bg-secondary` (`#12161b`), full width, height ~280px. Drops over the page content. Scrim below: `rgba(0,0,0,0.45)`.

### Quick Toggles (row of 4 large buttons, 240×72px each)

| Button | State A | State B | Action |
|---|---|---|---|
| MAP SOURCE | `● ONLINE` | `○ OFFLINE` | Switch map tile source |
| ALS | `● ENABLED` | `○ DISABLED` | Toggle ambient light auto-brightness |
| READING LIGHT | `OFF` / `ON` | — | Toggle Light Bar peripheral (0x04) |
| HUD OVERLAY | `HUD ON` | `HUD OFF` | Toggle camera page HUD |

Active state uses `accent-yellow` border. Inactive uses `border` border.

### System Stats

Two rows of inline stats. Labels `text-secondary`, values `text-primary`. Temperature values color-coded with status dots.

| Stat | Source |
|---|---|
| PI CPU temp | `/sys/class/thermal/thermal_zone0/temp` |
| CASE CH2–CH5 | ADC CH2–CH5 NTC readings |
| UPTIME | Linux `/proc/uptime` |
| MEM | `/proc/meminfo` |
| DISK | `df /` |
| PICO | Heartbeat state from `PicoLink` |

### Link Quality

Signal bars (20-segment) for MAVLink radio RSSI + CDC link. Numeric RSSI, SNR. Heartbeat sequence counter (confirms Pico is alive). ALS current reading + gain + integration time setting.

---

## 7. Submenus & Overlays

Some actions trigger a modal overlay (darkened background, centered panel). Overlays are used for:

### 6.1 Confirm Action Overlay

Used for ARM, CLEAR MISSION, UPLOAD PARAMS.

```
┌──────────────────────────────────────────────┐
│                                              │
│   ARM DRONE                                  │
│   ──────────────────────────────────         │
│   Key switch must be unlocked.               │
│   This action will arm motors.               │
│                                              │
│   ┌──────────────┐   ┌──────────────┐        │
│   │    CANCEL    │   │   CONFIRM    │        │
│   └──────────────┘   └──────────────┘        │
│                                              │
└──────────────────────────────────────────────┘
```

CANCEL uses `bg-elevated`. CONFIRM uses `status-crit` for destructive actions, `status-ok` for safe ones.

### 6.2 Parameter Edit Overlay

Triggered by tapping a parameter value.

```
┌──────────────────────────────────────────────┐
│   ATC_ANG_RLL_P                              │
│   Attitude roll angle P gain                 │
│   ──────────────────────────────             │
│   Current:  4.50    Range: 1.0 – 20.0        │
│                                              │
│   ┌───────────────────────────────────────┐  │
│   │          [  4.50  ]                   │  │
│   └───────────────────────────────────────┘  │
│   ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐    │
│   │ 1  │  │ 2  │  │ 3  │  │ .  │  │ ←  │    │
│   └────┘  └────┘  └────┘  └────┘  └────┘    │
│   ┌──────────────┐   ┌──────────────┐        │
│   │    CANCEL    │   │     SET      │        │
│   └──────────────┘   └──────────────┘        │
└──────────────────────────────────────────────┘
```

Numeric keypad only (no full keyboard needed for parameters). Value validated against range before SET is enabled.

---

## 7. Button Dimensions

Touch targets on 1024×600 at 170 DPI. Physical screen is ~15cm × 9cm.

| Type | Width | Height | Notes |
|---|---|---|---|
| Nav tab | 128px | 72px | Bottom nav |
| Primary action | full or 220px | 72px | ARM, UPLOAD, etc. |
| Mode selector | 156px | 72px | Flight modes, TFT modes |
| Peripheral preset | 156px | 56px | OFF/50%/100% |
| Slider control | full width | 48px | Touch-friendly thumb |
| Parameter row | full width | 56px | With ±buttons at 64×56px |
| Overlay button | 180px | 64px | Confirm/Cancel |

Minimum touch target: **56px × 56px** (≈8mm × 8mm physical). No touch target smaller than this.

---

## 8. Dual-Screen Behavior

Each screen is an independent Qt window (`Window` in QML). Both windows run the same `Main.qml` but maintain separate navigation state.

**State isolation:** Each window tracks its own `currentPage` property. No shared state between screens for navigation. Sensor data (MAVLink, ADC, digital I/O) is shared via a singleton model/backend — both screens read the same live data but navigate independently.

**Startup default:**
- Screen Left → DASH page
- Screen Right → DRONE page

This can be changed by the operator at any time.

**Multi-window in QML:**
```qml
// main.cpp: load two windows
engine.loadFromModule("PICODE", "Main");  // Screen 1
engine.loadFromModule("PICODE", "Main");  // Screen 2
// Position via QScreen geometry
```

Each `Main.qml` instance gets a `screenIndex` property (0 or 1) passed from C++ to set initial page and screen positioning.

---

## 9. Hardware Event Mapping

Physical inputs from the Pico (via `PROTO_TYPE_EVENT`) drive UI state:

| Hardware | Event ID | UI Action |
|---|---|---|
| KEY switch → locked | `EVT_KEY_LOCK_CHANGED` value=0 | ARM button shows `LOCKED` overlay; disables all destructive actions |
| KEY switch → unlocked | `EVT_KEY_LOCK_CHANGED` value=1 | ARM button enabled |
| SW1[0] toggle | `EVT_SWITCH_CHANGED` PB0 | User-assignable in CASE page |
| SW1[1] toggle | `EVT_SWITCH_CHANGED` PB1 | User-assignable |
| SW1[2] toggle | `EVT_SWITCH_CHANGED` PB2 | User-assignable |
| SW2[0] press | `EVT_BUTTON_PRESSED` | User-assignable |
| SW2[1] press | `EVT_BUTTON_PRESSED` | User-assignable |
| SW2[2] press | `EVT_BUTTON_PRESSED` | User-assignable |
| RGB BTN 0 press | `EVT_BUTTON_PRESSED` btn=0 | User-assignable |
| RGB BTN 1 press | `EVT_BUTTON_PRESSED` btn=1 | User-assignable |

SW3 small toggles and SW2 are user-assignable from the CASE page — a simple 2-column assignment grid maps each switch to a function (e.g. toggle reading light, switch TFT mode, etc.).

Warning panel icons are driven by the Pi sending `PROTO_TYPE_WARNING` — the UI backend maps MAVLink health data to the 9 severity levels and sends the command on change.

---

## 10. QML Architecture

```
PICODE/
├── main.cpp               — launch two windows on two screens
├── Main.qml               — root window, navigation state
├── components/
│   ├── StatusBar.qml
│   ├── NavBar.qml
│   ├── DataCard.qml       — value + label + unit tile
│   ├── BigButton.qml      — touch-optimized button
│   ├── SliderRow.qml      — label + slider + value
│   ├── ConfirmOverlay.qml
│   └── StatusDot.qml      — colored ● indicator
├── pages/
│   ├── DashPage.qml
│   ├── DronePage.qml
│   ├── CameraPage.qml
│   ├── MapPage.qml
│   ├── MissionPage.qml
│   ├── ParamsPage.qml
│   ├── PeriphPage.qml
│   └── CasePage.qml
└── backend/
    ├── PicoLink.qml        — USB CDC serial, sends/receives protocol packets
    ├── MavlinkLink.qml     — MAVLink serial/UDP parser
    └── GCSState.qml        — singleton: all live data exposed as Q_PROPERTY
```

`GCSState` is a singleton registered in C++. Both `Main.qml` instances read from it. Only `PicoLink` and `MavlinkLink` write to it.

---

## 11. Implementation Priority Order

1. `GCSState` singleton + `PicoLink` CDC parser — no UI is useful without live data
2. `StatusBar` + `NavBar` + page scaffold — navigation skeleton
3. `CasePage` — immediate value: brightness and TFT control work on day one
4. `DashPage` — most-used page, validates the data pipeline
5. `DronePage` — MAVLink integration
6. `PeriphPage` — RS-485 peripherals
7. `CameraPage` — video stream integration
8. `MapPage` — map tile integration
9. `MissionPage` — mission editor
10. `ParamsPage` — parameter read/write
