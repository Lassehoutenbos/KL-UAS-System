# Pi Code — Hardware Reference

This document describes all physical inputs, outputs, and sensors available to the Raspberry Pi via the Pico over USB CDC. It serves as the primary reference for writing Pi-side (Qt/QML) code that interacts with the GCS hardware.

---

## Architecture Overview

```
Raspberry Pi (Qt/QML app)
    │
    USB CDC  (/dev/ttyACM0, 115200 baud)
    │
RP2040 Pico (FreeRTOS firmware)
    ├── MCP23017 I2C GPIO expander  — switches, key, indicator LEDs
    ├── MCP3208 SPI ADC             — battery, ext voltage, 4× NTC temp
    ├── VEML7700 I2C                — ambient light sensor
    ├── SK6812 LED strip (PIO)      — panel lighting + 9 warning icons
    ├── WS2811 LED chain (PIO)      — RGB button backlights
    ├── ST7735S TFT 128×128 (SPI)   — small status display
    └── RS-485 UART1                — peripheral bus (rear connector)
```

All Pi ↔ Pico communication uses the CDC packet protocol (SOF `0xAA`, type, length, payload, checksum). See `protocol.h` for packet definitions.

---

## Switches

All switches are active-low, active when pulled to GND. The MCP23017 has internal pull-ups enabled on all input pins. The Pico reads switch state in `digital_io_task` and sends it to the Pi as:
- **Periodic digital state** (`PROTO_TYPE_DIGITAL`, `0x02`) — contains raw `port_a` and `port_b` bytes
- **Input events** (`PROTO_TYPE_EVENT`, `0x06`) — sent on state change

### SW1 — Armed switches (covered)

Three independent covered toggle switches. Each has a flip-up safety cover and two states (on/off).

| Contact | MCP23017 pin | Port | Bit | Label | Function |
|---------|-------------|------|-----|-------|----------|
| SW1_1 | GPA0 (Port B bit 0) | B | 0 | **ARM** | Master arm switch |
| SW1_2 | GPA1 (Port B bit 1) | B | 1 | **Remote control icon** | Remote control enable |
| SW1_3 | GPA2 (Port B bit 2) | B | 2 | **Return home icon** | Return-to-home enable |

**State**: `0` = off (cover closed / switch off), `1` = on (cover open, switch flipped)

**Pi-side access**: Read `port_b` bits 0–2 from `PROTO_TYPE_DIGITAL` packets, or react to `EVT_SWITCH_CHANGED` events.

### SW2 — Momentary buttons (with indicator LEDs)

Three momentary push buttons. Each button has a built-in indicator LED driven by MCP23017 Port A outputs (LED2–LED4). The LEDs indicate when the associated function is enabled or active.

| Contact | MCP23017 pin | Port | Bit | Label | Function |
|---------|-------------|------|-----|-------|----------|
| SW2_1 | Port B bit 3 | B | 3 | **Crosshair** | Target / aim function |
| SW2_2 | Port B bit 4 | B | 4 | **1** | Function button 1 |
| SW2_3 | Port B bit 5 | B | 5 | **2** | Function button 2 |

**State**: Momentary — `1` while pressed, `0` when released.

**Indicator LEDs** (accent LEDs inside the SW2 buttons):

| LED | MCP23017 pin | Port | Bit | Indicates |
|-----|-------------|------|-----|-----------|
| LED2 | Port A bit 0 | A | 0 | SW2_1 (crosshair) active |
| LED3 | Port A bit 1 | A | 1 | SW2_2 (button 1) active |
| LED4 | Port A bit 2 | A | 2 | SW2_3 (button 2) active |

These LEDs are controlled by the Pico firmware. The Pi can set them via `PROTO_TYPE_LED` with `chain=0x02` (MCP23017 LEDs).

### SW3 — Payload switches

SW3 is a mixed group controlling two payload channels (PAY1 and PAY2). Each payload channel has an arm toggle switch and a momentary action button with a WS2811 RGB backlight.

#### Payload arm toggles

