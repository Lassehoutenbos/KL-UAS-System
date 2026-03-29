# GCS Pico PCB Design Reference

Ground Control Station co-processor board based on the **Raspberry Pi Pico (RP2040)**.
The Pico communicates with the Raspberry Pi host over **USB CDC** (TinyUSB).

---

## Components

| Component | Part | Package | Interface | Supply |
| --- | --- | --- | --- | --- |
| Microcontroller | Raspberry Pi Pico (RP2040) | 51-pin castellated edge (21 × 51 mm) | — | 3.3 V (onboard LDO) |
| LED strip | SK6812 RGBW, up to 70 LEDs (fw max 128) | Strip connector | PIO0 SM0 / 800 kbps NRZ | 5 V |
| Button LEDs | WS2811 RGB, 2 buttons (LED5, LED6) | Integrated in button body | PIO0 SM1 / 400 kbps NRZ | 5 V |
| GPIO expander | MCP23017-E/SS | SSOP-28 | I2C0 400 kHz, addr 0x20 | 3.3 V |
| Ambient light sensor | VEML7700 | OPLGA-6 (2 × 2 mm) | I2C0 400 kHz, addr 0x10 | 3.3 V (2.5–3.6 V) |
| ADC | MCP3208-CI/SL (8-ch, 12-bit) | SOIC-16 | SPI1 1 MHz, CS = GP17 | 3.3 V, Vref = 3.3 V |
| Display | ST7735S TFT 1.44" 128×128 GreenTab | 8-pin module header | SPI1 4 MHz, CS = GP20 | 3.3 V |
| Level shifter A | 74AHCT125 (or SN74LVC2T45) | SO-14 | GP0 → SK6812 DIN | IN: 3.3 V / OUT: 5 V |
| Level shifter B | 74AHCT125 (or SN74LVC2T45) | SO-14 | GP1 → WS2811 DIN | IN: 3.3 V / OUT: 5 V |

---

## Pin Assignment

### Pico GPIO master table

| GPIO | Pico pin # | Signal | Connected to |
| --- | --- | --- | --- |
| GP0 | 1 | SK6812_DATA | Level shifter A pin 2 (1A input) |
| GP1 | 2 | WS2811_DATA | Level shifter B pin 2 (1A input) |
| GP4 | 6 | I2C0 SDA | MCP23017 pin 13 + VEML7700 pin SDA + 4.7 kΩ pull-up to 3.3 V |
| GP5 | 7 | I2C0 SCL | MCP23017 pin 12 + VEML7700 pin SCL + 4.7 kΩ pull-up to 3.3 V |
| GP16 | 21 | SPI1 MISO (RX) | MCP3208 pin 12 (DOUT) |
| GP17 | 22 | MCP3208 CS (active low) | MCP3208 pin 10 (/CS-/SHDN) |
| GP18 | 24 | SPI1 SCK | MCP3208 pin 13 (CLK) + ST7735 SCK |
| GP19 | 25 | SPI1 MOSI (TX) | MCP3208 pin 11 (DIN) + ST7735 SDA/MOSI |
| GP20 | 26 | ST7735 CS (active low) | ST7735 module pin CS |
| GP21 | 27 | ST7735 DC | ST7735 module pin DC/RS |
| GP22 | 29 | ST7735 RST (active low) | ST7735 module pin RES (10 kΩ pull-up to 3.3 V) |
| VSYS | 39 | 5 V input | 5 V rail via Schottky diode (BAT54 or similar) |
| 3V3 OUT | 36 | 3.3 V output | 3.3 V rail — powers MCP23017, MCP3208, level shifter VCC_A |
| GND | 3/8/13/18/23/28/33/38 | GND | Common GND plane |

---

### PIO — LED data lines

