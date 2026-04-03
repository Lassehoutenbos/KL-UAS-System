# RS-485 Peripheral Bus — GCS Extension Interface

This document describes the RS-485 based peripheral bus exposed on the 8-pin rear connector of the GCS flight case. It covers the physical layer, required components, protocol, and example peripheral designs.

---

## Overview

Each peripheral on the bus is an independent MCU-based module. The GCS Pico acts as **bus master**; peripherals are **slaves** that respond only when addressed. The bus uses RS-485 differential signalling, making it robust against EMI, ground noise, and long cable runs — all common in field UAS operations.

```
GCS Pico (UART1)
    │
    GP8 (TX) ──► MAX485 DI
    GP9 (RX) ◄── MAX485 RO
    GP2 (DE/RE)─► MAX485 DE + /RE
    │
    MAX485 A/B ══════════════════════════════════ 8-pin rear connector
                                                        │
                              ┌─────────────────────────┤
                              │                         │
                         Peripheral A             Peripheral B
                         (Searchlight)            (Radar node)
                         MAX485 + MCU             MAX485 + MCU
                         addr = 0x01              addr = 0x02
```

---

## Physical Layer

### Bus characteristics

| Parameter | Value |
|---|---|
| Signalling | RS-485 half-duplex differential |
| Max nodes | 32 (standard) — more with repeater |
| Max cable length | ~1200 m at 9600 baud / ~100 m at 460800 baud |
| Recommended baud rate | 115200 baud (field default) |
| Logic levels | Differential: A−B > +200 mV = mark, A−B < −200 mV = space |
| Termination | 120 Ω across A/B at each **end** of the bus |
| Cable type | Shielded twisted pair (STP), e.g. Belden 9841 or CAT5 |

### 8-pin rear connector pinout

Recommended connector: **GX16-8** circular aviation connector (IP54, field-proven, locking).

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | GND | Common ground — connect shield here too |
| 2 | +5 V | Regulated 5 V for peripheral logic / MCU |
| 3 | VBAT | Fused raw battery voltage for high-power loads |
| 4 | RS-485 A | Differential data line + |
| 5 | RS-485 B | Differential data line − |
| 6 | /INT | Open-drain alert — peripheral pulls low to signal async event |
| 7 | spare | Reserved for future expansion |
| 8 | spare | Reserved for future expansion |

> **Cable colour suggestion (for consistent field builds):**
> GND = black, +5 V = red, VBAT = orange, A = yellow, B = green, /INT = white.

---

## GCS-Side Hardware

### Components

| Component | Part | Package | Purpose |
|---|---|---|---|
| RS-485 transceiver | **SP3485EN** (3.3 V) | SOIC-8 | Converts UART1 to RS-485 differential |
| Bus termination | 120 Ω 0402, 1% | — | Across A/B at the PCB-side bus end |
| ESD / surge protection | **PRTR5V0U2X** or 2× **SMBJ6.5A** TVS | SOT-363 / SMB | On A and B before the connector |
| VBAT fuse | Polyfuse 1 A (e.g. MF-MSMF110) | 1812 | Inline with VBAT connector pin |
| Pull-up / pull-down | 560 Ω to +5 V on A, 560 Ω to GND on B | 0402 | Bias bus to idle mark when no driver active |

### SP3485 wiring (SOIC-8)

```
Pico GP8 (UART1 TX) ──── SP3485 pin 4 (DI)
Pico GP9 (UART1 RX) ──── SP3485 pin 1 (RO)
Pico GP2            ──┬─ SP3485 pin 3 (DE)   ← drive high to transmit
                      └─ SP3485 pin 2 (/RE)  ← tie to DE (both together)

SP3485 pin 6 (A) ──── bus A (via TVS)
SP3485 pin 7 (B) ──── bus B (via TVS)

SP3485 pin 8 (VCC) ── 3.3 V + 100 nF to GND
SP3485 pin 5 (GND) ── GND
```

> Use SP3485 (3.3 V) not MAX485 (5 V) — the Pico is 3.3 V logic. Both are pin-compatible.

### GP2 DE/RE control

In firmware, GP2 must be driven **high before** sending and returned **low after** the last byte is transmitted. The UART shift register must finish before pulling DE low — use `uart_tx_wait_blocking(uart1)` before toggling GP2.

---

## Protocol

The bus uses a lightweight request–response protocol. The GCS Pico is always master. Peripherals never transmit unsolicited except via the `/INT` line.

### Frame structure

