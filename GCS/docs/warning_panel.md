# Warning Panel — SK6812 LEDs 61–70

## Overview

The bottom panel of the GCS controller contains 9 warning icons. Each icon is
backlit by a dedicated LED from the SK6812 strip. The Raspberry Pi sends warning
severity states to the Pico over USB CDC using `PROTO_TYPE_WARNING (0x0A)`. The
Pico handles all color selection and blink animation locally, so the Pi does not
need to manage LED timing.

---

## Icon → LED Mapping

| Icon index | Constant                | LED index | Description              |
|-----------|-------------------------|-----------|--------------------------|
| 0         | `WARN_ICON_TEMP`        | 61        | Temperature warning      |
| 1         | `WARN_ICON_SIGNAL`      | 62        | Signal strength warning  |
| 2         | `WARN_ICON_AIRCRAFT`    | 63        | Aircraft warning         |
| 3         | `WARN_ICON_DRONE_LINK`  | 64        | Drone link status        |
| 4         | `WARN_ICON_MAIN`        | 65 + 66   | Main / big warning (2 LEDs, 1 protocol byte) |
| 5         | `WARN_ICON_GPS_GCS`     | 67        | GPS lock (GCS)           |
| 6         | `WARN_ICON_NETWORK_GCS` | 68        | Network connection (GCS) |
| 7         | `WARN_ICON_LOCKED`      | 69        | Locked state             |
| 8         | `WARN_ICON_DRONE_STATUS`| 70        | Drone status             |

---

## Severity Levels

| Value | Constant        | Meaning                        |
|-------|-----------------|--------------------------------|
| 0     | `WARN_OK`       | Normal operation               |
| 1     | `WARN_WARNING`  | Caution — attention needed     |
| 2     | `WARN_CRITICAL` | Critical fault — act now       |

---

## Color & Behavior Table

### Normal icons (0 Temp, 1 Signal, 2 Aircraft, 3 Drone link, 4 Main warning, 8 Drone status)

| Severity        | Color  | Behavior             |
|-----------------|--------|----------------------|
| `WARN_CRITICAL` | Red    | Blink fast (250 ms)  |
| `WARN_WARNING`  | Amber  | Solid                |
| `WARN_OK`       | Green  | Solid                |

### GPS / Network icons (5 GPS lock GCS, 6 Network GCS)

| Severity        | Color | Behavior             |
|-----------------|-------|----------------------|
| `WARN_CRITICAL` | Red   | Solid                |
| `WARN_WARNING`  | Blue  | Blink slow (500 ms)  |
| `WARN_OK`       | Blue  | Solid                |

### Locked icon (7 Locked state)

| Severity        | Color | Behavior |
|-----------------|-------|----------|
| `WARN_CRITICAL` | Red   | Solid    |
| `WARN_WARNING`  | Amber | Solid    |
| `WARN_OK`       | Green | Solid    |

---

## Protocol

### Packet format

```
[0xAA] [0x0A] [0x09] [sev_0] [sev_1] ... [sev_8] [checksum]
  SOF   type   len    icon 0   icon 1       icon 8
```

- **Type**: `0x0A` (`PROTO_TYPE_WARNING`)
- **Length**: `9` (one byte per icon; `WARN_ICON_MAIN` drives two physical LEDs internally)
- **Payload**: 9 bytes, each `WARN_OK (0)`, `WARN_WARNING (1)`, or `WARN_CRITICAL (2)`
- **Checksum**: XOR of type, length, and all payload bytes

### C struct (Pico side, `protocol.h`)

```c
typedef struct __attribute__((packed)) {
    uint8_t severity[WARN_ICON_COUNT];  /* index matches WARN_ICON_* constants */
} warning_cmd_t;
```

---

## Pi-side Example (Python)

```python
import struct

PROTO_SOF       = 0xAA
PROTO_TYPE_WARNING = 0x0A

WARN_OK       = 0
WARN_WARNING  = 1
WARN_CRITICAL = 2

def build_warning_packet(severities: list[int]) -> bytes:
    """severities: list of 9 values (WARN_OK / WARN_WARNING / WARN_CRITICAL)."""
    assert len(severities) == 9
    payload = bytes(severities)
    checksum = PROTO_TYPE_WARNING ^ len(payload)
    for b in payload:
        checksum ^= b
    return bytes([PROTO_SOF, PROTO_TYPE_WARNING, len(payload)]) + payload + bytes([checksum])

# Example: GPS warning, all others OK
severities = [WARN_OK] * 9
severities[5] = WARN_WARNING   # WARN_ICON_GPS_GCS  → blue blink slow
severities[2] = WARN_CRITICAL  # WARN_ICON_AIRCRAFT → red blink fast

packet = build_warning_packet(severities)
serial_port.write(packet)
```

---

## Firmware Notes

- The `sk6812_task` wakes every **50 ms** (or immediately on a new LED/warning
  packet) to recompute blink state and push pixels via DMA.
- Warning LEDs are always appended to any Pi-controlled pixel data. If the Pi sends
  fewer than 71 pixels, the gap is zero-filled and the strip is extended to LED 70.
- Brightness set via `PROTO_TYPE_BRIGHTNESS (target=0)` applies uniformly to both
  the Pi-controlled pixels and the warning panel.
- Default severity on power-up is `WARN_OK` (green) for all icons.