| GPIO | Pico pin # | PIO | Signal | To level shifter |
| --- | --- | --- | --- | --- |
| GP0 | 1 | PIO0 SM0, 800 kbps NRZ | SK6812_DATA | Level shifter A — 74AHCT125 pin 2 (1A) |
| GP1 | 2 | PIO0 SM1, 400 kbps NRZ | WS2811_DATA | Level shifter B — 74AHCT125 pin 2 (1A) |

---

### I2C0 — MCP23017 (SSOP-28, address 0x20)

| GPIO | Pico pin # | Signal | MCP23017 pin |
| --- | --- | --- | --- |
| GP4 | 6 | SDA | Pin 13 (SDA) |
| GP5 | 7 | SCL | Pin 12 (SCL) |

**MCP23017 full pin wiring:**

| IC pin | Name | Connect to |
| --- | --- | --- |
| 1 | GPB0 | PB0 — SW1, contact 1 → GND via switch |
| 2 | GPB1 | PB1 — SW1, contact 2 → GND via switch |
| 3 | GPB2 | PB2 — SW1, contact 3 → GND via switch |
| 4 | GPB3 | PB3 — SW2, contact 1 → GND via switch |
| 5 | GPB4 | PB4 — SW2, contact 2 → GND via switch |
| 6 | GPB5 | PB5 — SW2, contact 3 → GND via switch |
| 7 | GPB6 | PB6 — SW3, contact 1 → GND via switch |
| 8 | GPB7 | PB7 — SW3, contact 2 → GND via switch |
| 9 | VDD | 3.3 V + 100 nF + 1 µF to GND |
| 10 | VSS | GND |
| 11 | NC | Leave unconnected |
| 12 | SCL | GP5 (Pico pin 7) + 4.7 kΩ to 3.3 V |
| 13 | SDA | GP4 (Pico pin 6) + 4.7 kΩ to 3.3 V |
| 14 | NC | Leave unconnected |
| 15 | A0 | GND (address bit 0 = 0) |
| 16 | A1 | GND (address bit 1 = 0) |
| 17 | A2 | GND (address bit 2 = 0) → I2C address = 0x20 |
| 18 | /RESET | 3.3 V via 10 kΩ pull-up (tie high; add cap to GND for power-on reset) |
| 19 | INTB | Leave unconnected (not used by firmware) |
| 20 | INTA | Leave unconnected (not used by firmware) |
| 21 | GPA0 | PA0 — 120 Ω → LED2 anode; LED2 cathode → GND |
| 22 | GPA1 | PA1 — 120 Ω → LED3 anode; LED3 cathode → GND |
| 23 | GPA2 | PA2 — 120 Ω → LED4 anode; LED4 cathode → GND |
| 24 | GPA3 | Leave floating (unused, PA3 pull-up enabled in firmware) |
| 25 | GPA4 | Leave floating (unused, PA4 pull-up enabled in firmware) |
| 26 | GPA5 | PA5 — key switch → GND (active low = locked) |
| 27 | GPA6 | PA6 — SW3, contact 3 → GND via switch |
| 28 | GPA7 | PA7 — SW3, contact 4 → GND via switch (spare) |

**I2C pull-ups:** 4.7 kΩ from SDA and SCL to 3.3 V. Place resistors within 10 mm of IC pins 12–13. Do not rely on Pico internal pull-ups for 400 kHz fast-mode.

**Direction registers (firmware):**

- `IODIRA = 0b11111000` — PA0–PA2 output, PA3–PA7 input
- `IODIRB = 0xFF` — all inputs
- `GPPUA = 0b11111000` — pull-ups on PA3–PA7
- `GPPUB = 0xFF` — pull-ups on all PB

**Indicator LED resistor** (green LED, Vf ≈ 2.1 V, 10 mA target):
```
R = (3.3 V − 2.1 V) / 10 mA = 120 Ω  →  use 120 Ω or 150 Ω (0402/0603)
```

### I2C0 — VEML7700 ambient light sensor (OPLGA-6, 2 × 2 mm)