| Contact | MCP23017 pin | Port | Bit | Label | Function |
|---------|-------------|------|-----|-------|----------|
| SW3_1 | Port B bit 6 | B | 6 | **PAY1 ARM** | Arm payload 1 |
| SW3_2 | Port B bit 7 | B | 7 | **PAY2 ARM** | Arm payload 2 |

**State**: Toggle — `1` = armed, `0` = disarmed.

#### Payload action buttons (with WS2811 RGB LEDs)

| Contact | MCP23017 pin | Port | Bit | Label | Function |
|---------|-------------|------|-----|-------|----------|
| SW3_3 | Port A bit 6 | A | 6 | **PAY1** | Fire / activate payload 1 |
| SW3_4 | Port A bit 7 | A | 7 | **PAY2** | Fire / activate payload 2 |

**State**: Momentary — `1` while pressed, `0` when released.

These buttons have built-in WS2811 RGB LEDs on the WS2811 chain, controllable via `PROTO_TYPE_LED` with `chain=0x01`. Use LED animation modes (`LED_ANIM_*`) to indicate payload state (e.g. blink when armed, solid when firing).

### Key switch

| Contact | MCP23017 pin | Port | Bit | Function |
|---------|-------------|------|-----|----------|
| KEY | Port A bit 5 | A | 5 | System lock/unlock |

The key switch controls the **locked state** of the entire GCS. When locked, no state changes can be made from the control panel. The Pico sends `EVT_KEY_LOCK_CHANGED` (`0x04`) when the key is turned, and the TFT display switches to lock screen mode.

**Pi-side access**: `GCSState::keyUnlocked` property, updated by `EVT_KEY_LOCK_CHANGED` events.

---

## Complete MCP23017 bit map

### Port A (GPIOA) — `0x12`

| Bit | Direction | Assignment |
|-----|-----------|-----------|
| 0 | Output | LED2 — SW2_1 (crosshair) indicator |
| 1 | Output | LED3 — SW2_2 (button 1) indicator |
| 2 | Output | LED4 — SW2_3 (button 2) indicator |
| 3 | — | Unused |
| 4 | — | Unused |
| 5 | Input | KEY — key switch (lock/unlock) |
| 6 | Input | SW3_3 — PAY1 momentary button |
| 7 | Input | SW3_4 — PAY2 momentary button |

IODIRA = `0b11111000` (bits 0–2 output, 3–7 input)

### Port B (GPIOB) — `0x13`

| Bit | Direction | Assignment |
|-----|-----------|-----------|
| 0 | Input | SW1_1 — ARM (covered toggle) |
| 1 | Input | SW1_2 — Remote control (covered toggle) |
| 2 | Input | SW1_3 — Return home (covered toggle) |
| 3 | Input | SW2_1 — Crosshair (momentary button) |
| 4 | Input | SW2_2 — Button 1 (momentary button) |
| 5 | Input | SW2_3 — Button 2 (momentary button) |
| 6 | Input | SW3_1 — PAY1 ARM (toggle) |
| 7 | Input | SW3_2 — PAY2 ARM (toggle) |

IODIRB = `0xFF` (all inputs)

---

## Sensors

### ADC — MCP3208 (8-channel, 12-bit, SPI)

| Channel | Constant | Sensor | Location | Conversion |
|---------|----------|--------|----------|------------|
| 0 | `ADC_CH_BAT_VIN` | Voltage divider (8.02:1) | Battery input | `raw / 4095 × 3.3 × 8.021` = volts |
| 1 | `ADC_CH_EXT_VIN` | Voltage divider (8.02:1) | External power input | Same as battery |
| 2 | `ADC_CH_SENS2` | 10k NTC thermistor (B=3950) | Power circuit on PCB | NTC formula, celsius |
| 3 | `ADC_CH_SENS3` | 10k NTC thermistor (B=3950) | Raspberry Pi | NTC formula, celsius |
| 4 | `ADC_CH_SENS4` | 10k NTC thermistor (B=3950) | Charger module | NTC formula, celsius |
| 5 | `ADC_CH_SENS5` | 10k NTC thermistor (B=3950) | Back of VRX module | NTC formula, celsius |
| 6–7 | — | Reserved | — | — |