```
Byte 0:   SOF  = 0xAB          (start-of-frame, differs from CDC 0xAA)
Byte 1:   ADDR                  (peripheral address 0x01–0xFE; 0xFF = broadcast)
Byte 2:   CMD                   (command / response type)
Byte 3:   LEN                   (payload length, 0–255)
Byte 4…:  PAYLOAD               (LEN bytes)
Last:     CRC8                  (CRC-8/MAXIM over bytes 1…payload end)
```

> Broadcast frames (ADDR = 0xFF) require no response. Use for sync or global commands.

### Address space

| Address | Assignment |
|---------|-----------|
| 0x00 | Reserved |
| 0x01 | Searchlight module |
| 0x02 | Radar / rangefinder node |
| 0x03 | Pan-tilt unit |
| 0x04 | External lighting bar |
| 0x05–0xFE | User-assignable |
| 0xFF | Broadcast |

Addresses should be configured via DIP switches or solder jumpers on each peripheral board.

### Command types (CMD byte)

| CMD | Direction | Name | Description |
|-----|-----------|------|-------------|
| 0x01 | Master → Slave | PING | Alive check. Slave responds with PONG |
| 0x02 | Slave → Master | PONG | Response to PING. Payload: 1-byte firmware version |
| 0x10 | Master → Slave | SET_OUTPUT | Set a digital/PWM output. Payload: `channel (u8), value (u8)` |
| 0x11 | Master → Slave | GET_STATUS | Request full status packet from peripheral |
| 0x12 | Slave → Master | STATUS | Status response. Payload: peripheral-defined struct |
| 0x20 | Master → Slave | SET_PARAM | Write a configuration parameter. Payload: `param_id (u8), value (u16)` |
| 0x21 | Master → Slave | GET_PARAM | Read a configuration parameter. Payload: `param_id (u8)` |
| 0x22 | Slave → Master | PARAM_VAL | Response to GET_PARAM. Payload: `param_id (u8), value (u16)` |
| 0x30 | Master → Slave | STREAM_ON | Start periodic status broadcast at given interval. Payload: `interval_ms (u16)` |
| 0x31 | Master → Slave | STREAM_OFF | Stop periodic streaming |
| 0x32 | Slave → Master | STREAM_DATA | Periodic data frame. Payload: peripheral-defined |
| 0xF0 | Slave → Master | ERROR | Error report. Payload: `error_code (u8)` |
| 0xFF | Broadcast | SYNC | Global time sync. Payload: `timestamp_ms (u32)` |

### Timing

| Parameter | Value |
|---|---|
| Inter-frame gap | ≥ 2 ms |
| Slave response timeout | 50 ms |
| PING interval (health check) | 1000 ms per device |
| Max retries on timeout | 3, then flag device offline |

### CRC-8 polynomial

`x^8 + x^5 + x^4 + 1` (0x31, Dallas/Maxim). Simple to implement on any MCU, no library needed.

```c
uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}
```

---

## Peripheral MCU Recommendation

Any MCU with a UART works. Suggested options for new peripheral boards:

| MCU | Why |
|-----|-----|
| **RP2040 (Pico)** | Same toolchain as GCS, cheap, plenty of I/O |
| **STM32G031** | Tiny, cheap, low power, SOIC-8 or TSSOP-20 |
| **ATmega328P** | Arduino ecosystem, easy prototyping |
| **CH32V003** | Extremely cheap (< €0.15), RISC-V, SOIC-8 |

Each peripheral board needs a **SP3485** (or MAX485 if running at 5 V logic) plus the protection components listed above.

---

## Example Peripheral: Searchlight Module

**Purpose:** High-power LED spotlight with PWM brightness control and thermal monitoring.

**Hardware:**
- MCU: RP2040 or STM32G031
- SP3485 for RS-485
- MOSFET (e.g. IRLZ44N) to switch the LED load from VBAT
- NTC thermistor on the LED heatsink
- Onboard fuse for VBAT

**Protocol usage:**

| CMD | Payload | Effect |
|-----|---------|--------|
| `SET_OUTPUT` | `ch=0, val=0–255` | PWM brightness (0 = off, 255 = full) |
| `GET_STATUS` | — | Returns: `brightness (u8), temp_C (i8), fault_flags (u8)` |
| `SET_PARAM` | `param=0x01, val=1` | Enable thermal shutoff at 80 °C |

**Fault flags byte:**

| Bit | Meaning |
|-----|---------|
| 0 | Overcurrent |
| 1 | Overtemperature |
| 2 | VBAT undervoltage |
| 3–7 | Reserved |