Shares the same I2C0 bus as the MCP23017. No address configuration — I2C address is fixed at **0x10**.

**VEML7700 full pin wiring:**

| IC pin | Name | Connect to |
| --- | --- | --- |
| 1 | VDD | 3.3 V + 100 nF to GND (within 1 mm — small package) |
| 2 | SDA | GP4 (Pico pin 6) — shared I2C0 SDA bus wire |
| 3 | SCL | GP5 (Pico pin 7) — shared I2C0 SCL bus wire |
| 4 | INT | Optional: GPIO input on Pico (e.g. GP26) for threshold interrupt; or leave unconnected. Open-drain, active low — add 10 kΩ pull-up to 3.3 V if used. |
| 5 | NC | Leave unconnected |
| 6 | GND | GND |

**Notes:**

- Pull-ups on SDA/SCL are shared with MCP23017 — one pair of 4.7 kΩ resistors on the bus is sufficient for both devices.
- The VEML7700 has a photodiode window on the top face; position it with an unobstructed view of the ambient light source (panel front or display area). Do not cover with silkscreen or conformal coating.
- The OPLGA package has an exposed pad on the underside — tie it to GND.
- Measurement range: 0–120,000 lux (gain/integration time configurable over I2C). Typical use: auto-adjust SK6812 / ST7735 brightness via `PROTO_TYPE_BRIGHTNESS` command.

---

### SPI1 — MCP3208 ADC (SOIC-16)

**Chip select:** GP17 (Pico pin 22), software-controlled, active low, 1 MHz clock.

**MCP3208 full pin wiring:**

| IC pin | Name | Connect to |
| --- | --- | --- |
| 1 | CH0 | Battery voltage input (via 4:1 divider — see below) |
| 2 | CH1 | External voltage input (via 4:1 divider — see below) |
| 3 | CH2 | Spare sensor input (direct 0–3.3 V) |
| 4 | CH3 | Spare sensor input |
| 5 | CH4 | Spare sensor input |
| 6 | CH5 | Spare sensor input |
| 7 | CH6 | Reserved — leave unconnected |
| 8 | CH7 | Reserved — leave unconnected |
| 9 | DGND | GND |
| 10 | /CS-/SHDN | GP17 (Pico pin 22) |
| 11 | DIN | GP19 / SPI1 MOSI (Pico pin 25) |
| 12 | DOUT | GP16 / SPI1 MISO (Pico pin 21) |
| 13 | CLK | GP18 / SPI1 SCK (Pico pin 24) |
| 14 | AGND | GND (star-point, isolated from 5 V LED return where possible) |
| 15 | VREF | 3.3 V — tied to VDD pin 16 (single-supply, Vref = VDD = 3.3 V) |
| 16 | VDD | 3.3 V + 100 nF to AGND (place cap within 2 mm of pin) |

**Reference voltage:** VREF (pin 15) = 3.3 V = VDD. Full-scale input = 3.3 V. Resolution = 3.3 V / 4096 ≈ 0.806 mV per count.

**Voltage divider for CH0 (battery) and CH1 (external):**
```
Vin (up to 13.2 V)
    │
   R1 = 30 kΩ (0402, 1%)
    │
    ├──── CH0 or CH1 (MCP3208 input)
    │     └── 100 nF to GND  (noise filter)
   R2 = 10 kΩ (0402, 1%)
    │
   GND
```
Ratio = 4:1. Max input = 3.3 V × 4 = **13.2 V** (covers 3S LiPo max 12.6 V).
Firmware conversion: `Vbat = (raw / 4095.0) × 3.3 × 4.0`

**Spare channels CH2–CH5:** connect directly to 0–3.3 V analog signals. Add 100 nF to GND at each input pin.

---

### SPI1 — ST7735S TFT 1.44" 128×128 (module)

**Chip select:** GP20 (Pico pin 26), software-controlled, active low, 4 MHz clock.