**Pi-side access**: `GCSState` properties `batteryVoltage`, `batteryPercent`, `extVoltage`, `tempCaseA`–`tempCaseD`.

**NTC conversion** (used in `picolink.cpp`):
```
V = raw / 4095 × 3.3
R = (V × 10000) / (3.3 - V)
T = 1 / (1/298.15 + ln(R/10000) / 3950) - 273.15   [celsius]
```

**Battery percentage**: Linear map from 19.8 V (0%) to 25.2 V (100%) — 6S LiPo.

### Ambient light sensor — VEML7700 (I2C)

| Parameter | Value |
|-----------|-------|
| I2C address | `0x10` |
| Bus | I2C0 (shared with MCP23017) |
| Output | ALS raw, white raw, millilux |

Sent as `PROTO_TYPE_ALS` (`0x0B`). Pi-side: `GCSState::alsLux`.

Used for automatic brightness control when `alsAutoEnabled` is true.

### CPU temperature (Pi-side)

Read directly by the Pi from `/sys/class/thermal/thermal_zone0/temp`. Not a Pico sensor.

Pi-side: `GCSState::tempCpuPi`.

---

## LED chains

### SK6812 strip (GRBW, 800 kHz, GP0)

The SK6812 chain drives switch backlighting, the warning panel, and the worklights.

| LED range | Purpose |
|-----------|---------|
| 0–13 | SW2 switch backlights (3 buttons × 3 LEDs, with gaps) |
| 14–27 | SW1 switch backlights (3 switches × 3 LEDs, with gaps) |
| 28–47 | Remaining panel illumination |
| 38–46 | Warning panel icons — controlled via `PROTO_TYPE_WARNING` (`0x0A`) |
| 48–70 | Worklights (23 LEDs, act as one unified light) — controlled via `PROTO_TYPE_WORKLIGHT` (`0x10`) |

#### Switch backlight layout

The strip starts under **SW2 B3** (crosshair). Each switch has **3 LEDs** underneath, followed by **1 skipped LED** (off/spacer), then the next switch. SW2 and SW1 each occupy 14 LEDs on the strip.

**SW2 section (LEDs 0–13):**

| LED index | Assignment |
|-----------|-----------|
| 0, 1, 2 | SW2_1 — Crosshair button |
| 3 | skip (spacer) |
| 4, 5, 6 | SW2_2 — Button 1 |
| 7 | skip (spacer) |
| 8, 9, 10 | SW2_3 — Button 2 |
| 11, 12, 13 | padding |

**SW1 section (LEDs 14–27):**

| LED index | Assignment |
|-----------|-----------|
| 14, 15, 16 | SW1_1 — ARM (covered toggle) |
| 17 | skip (spacer) |
| 18, 19, 20 | SW1_2 — Remote control (covered toggle) |
| 21 | skip (spacer) |
| 22, 23, 24 | SW1_3 — Return home (covered toggle) |
| 25, 26, 27 | padding |

#### Worklights (LEDs 48–70)

The last 23 LEDs of the strip (indices 48–70) form the **worklights**. These are managed entirely by the Pico firmware — the Pi sends a single `PROTO_TYPE_WORKLIGHT` (`0x10`) packet and the Pico fills all 23 LEDs with the requested uniform colour on every refresh cycle.

**Payload** (`worklight_cmd_t`, 4 bytes):

| Byte | Field | Description |
|------|-------|-------------|
| 0 | `on` | `0` = off, `1` = on |
| 1 | `r` | Red (0–255) |
| 2 | `g` | Green (0–255) |
| 3 | `b` | Blue (0–255) |

Brightness scaling (`PROTO_TYPE_BRIGHTNESS`, `target=0`) is applied by the Pico before writing to the pixel buffer.

