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
| ----- | -------- | ------- |
| 1 | GND | Common ground — connect shield here too |
| 2 | +5 V | Regulated 5 V for peripheral logic / MCU |
| 3 | RS-485 A | Differential data line + |
| 4 | spare | Reserved for future expansion |
| 5 | spare | Reserved for future expansion |
| 6 | /INT | Open-drain alert — peripheral pulls low to signal async event |
| 7 | RS-485 B | Differential data line − |
| 8 | VIN | Fused raw battery voltage for high-power loads |

> **Cable colour suggestion (for consistent field builds):**
> GND = Brown, +5 V = Red, VIN = White, A = Green, B = Blue, /INT = Orange.

---

## GCS-Side Hardware

### Components

| Component | Part | Package | Purpose |
|---|---|---|---|
| RS-485 transceiver | **SP3485EN** (3.3 V) | SOIC-8 | Converts UART1 to RS-485 differential |
| Bus termination | 120 Ω 0402, 1% | — | Across A/B at the PCB-side bus end |
| ESD / surge protection | **PRTR5V0U2X** or 2× **SMBJ6.5A** TVS | SOT-363 / SMB | On A and B before the connector |
| VBAT fuse | Polyfuse 1 A (e.g. MF-MSMF110) | 1812 | Inline with VBAT connector pin |
| Pull-up / pull-down | 560 Ω to 3.3 V on A, 560 Ω to GND on B | 0402 | Bias bus to idle mark when no driver active |

### SP3485 wiring (SOIC-8)

```
Pico GP8 (UART1 TX) ──── SP3485 pin 4 (DI)
Pico GP9 (UART1 RX) ──── SP3485 pin 1 (RO)
Pico GP2            ──┬─ SP3485 pin 3 (DE)   ← drive high to transmit
                      └─ SP3485 pin 2 (/RE)  ← tie to DE (both together)
Pico GP6 (/INT in)  ──── 8-pin connector pin 6 + 10 kΩ pull-up to 3.3 V
                         (open-drain; peripherals pull low to signal event)

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
| --------- | ----------- |
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
| ----- | ----------- | ------ | ------------- |
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
| --- | --- |
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
| ----- | ----- |
| **RP2040 (Pico)** | Same toolchain as GCS, cheap, plenty of I/O |
| **STM32F103C8T6** | Primary chip on the KL-GCS-MODBUS03 PCB; LQFP-48, USB, lots of timers/ADC |
| **ATmega328P** | Arduino ecosystem, easy prototyping |
| **CH32V003** | Extremely cheap (< €0.15), RISC-V, SOIC-8 |

Each peripheral board needs a **SP3485** (or MAX485 if running at 5 V logic) plus the protection components listed above.

---

## Example Peripheral: Searchlight Module

**Purpose:** High-power LED spotlight with PWM brightness control and thermal monitoring.

**Hardware:**

- MCU: STM32F103C8T6 (or RP2040 / ESP32)
- SP3485 for RS-485
- MOSFET (e.g. IRLZ44N) to switch the LED load from VBAT
- NTC thermistor on the LED heatsink
- Onboard fuse for VBAT

**Protocol usage:**

| CMD | Payload | Effect |
| ----- | --------- | -------- |
| `SET_OUTPUT` | `ch=0, val=0–255` | PWM brightness (0 = off, 255 = full) |
| `GET_STATUS` | — | Returns: `brightness (u8), temp_C (i8), fault_flags (u8)` |
| `SET_PARAM` | `param=0x01, val=1` | Enable thermal shutoff at 80 °C |

**Fault flags byte:**

| Bit | Meaning |
| ----- | --------- |
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
| ----- | --------- | -------- |
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
| ----- | --------- | -------- |
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
| ----- | --------- | -------- |
| `SET_OUTPUT` | `ch=0, val=0–255` | Global brightness |
| `SET_PARAM` | `param=0x01, val=<mode>` | Set lighting mode (0=solid, 1=flash, 2=breathe) |
| `SET_PARAM` | `param=0x02, val=<colour>` | Set colour (packed RGB u16: RRRRRGGGGGGBBBBB) |
| `STREAM_ON` | `interval=500` | Stream power draw estimate every 500 ms |

---

## Integration with GCS Firmware

### 1. New pin definitions — `src/pins.h`

Add the following block:

```c
/* ------------------------------------------------------------------ */
/* UART1 — RS-485 peripheral bus (GP8/GP9 + GP2 DE/RE)                 */
/* ------------------------------------------------------------------ */
#define PIN_RS485_TX        8   /* GP8  — UART1 TX → SP3485 DI          */
#define PIN_RS485_RX        9   /* GP9  — UART1 RX ← SP3485 RO          */
#define PIN_RS485_DE        2   /* GP2  — SP3485 DE + /RE (active high)  */
#define PIN_RS485_INT       6   /* GP6  — /INT input (active low, pulled */
                                /*         high; peripheral open-drains) */