The ST7735 module shares SCK and MOSI with the MCP3208. MISO is not connected to the display (display is write-only). Bus access is protected by `g_spi1_mutex`.

| Module pin | Signal | Connected to |
| --- | --- | --- |
| GND | Power GND | GND |
| VCC | 3.3 V supply | 3.3 V + 100 nF local decoupling |
| SCL / SCK | SPI clock | GP18 (Pico pin 24) / SPI1 SCK |
| SDA / MOSI | SPI data | GP19 (Pico pin 25) / SPI1 TX |
| RES | Hardware reset (active low) | GP22 (Pico pin 29); 10 kΩ pull-up to 3.3 V |
| DC / RS | Data=1 / Command=0 | GP21 (Pico pin 27) |
| CS | Chip select (active low) | GP20 (Pico pin 26) |
| BL | Backlight LED anode | 3.3 V via 10 Ω resistor (or GP26 for PWM dimming) |

**Note:** if ST7735 displays a white screen, reduce `ST7735_SPI_BAUD` from 4 MHz to 2 MHz in [pins.h](../src/pins.h#L84).

---

### 74AHCT125 level shifter (SO-14) — two ICs, one per LED chain

Both ICs are wired identically. Only channel 1 of each IC is used (pins 1–3).

| IC pin | Name | Connect to — Level shifter A (SK6812) | Connect to — Level shifter B (WS2811) |
| --- | --- | --- | --- |
| 1 | /OE1 | GND (always enabled) | GND (always enabled) |
| 2 | 1A (input) | GP0 (Pico pin 1) | GP1 (Pico pin 2) |
| 3 | 1Y (output) | 330 Ω → SK6812 strip DIN | 330 Ω → WS2811 button DIN |
| 4 | /OE2 | VCC_HV (disable unused ch) | VCC_HV (disable unused ch) |
| 5 | 2A | Leave unconnected | Leave unconnected |
| 6 | 2Y | Leave unconnected | Leave unconnected |
| 7 | GND | GND | GND |
| 8 | 3Y | Leave unconnected | Leave unconnected |
| 9 | 3A | Leave unconnected | Leave unconnected |
| 10 | /OE3 | VCC_HV (disable) | VCC_HV (disable) |
| 11 | 4Y | Leave unconnected | Leave unconnected |
| 12 | 4A | Leave unconnected | Leave unconnected |
| 13 | /OE4 | VCC_HV (disable) | VCC_HV (disable) |
| 14 | VCC | 5 V LED rail (VCC_HV) + 100 nF to GND | 5 V LED rail + 100 nF to GND |

The 74AHCT125 input threshold is ~1.5 V; 3.3 V logic input is well within spec. VCC pin 14 must be tied to the **5 V rail** so the output swings rail-to-rail at 5 V.

**Series resistors:** place 330–470 Ω 0402 resistors between each 1Y output (pin 3) and the LED DIN connector pad to suppress ringing.

---

## Physical switch / button summary

| Group | Type | Count | MCP23017 pins | Notes |
| --- | --- | --- | --- | --- |
| SW1 | Guard-covered toggle (on/off) | 3 | PB0, PB1, PB2 (pins 1–3) | Red flip-up safety covers |
| SW2 | Momentary pushbutton + green LED | 3 | PB3, PB4, PB5 (pins 4–6) | LED outputs: PA0, PA1, PA2 (pins 21–23) |
| SW3 | Small toggle (on/off) | 2 used, 2 spare | PB6, PB7 (pins 7–8); PA6, PA7 (pins 27–28) | |
| KEY | Key switch (2-state) | 1 | PA5 (pin 26) | Active low = locked |
| WS2811 | Momentary RGB button | 2 | — (WS2811 chain, GP1) | SK6812 IDs 0 and 1; events over CDC |

All switch inputs connect the MCP23017 input pin to **GND** when activated. The MCP23017 internal pull-ups hold the lines high at rest.

---

## SK6812 LED chain layout

Firmware buffer: indices 0–127 (`SK6812_MAX_PIXELS = 128`).

| Index range | Function |
| --- | --- |
| 0–60 | General-purpose strip LEDs |
| 61–69 | Warning panel icons (`WARN_PANEL_LED_BASE = 61`, 9 icons) |
| 70–127 | Reserved / spare |

Warning icon map (index = 61 + `WARN_ICON_*`):

| Index | Constant | Display purpose |
| --- | --- | --- |
| 61 | `WARN_ICON_TEMP` | Temperature |
| 62 | `WARN_ICON_SIGNAL` | Signal strength |
| 63 | `WARN_ICON_AIRCRAFT` | Aircraft status |
| 64 | `WARN_ICON_DRONE_LINK` | Drone link |
| 65 | `WARN_ICON_MAIN` | Main warning |
| 66 | `WARN_ICON_GPS_GCS` | GPS lock (GCS) |
| 67 | `WARN_ICON_NETWORK_GCS` | Network connection |
| 68 | `WARN_ICON_LOCKED` | Locked state |
| 69 | `WARN_ICON_DRONE_STATUS` | Drone status |

---

## Level Shifter Design (CRITICAL)

SK6812 and WS2811 require a logic-high ≥ 3.5 V (0.7 × 5 V). The Pico outputs 3.3 V — too low without a level shifter.

```
Pico GP0/GP1 (3.3 V)
    │
74AHCT125 pin 2 (1A)   ← VCC = 5 V (pin 14), GND (pin 7)
    │
74AHCT125 pin 3 (1Y)   ← output swings 0 – 5 V
    │
330–470 Ω              ← series resistor right at connector pad
    │
LED strip DIN (5 V)
```

---

## Power

| Rail | Source | Consumers | Est. max current |
| --- | --- | --- | --- |
| 3.3 V | Pico onboard LDO | Pico, MCP23017, MCP3208, ST7735, level shifter inputs | ~250 mA |
| 5 V | External supply | SK6812 strip, WS2811 LEDs, level shifter VCC (pin 14) | ≥ 3 A |

**5 V current budget:**

| Consumer | Worst case |
| --- | --- |
| SK6812 RGBW, 70 LEDs, full white | 70 × ~60 mA = **4.2 A** |
| SK6812 RGBW, typical (30 % brightness) | ~1.3 A |
| WS2811 RGB, 2 buttons, full | 2 × ~60 mA = 120 mA |

Size the 5 V supply for **≥ 3 A** typical, or up to **5 A** if strip runs at full white.

**Decoupling:**

| Location | Cap value |
| --- | --- |
| MCP23017 VDD (pin 9) | 100 nF + 1 µF ceramic, within 2 mm |
| MCP3208 VDD (pin 16) | 100 nF ceramic, within 2 mm |
| MCP3208 VREF (pin 15) | 100 nF ceramic to AGND |
| 74AHCT125 VCC (pin 14), each IC | 100 nF ceramic |
| SK6812 strip 5 V input connector | 1000 µF / 6.3 V electrolytic + 100 nF ceramic |
| Each MCP3208 analog input | 100 nF to AGND (noise filter) |

---

## USB CDC

### Physical connection

The Pico's onboard **micro-USB connector** carries the CDC link to the Raspberry Pi host. USB D+ and D− are routed internally on the Pico board — no additional USB circuitry is required on this PCB.

```
Raspberry Pi USB-A port
    │
USB A–to–micro-B cable
    │
Pico micro-USB connector (D+, D−, VBUS, GND)
```

- **USB speed:** Full Speed (12 Mbps) — RP2040 supports FS only.
- **VBUS:** the Pico can optionally be powered from the Pi's USB VBUS (5 V, 500 mA). If the board is also powered from an external 5 V supply via VSYS, add a **Schottky diode** (e.g. BAT54, 200 mA) in series with VBUS to VSYS to prevent back-feeding the Pi.
- Keep the **BOOTSEL** button accessible for firmware flashing (hold BOOTSEL, power-cycle → Pico appears as USB mass storage).

### TinyUSB configuration ([tusb_config.h](../tusb_config.h))

| Parameter | Value | Notes |
| --- | --- | --- |
| MCU | `OPT_MCU_RP2040` | |
| OS | `OPT_OS_FREERTOS` | TinyUSB uses FreeRTOS task notifications |
| Classes enabled | CDC only | MSC, HID, MIDI, VENDOR all disabled |
| Endpoint 0 size | 64 bytes | |
| CDC RX buffer | 64 bytes (`CFG_TUD_CDC_RX_BUFSIZE`) | Read per `cdc_task` poll |
| CDC TX buffer | 256 bytes (`CFG_TUD_CDC_TX_BUFSIZE`) | Larger to absorb burst ADC packets |
| CDC endpoint buffer | 64 bytes (`CFG_TUD_CDC_EP_BUFSIZE`) | |

### FreeRTOS tasks ([usb_cdc.c](../src/usb_cdc.c), [main.c](../src/main.c))

| Task | Priority | Stack | Period | Role |
| --- | --- | --- | --- | --- |
| `usb_device_task` | 4 (highest) | 1024 words | 1 ms | Runs `tud_task()` — must be highest priority for timely USB enumeration |
| `cdc_task` | 3 | 512 words | 1 ms | Drains `g_tx_queue` → `tud_cdc_write()`, reads RX into `proto_handle_rx()`, sends heartbeat |

**TX queue** (`g_tx_queue`): depth 16 items × 64 bytes each. Any FreeRTOS task posts packets here; `cdc_task` flushes them on every tick.

### Packet framing ([protocol.h](../src/protocol.h), [protocol.c](../src/protocol.c))

All packets share the same 4-byte header + payload + checksum structure:

```text
Byte 0:  SOF = 0xAA
Byte 1:  Type  (see table below)
Byte 2:  Payload length (0–255)
Byte 3…: Payload (0–255 bytes)
Last:    Checksum = type XOR len XOR payload[0] XOR … XOR payload[n-1]
```

Packet types:

| Type | Direction | Name | Payload struct |
| --- | --- | --- | --- |
| 0x01 | Pico → Pi | ADC data | `adc_packet_t` — 6× uint16 raw counts + uint16 timestamp (14 bytes) |
| 0x02 | Pico → Pi | Digital I/O state | `digital_packet_t` — port_a, port_b, uint16 timestamp (4 bytes) |
| 0x03 | Pi → Pico | LED command | `led_cmd_header_t` + pixel data (chain 0x00 = SK6812, 0x01 = WS2811, 0x02 = MCP LEDs) |
| 0x04 | Pi → Pico | Screen mode | `screen_cmd_t` — mode byte (0=auto, 1=main, 2=warning, 3=lock, 4=bat_warning) |
| 0x05 | Bidirectional | Heartbeat | `heartbeat_pkt_t` — seq byte |
| 0x06 | Pico → Pi | Input event | `event_pkt_t` — event_id + uint16 value |
| 0x07 | Pico → Pi | Error | `error_pkt_t` — error_code byte |
| 0x08 | Pi → Pico | Brightness | `brightness_cmd_t` — target (0=SK6812, 1=WS2811) + level 0–255 |
| 0x09 | Pi → Pico | State override | `mode_cmd_t` — state byte |
| 0x0A | Pi → Pico | Warning severity | `warning_cmd_t` — 9 severity bytes (one per `WARN_ICON_*`) |
| 0x0B | Pico → Pi | ALS data (VEML7700) | `als_packet_t` — als_raw (u16), white_raw (u16), lux_milli (u32), ts_ms (u16) — 10 bytes |

### Heartbeat & connection state

| Parameter | Value |
| --- | --- |
| Heartbeat TX interval | 1000 ms |
| Heartbeat RX timeout | 3000 ms — triggers `EVT_USB_DISCONNECTED`, state → `SYS_WAITING_FOR_PI` |
| Watchdog timeout | 5000 ms (hardware watchdog, serviced by `watchdog_task` every 500 ms) |

Connection state machine transitions:

- Power-on → `SYS_BOOT` → `SYS_INIT` → `SYS_WAITING_FOR_PI`
- USB enumeration + first heartbeat received → `SYS_CONNECTED`
- No heartbeat for 3 s → `SYS_WAITING_FOR_PI`
- Watchdog reset detected on boot → `SYS_ERROR` (sends `ERR_WATCHDOG_RESET` packet once CDC connects)

---

## Connector Summary

| Connector | Type | Pins | Signal details |
| --- | --- | --- | --- |
| SK6812 strip | JST-XH 3-pin 2.54 mm | 5 V, GND, DIN | DIN from 74AHCT125 A pin 3 via 330 Ω |
| WS2811 buttons | JST-XH 3-pin 2.54 mm | 5 V, GND, DIN | DIN from 74AHCT125 B pin 3 via 330 Ω |
| SW1 (guard toggles) | JST-XH 4-pin | GND, PB0, PB1, PB2 | MCP23017 pins 10, 1, 2, 3 |
| SW2 (pushbuttons) | JST-XH 4-pin | GND, PB3, PB4, PB5 | MCP23017 pins 10, 4, 5, 6 |
| SW3 (small toggles) | JST-XH 5-pin | GND, PB6, PB7, PA6, PA7 | MCP23017 pins 10, 7, 8, 27, 28 |
| Key switch | JST-XH 2-pin | GND, PA5 | MCP23017 pins 10, 26 |
| Indicator LED 2 | JST-XH 2-pin | PA0 via 120 Ω, GND | MCP23017 pin 21 |
| Indicator LED 3 | JST-XH 2-pin | PA1 via 120 Ω, GND | MCP23017 pin 22 |
| Indicator LED 4 | JST-XH 2-pin | PA2 via 120 Ω, GND | MCP23017 pin 23 |
| ST7735 TFT | 8-pin 0.1" header | GND, VCC, SCL, SDA, RES, DC, CS, BL | See ST7735 table above |
| Battery voltage | Screw terminal 2-pin | Vin (≤ 13.2 V), GND | CH0 via 30 kΩ/10 kΩ divider |
| External voltage | Screw terminal 2-pin | Vin (≤ 13.2 V), GND | CH1 via 30 kΩ/10 kΩ divider |
| Spare analog CH2–CH5 | 0.1" header | CH2, CH3, CH4, CH5, GND | Direct 0–3.3 V input |
| 5 V power input | Screw terminal / XT30 | 5 V, GND | Size for ≥ 3 A |
| USB to Pi host | Pico onboard micro-USB | D+, D−, GND | TinyUSB CDC |

---

## PCB Layout Notes

- LED power traces (5 V / GND to SK6812 connector) carry up to 4 A — use ≥ 2 mm trace width on 35 µm (1 oz) copper.
- Keep GP0 and GP1 traces < 30 mm from Pico to level shifter input.
- Place 330–470 Ω series resistors at the LED DIN connector pads, not at the level shifter output.
- Route I2C pull-up resistors within 10 mm of MCP23017 pins 12–13.
- Keep SPI traces (GP16–GP22) away from 5 V high-current traces.
- Voltage divider resistors for CH0/CH1 should sit close to MCP3208 input pins (high-impedance nodes pick up noise easily).
- Separate analog GND return from the 5 V LED supply return; join at a single star point near the main 5 V input connector.
- Use a continuous GND copper pour on the bottom layer.
- Keep BOOTSEL accessible for firmware flashing.