**Pi-side**: `GCSState::cmdWorklightChanged(bool on, QColor color)` signal → `PicoLink::onWorklightChanged()` — sends one packet instead of 23 individual LED commands.

See `warning_panel.md` for warning icon mapping and severity behaviour.

**Brightness**: `PROTO_TYPE_BRIGHTNESS` with `target=0` (`BRIGHTNESS_TGT_SK6812`).

### WS2811 chain (RGB, 400 kHz, GP1)

Drives RGB LEDs inside the payload action buttons (SW3_3 PAY1, SW3_4 PAY2).

**Control**: `PROTO_TYPE_LED` with `chain=0x01`. Animation modes:

| Mode | Constant | Behavior |
|------|----------|----------|
| 0 | `LED_ANIM_OFF` | Off |
| 1 | `LED_ANIM_ON` | Solid on |
| 2 | `LED_ANIM_BLINK_SLOW` | Blink 500 ms period |
| 3 | `LED_ANIM_BLINK_FAST` | Blink 100 ms period |
| 4 | `LED_ANIM_PULSE` | Sine-wave breathing |

**Brightness**: `PROTO_TYPE_BRIGHTNESS` with `target=1` (`BRIGHTNESS_TGT_WS2811`).

### MCP23017 indicator LEDs

The three indicator LEDs inside the SW2 buttons (LED2, LED3, LED4) are driven via MCP23017 Port A bits 0–2.

**Control**: `PROTO_TYPE_LED` with `chain=0x02`.

---

## TFT display — ST7735S (128×128, SPI1)

Small status screen on the control panel. Driven entirely by the Pico; the Pi selects which screen mode to show.

| Mode | Constant | Description |
|------|----------|-------------|
| 0 | `SCREEN_MODE_AUTO` | Pico state-machine driven |
| 1 | `SCREEN_MODE_MAIN` | Main status screen |
| 2 | `SCREEN_MODE_WARNING` | Warning detail screen |
| 3 | `SCREEN_MODE_LOCK` | Lock screen (key switch locked) |
| 4 | `SCREEN_MODE_BATWARNING` | Low battery warning |
| 5 | `SCREEN_MODE_PERIPH` | Peripheral bus overview |
| 6 | `SCREEN_MODE_PERIPH_DETAIL` | Single peripheral detail |

**Control**: `PROTO_TYPE_SCREEN` (`0x04`) with `screen_cmd_t.mode`.
**Peripheral detail**: `PROTO_TYPE_PERIPH_SCREEN` (`0x0F`) with target address.
**Backlight brightness**: `PROTO_TYPE_BRIGHTNESS` with `target=2` (`BRIGHTNESS_TGT_TFT_BLK`).

The Pi auto-selects the TFT mode in `PicoLink::updateTftMode()` based on priority: low battery > active warnings > key locked > peripherals online > main.

---

## RS-485 peripheral bus

The rear GX16-8 connector exposes an RS-485 bus for external peripherals (searchlight, radar, pan-tilt, lighting bar). The Pico acts as bus master.

**Pi-side interaction**:
- Send commands to peripherals: `GCSState::sendPeriphCmd(address, cmd, payload)` -> `PROTO_TYPE_PERIPH_CMD` (`0x0C`)
- Receive peripheral data: `PROTO_TYPE_PERIPH_DATA` (`0x0D`) -> `GCSState::peripherals` list
- Receive online/offline notifications: `PROTO_TYPE_PERIPH_STATE` (`0x0E`)

See `RS485_PERIPHERAL_BUS.md` for full bus protocol, addressing, and peripheral examples.

---

## CDC protocol quick reference

All packets: `[0xAA] [type] [len_lo] [len_hi] [payload...] [checksum]`

Checksum = XOR of type, len_lo, len_hi, and all payload bytes.

### Pico -> Pi