#define RS485_UART_INST     uart1
#define RS485_BAUD          115200
```

---

### 2. New CDC packet types — `src/protocol.h`

Append to the existing `PROTO_TYPE_*` defines:

```c
#define PROTO_TYPE_PERIPH_CMD   0x0C  /* Pi→Pico:  command for a bus peripheral  */
#define PROTO_TYPE_PERIPH_DATA  0x0D  /* Pico→Pi:  data from a bus peripheral    */
#define PROTO_TYPE_PERIPH_STATE 0x0E  /* Pico→Pi:  peripheral online/offline     */
```

Add the corresponding payload structs:

```c
/* Type 0x0C — Peripheral command (Pi → Pico → RS-485 bus) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* target peripheral address (0x01–0xFE) */
    uint8_t cmd;        /* RS-485 bus CMD byte                   */
    uint8_t len;        /* payload byte count (0–255)            */
    uint8_t payload[];  /* flexible array — len bytes            */
} periph_cmd_t;

/* Type 0x0D — Peripheral data (RS-485 bus → Pico → Pi) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* source peripheral address             */
    uint8_t cmd;        /* RS-485 CMD byte of the response       */
    uint8_t len;        /* payload byte count                    */
    uint8_t payload[];  /* response payload — len bytes          */
} periph_data_t;

/* Type 0x0E — Peripheral presence notification */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* peripheral address                    */
    uint8_t online;     /* 1 = online, 0 = offline               */
} periph_state_t;
```

---

### 3. Handle the new CDC packet type — `src/protocol.c`

In `proto_handle_rx()`, add a case inside the checksum-verified `switch (s_rx_type)` block (after `PROTO_TYPE_WARNING`):

```c
case PROTO_TYPE_PERIPH_CMD:
    /* Forward to rs485_task via its command queue */
    if (s_rx_len >= 3) {
        rs485_forward_cmd(s_rx_buf, s_rx_len);
    }
    break;
```

`rs485_forward_cmd()` is declared in `rs485.h` and posts to the RS-485 task queue.

Also add the include at the top of `protocol.c`:

```c
#include "rs485.h"
```

---

### 4. New RS-485 driver — `src/rs485.h` / `src/rs485.c`

Create these two new files.

**`src/rs485.h`**

```c
#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* RS-485 bus frame SOF — distinct from CDC SOF (0xAA) */
#define RS485_SOF   0xAB

/* RS-485 CMD bytes */
#define RS485_CMD_PING          0x01
#define RS485_CMD_PONG          0x02
#define RS485_CMD_SET_OUTPUT    0x10
#define RS485_CMD_GET_STATUS    0x11
#define RS485_CMD_STATUS        0x12
#define RS485_CMD_SET_PARAM     0x20
#define RS485_CMD_GET_PARAM     0x21
#define RS485_CMD_PARAM_VAL     0x22
#define RS485_CMD_STREAM_ON     0x30
#define RS485_CMD_STREAM_OFF    0x31
#define RS485_CMD_STREAM_DATA   0x32
#define RS485_CMD_ERROR         0xF0
#define RS485_CMD_SYNC          0xFF