When a fault is detected the module sends an `ERROR` frame and asserts `/INT`.

---

## Example Peripheral: Radar / Rangefinder Node

**Purpose:** Relay distance and detection data from a radar or laser rangefinder to the GCS.

**Hardware:**
- MCU: RP2040 or STM32
- SP3485 for RS-485
- Sensor connected via secondary UART or I2C on the peripheral MCU (e.g. TF-Luna LiDAR via UART, or LD06 radar via UART)

**Protocol usage:**

| CMD | Payload | Effect |
|-----|---------|--------|
| `STREAM_ON` | `interval=100` | Stream detection data every 100 ms |
| `STREAM_DATA` | `distance_mm (u16), signal_strength (u8), status (u8)` | Periodic reading |
| `GET_STATUS` | — | Returns: `mode (u8), range_max_m (u8), sample_rate_hz (u8)` |
| `SET_PARAM` | `param=0x01, val=<mm>` | Set minimum detection threshold |

---

## Example Peripheral: Pan-Tilt Unit

**Purpose:** Two-axis servo-driven camera or sensor mount, controllable from the GCS.

**Hardware:**
- MCU: RP2040
- SP3485 for RS-485
- 2× servo PWM output (pan + tilt)
- Optional: 2× encoder feedback or potentiometer for position readback

**Protocol usage:**

| CMD | Payload | Effect |
|-----|---------|--------|
| `SET_OUTPUT` | `ch=0, val=0–180` | Pan angle in degrees |
| `SET_OUTPUT` | `ch=1, val=0–180` | Tilt angle in degrees |
| `GET_STATUS` | — | Returns: `pan_deg (u8), tilt_deg (u8), moving (u8)` |
| `SET_PARAM` | `param=0x01, val=<deg/s>` | Set slew rate limit |
| `SET_PARAM` | `param=0x02, val=1` | Enable soft limits |

---

## Example Peripheral: External Lighting Bar

**Purpose:** Addressable LED bar (e.g. SK6812) for area illumination or status indication, driven by a dedicated peripheral MCU rather than the main GCS Pico chain.

**Hardware:**
- MCU: RP2040 or ATmega328P
- SP3485 for RS-485
- PIO/timer output to SK6812 strip
- Powered from VBAT via MOSFET switch

**Protocol usage:**

| CMD | Payload | Effect |
|-----|---------|--------|
| `SET_OUTPUT` | `ch=0, val=0–255` | Global brightness |
| `SET_PARAM` | `param=0x01, val=<mode>` | Set lighting mode (0=solid, 1=flash, 2=breathe) |
| `SET_PARAM` | `param=0x02, val=<colour>` | Set colour (packed RGB u16: RRRRRGGGGGGBBBBB) |
| `STREAM_ON` | `interval=500` | Stream power draw estimate every 500 ms |

---

## Integration with GCS Firmware

The RS-485 bus should run as its own **FreeRTOS task** (`rs485_task`) at the same priority as `cdc_task`. The task:

1. Polls each known peripheral with `PING` every 1000 ms and marks online/offline.
2. Forwards `SET_OUTPUT` / `SET_PARAM` commands received from the Pi (new CDC packet type `0x0C` — peripheral command passthrough).
3. Forwards `STATUS` and `STREAM_DATA` responses back to the Pi (new CDC packet type `0x0D` — peripheral data upstream).
4. Monitors `/INT` line — on assertion, immediately polls the asserting device for status.

### Suggested new CDC packet types

| Type | Direction | Name | Payload |
|------|-----------|------|---------|
| 0x0C | Pi → Pico | Peripheral command | `addr (u8)` + RS-485 CMD frame payload |
| 0x0D | Pico → Pi | Peripheral data | `addr (u8)` + RS-485 STATUS/STREAM payload |
| 0x0E | Pico → Pi | Peripheral online/offline | `addr (u8), online (u8)` |

---

## PCB Layout Notes

- Place the SP3485 as close to the 8-pin rear connector as possible — keep A/B traces short before the TVS diodes.
- Route A and B as a differential pair with matched length; keep away from the 5 V LED supply traces.
- Place 120 Ω termination resistor directly at the connector pad (not at the SP3485).
- Place 560 Ω bias resistors (A pull-up, B pull-down) near the SP3485 — these ensure a defined idle state when no peripheral is connected.
- The polyfuse on VBAT should be the first component after the connector pin, before any branching.
- Add a LED (with 1 kΩ resistor) on the DE line to indicate when the bus is transmitting — useful for debugging in the field.