| Type | Name | Payload struct | Description |
|------|------|----------------|-------------|
| `0x01` | ADC | `adc_packet_t` (14 B) | 6 ADC channels + timestamp |
| `0x02` | DIGITAL | `digital_packet_t` (4 B) | Port A + Port B + timestamp |
| `0x05` | HEARTBEAT | `heartbeat_pkt_t` (1 B) | Sequence number echo |
| `0x06` | EVENT | `event_pkt_t` (3 B) | Input event (switch change, button press, key) |
| `0x07` | ERROR | `error_pkt_t` (1 B) | Pico error (watchdog, stack overflow, malloc) |
| `0x0B` | ALS | `als_packet_t` (10 B) | Ambient light: raw + millilux + timestamp |
| `0x0D` | PERIPH_DATA | `periph_data_t` | RS-485 peripheral response forwarded |
| `0x0E` | PERIPH_STATE | `periph_state_t` (2 B) | Peripheral online/offline change |

### Pi -> Pico

| Type | Name | Payload struct | Description |
|------|------|----------------|-------------|
| `0x03` | LED | `led_cmd_header_t` + data | Set LED pixels/animation |
| `0x04` | SCREEN | `screen_cmd_t` (1 B) | Set TFT screen mode |
| `0x05` | HEARTBEAT | `heartbeat_pkt_t` (1 B) | Keep-alive |
| `0x08` | BRIGHTNESS | `brightness_cmd_t` (2 B) | Set brightness (target + level) |
| `0x09` | MODE | `mode_cmd_t` (1 B) | State machine override |
| `0x0A` | WARNING | `warning_cmd_t` (9 B) | Set warning panel severities |
| `0x0C` | PERIPH_CMD | `periph_cmd_t` | Forward command to RS-485 peripheral |
| `0x0F` | PERIPH_SCREEN | `periph_screen_cmd_t` (1 B) | Select peripheral for TFT detail view |
| `0x10` | WORKLIGHT | `worklight_cmd_t` (4 B) | Set worklight on/off + RGB colour (Pico fills all 23 LEDs) |

---

## Event IDs (`PROTO_TYPE_EVENT`, `0x06`)

| ID | Constant | Value field | Trigger |
|----|----------|-------------|---------|
| `0x01` | `EVT_SWITCH_CHANGED` | `port_a << 8 \| port_b` | Any switch/button state change |
| `0x02` | `EVT_BUTTON_PRESSED` | Button ID | Momentary button pressed |
| `0x03` | `EVT_BUTTON_RELEASED` | Button ID | Momentary button released |
| `0x04` | `EVT_KEY_LOCK_CHANGED` | `0`=locked, `1`=unlocked | Key switch turned |
| `0x05` | `EVT_SOURCE_DETECTED` | Source bitmask | Power source change |
| `0x06` | `EVT_USB_CONNECTED` | — | USB host connected |
| `0x07` | `EVT_USB_DISCONNECTED` | — | USB host disconnected |

---

## GCSState properties (Pi-side, QML-accessible)

The `GCSState` singleton exposes all hardware state to the QML UI:

| Category | Properties |
|----------|-----------|
| Battery | `batteryVoltage`, `batteryPercent`, `extVoltage` |
| Temperatures | `tempCaseA` (PCB), `tempCaseB` (Pi), `tempCaseC` (charger), `tempCaseD` (VRX), `tempCpuPi` |
| Light sensor | `alsLux`, `alsGain`, `alsIntMs` |
| Pico link | `picoConnected`, `picoHeartbeatMs`, `picoHeartbeatSeq` |
| Key switch | `keyUnlocked` |
| Brightness | `brightnessScreenL`, `brightnessScreenR`, `brightnessLed`, `brightnessTft`, `brightnessBtnLeds`, `alsAutoEnabled` |
| Peripherals | `peripherals` (QVariantList of online devices) |
| System | `uptimeSeconds`, `memPercent`, `diskPercent` |
| Warnings | `warnTemp`, `warnSignal`, `warnDrone`, `warnGps`, `warnLink`, `warnNetwork`, `anyWarningActive` |
| Worklight | `cmdWorklightOn`, `cmdWorklightColor` — emits `cmdWorklightChanged(bool, QColor)` → single `PROTO_TYPE_WORKLIGHT` packet |