#define RS485_ADDR_BROADCAST    0xFF
#define RS485_TIMEOUT_MS        50
#define RS485_PING_INTERVAL_MS  1000
#define RS485_MAX_PERIPHERALS   8

void rs485_init(void);
void rs485_task(void *arg);
void rs485_forward_cmd(const uint8_t *payload, uint16_t len);

#endif /* RS485_H */
```

**`src/rs485.c` — structure outline**

```c
#include "rs485.h"
#include "pins.h"
#include "protocol.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>

/* ---- internal command queue (from CDC → RS-485 task) ---- */
#define RS485_CMD_QUEUE_DEPTH  8
#define RS485_CMD_BUF_SIZE     64

typedef struct {
    uint8_t buf[RS485_CMD_BUF_SIZE];
    uint8_t len;
} rs485_cmd_item_t;

static QueueHandle_t s_cmd_queue;

/* ---- peripheral registry ---- */
static uint8_t s_known_addrs[RS485_MAX_PERIPHERALS] = {
    0x01, 0x02, 0x03, 0x04,   /* extend as you add devices */
    0x00, 0x00, 0x00, 0x00
};
static bool s_online[RS485_MAX_PERIPHERALS];

/* ---- CRC-8/MAXIM ---- */
static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

/* ---- transmit one RS-485 frame ---- */
static void rs485_send(uint8_t addr, uint8_t cmd,
                       const uint8_t *payload, uint8_t plen)
{
    uint8_t frame[4 + 255 + 1];
    frame[0] = RS485_SOF;
    frame[1] = addr;
    frame[2] = cmd;
    frame[3] = plen;
    if (plen) memcpy(&frame[4], payload, plen);
    frame[4 + plen] = crc8(&frame[1], 3 + plen);

    gpio_put(PIN_RS485_DE, 1);          /* enable driver  */
    uart_write_blocking(RS485_UART_INST, frame, 5 + plen);
    uart_tx_wait_blocking(RS485_UART_INST);
    gpio_put(PIN_RS485_DE, 0);          /* back to receive */
}

/* ---- receive one RS-485 frame (blocking with timeout) ---- */
/* Returns payload length on success, -1 on timeout/CRC error */
static int rs485_recv(uint8_t *addr_out, uint8_t *cmd_out,
                      uint8_t *buf, uint8_t buf_size)
{
    /* ... byte-by-byte state machine with xTaskGetTickCount() timeout ... */
    /* Same pattern as proto_handle_rx() in protocol.c                      */
}

/* ---- report peripheral state change to Pi over CDC ---- */
static void notify_state(uint8_t addr, bool online)
{
    periph_state_t pkt = { .addr = addr, .online = online ? 1 : 0 };
    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_PERIPH_STATE,
                              (const uint8_t *)&pkt, sizeof(pkt));
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

/* ---- forward RS-485 response to Pi over CDC ---- */
static void forward_to_pi(uint8_t addr, uint8_t cmd,
                           const uint8_t *payload, uint8_t plen)
{
    uint8_t cdc_payload[2 + 255];
    cdc_payload[0] = addr;
    cdc_payload[1] = cmd;
    if (plen) memcpy(&cdc_payload[2], payload, plen);

    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_PERIPH_DATA,
                              cdc_payload, 2 + plen);
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

/* ------------------------------------------------------------------ */
void rs485_init(void)
{
    s_cmd_queue = xQueueCreate(RS485_CMD_QUEUE_DEPTH, sizeof(rs485_cmd_item_t));

    uart_init(RS485_UART_INST, RS485_BAUD);
    gpio_set_function(PIN_RS485_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_RS485_RX, GPIO_FUNC_UART);

    gpio_init(PIN_RS485_DE);
    gpio_set_dir(PIN_RS485_DE, GPIO_OUT);
    gpio_put(PIN_RS485_DE, 0);   /* receive mode by default */

    gpio_init(PIN_RS485_INT);
    gpio_set_dir(PIN_RS485_INT, GPIO_IN);
    gpio_pull_up(PIN_RS485_INT); /* open-drain line — pulled high at rest */
}

void rs485_forward_cmd(const uint8_t *payload, uint16_t len)
{
    if (!s_cmd_queue || len > RS485_CMD_BUF_SIZE) return;
    rs485_cmd_item_t item;
    memcpy(item.buf, payload, len);
    item.len = (uint8_t)len;
    xQueueSend(s_cmd_queue, &item, 0);
}

/* ------------------------------------------------------------------ */
void rs485_task(void *arg)
{
    (void)arg;
    TickType_t last_ping = xTaskGetTickCount();
    uint8_t    resp_buf[255];
    uint8_t    resp_addr, resp_cmd;

    for (;;) {
        /* 1. Forward any commands queued from the Pi */
        rs485_cmd_item_t cmd;
        while (xQueueReceive(s_cmd_queue, &cmd, 0) == pdTRUE) {
            if (cmd.len >= 3) {
                rs485_send(cmd.buf[0], cmd.buf[1],
                           &cmd.buf[3], cmd.buf[2]);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                if (r >= 0)
                    forward_to_pi(resp_addr, resp_cmd, resp_buf, (uint8_t)r);
            }
        }

        /* 2. Check /INT line — any peripheral asserting an async event */
        if (!gpio_get(PIN_RS485_INT)) {
            /* Line is low — poll all known peripherals for status */
            for (int i = 0; i < RS485_MAX_PERIPHERALS; i++) {
                if (!s_known_addrs[i] || !s_online[i]) continue;
                rs485_send(s_known_addrs[i], RS485_CMD_GET_STATUS, NULL, 0);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                if (r >= 0)
                    forward_to_pi(resp_addr, resp_cmd, resp_buf, (uint8_t)r);
            }
        }

        /* 3. Periodic PING sweep */
        if ((xTaskGetTickCount() - last_ping) >= pdMS_TO_TICKS(RS485_PING_INTERVAL_MS)) {
            last_ping = xTaskGetTickCount();
            for (int i = 0; i < RS485_MAX_PERIPHERALS; i++) {
                if (!s_known_addrs[i]) continue;
                rs485_send(s_known_addrs[i], RS485_CMD_PING, NULL, 0);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                bool now_online = (r >= 0 && resp_cmd == RS485_CMD_PONG);
                if (now_online != s_online[i]) {
                    s_online[i] = now_online;
                    notify_state(s_known_addrs[i], now_online);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

### 5. Register the task — `src/main.c`

In `main()` after the existing task creations, add:

```c
#include "rs485.h"

/* ... existing task creates ... */

rs485_init();
xTaskCreate(rs485_task, "rs485", 512, NULL, 2, NULL);
```

Priority 2 — same as `cdc_task`, below `usb_device_task` (4).

---

### 6. Summary of all file changes

| File | Change |
|------|--------|
| `src/pins.h` | Add `PIN_RS485_TX`, `PIN_RS485_RX`, `PIN_RS485_DE`, `RS485_UART_INST`, `RS485_BAUD` |
| `src/protocol.h` | Add `PROTO_TYPE_PERIPH_CMD/DATA/STATE` and three new packet structs |
| `src/protocol.c` | Add `#include "rs485.h"` and `case PROTO_TYPE_PERIPH_CMD` in `proto_handle_rx()` |
| `src/rs485.h` | New file — RS-485 constants and public API |
| `src/rs485.c` | New file — UART1 init, DE/RE control, frame TX/RX, FreeRTOS task |
| `src/main.c` | Call `rs485_init()` and `xTaskCreate(rs485_task, ...)` |
| `CMakeLists.txt` | Add `rs485.c` to the target sources |

---

## Peripheral Status Screens

The GCS ST7735 display (128×128 px) can show peripheral bus status. Two new screen modes are added: a **peripheral overview** listing all known devices and their online/offline state, and a **peripheral detail** view showing live data from a single selected device. The Pi can switch to these screens via the existing CDC screen-mode command (type `0x04`).

### Screen modes

| Mode ID | Name | Description |
|---------|------|-------------|
| `5` | `SCREEN_MODE_PERIPH` | Overview — list of all peripherals with online/offline status |
| `6` | `SCREEN_MODE_PERIPH_DETAIL` | Detail — live telemetry from one selected peripheral |

### Peripheral overview screen (mode 5)

```
┌────────────────────────────┐
│  ▓▓ PERIPHERALS ▓▓  (navy)│  ← header, teal accent line
├────────────────────────────┤
│  0x01 Searchlight  [ONLINE]│  ← green pill if online
│  0x02 Radar        [OFFLN] │  ← red pill if offline
│  0x03 Pan-Tilt     [ONLINE]│
│  0x04 Light Bar    [OFFLN] │
│                            │
│                            │
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │
│  4 devices  2 online (teal)│  ← footer summary
└────────────────────────────┘
```

Each peripheral row shows:
- Address (hex, 4 chars)
- Short name (up to 10 chars, looked up from a name table)
- Status pill: green `ON` or red `OFF`

The screen refreshes when `periph_state_t` packets (type `0x0E`) arrive over CDC, updating the cached online/offline flags. Up to 8 peripherals fit on the 128 px display (8 rows × 10 px row height + header + footer).

### Peripheral detail screen (mode 6)

The detail screen shows live data from one peripheral. The Pi selects which peripheral to display via a new CDC packet type (see below). The layout adapts based on the peripheral type.

**Searchlight detail example:**

```
┌────────────────────────────┐
│  ▓▓ SEARCHLIGHT ▓▓ (navy) │  ← header with device name
├────────────────────────────┤
│  Brightness       ███████  │  ← bar graph, 0–255
│                    78%     │
│                            │
│  Temp              42°C    │  ← colour-coded (green/amber/red)
│                            │
│  Faults           NONE     │  ← or list active fault flags
│                            │
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓    │
│  addr 0x01    ONLINE       │  ← footer with address + status
└────────────────────────────┘
```

**Radar detail example:**

```
┌────────────────────────────┐
│  ▓▓ RADAR NODE ▓▓  (navy) │
├────────────────────────────┤
│  Distance                  │
│        1234 mm             │  ← size 2 font
│                            │
│  Signal       ████░░░░    │  ← strength bar
│  Status       OK          │
│                            │
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │
│  addr 0x02    ONLINE      │
└────────────────────────────┘
```

### Peripheral name table

A static lookup table maps addresses to short display names. Default entries match the address space defined earlier in this document:

```c
static const char *periph_name(uint8_t addr) {
    switch (addr) {
        case 0x01: return "Searchlight";
        case 0x02: return "Radar";
        case 0x03: return "Pan-Tilt";
        case 0x04: return "Light Bar";
        default:   return "Device";
    }
}
```

### Navigating between screens

The Pi controls which screen is displayed via the existing screen-mode CDC packet (`PROTO_TYPE_SCREEN`, type `0x04`). To show the detail view for a specific peripheral, a new CDC packet type is added:

| Type | Direction | Name | Payload |
|------|-----------|------|---------|
| `0x0F` | Pi → Pico | `PROTO_TYPE_PERIPH_SCREEN` | `addr (u8)` — peripheral to show in detail view |

When the Pico receives this packet, it caches the target address and switches to `SCREEN_MODE_PERIPH_DETAIL`. Subsequent `STREAM_DATA` or `STATUS` frames from that address are rendered on the detail screen.

The overview screen can also be triggered by a physical switch (e.g. SW3 position) via the auto-mode state machine, allowing the operator to check peripheral status without Pi intervention.

---

## PCB Layout Notes

- Place the SP3485 as close to the 8-pin rear connector as possible — keep A/B traces short before the TVS diodes.
- Route A and B as a differential pair with matched length; keep away from the 5 V LED supply traces.
- Place 120 Ω termination resistor directly at the connector pad (not at the SP3485).
- Place 560 Ω bias resistors (A to **3.3 V**, B to GND) near the SP3485 — these ensure a defined idle state when no peripheral is connected.
- The polyfuse on VBAT should be the first component after the connector pin, before any branching.
- Add a LED (with 1 kΩ resistor) on the DE line to indicate when the bus is transmitting — useful for debugging in the field.
