# RS-485 Peripheral Slave Framework

Reusable slave-side library for the GCS RS-485 peripheral bus. Targets
**RP2040**, **STM32G031**, and **ESP32** out of the box. New peripheral
boards write only their command handlers and a small `board_<mcu>.c`.

The bus protocol is defined in
[`../Docs/RS485_PERIPHERAL_BUS.md`](../Docs/RS485_PERIPHERAL_BUS.md).

## Layout

```
core/   portable C — framing FSM, CRC, dispatch, PING/PONG, STREAM, /INT
hal/    one .c per MCU implementing the 8-function hal.h interface
cmake/  framework.cmake + per-MCU back-ends (rp2040 / stm32g0 / esp32)
```

## Writing a new peripheral

1. Pick an address (`Docs/RS485_PERIPHERAL_BUS.md` § Address space).
2. Create `MyPeriph/src/main.c`, `src/board.h`, and one
   `src/board_<mcu>.c` per MCU you want to support.
3. In `main.c`, declare a `rs485_handler_t[]` and call
   `rs485_slave_init(&cfg)` + `rs485_slave_poll()` in a loop.
4. Add a `MyPeriph/CMakeLists.txt` that includes `framework.cmake`
   and calls `add_peripheral(NAME … MCU … SOURCES …)`.
5. Build: `cmake -B build -DTARGET_MCU=rp2040 . && cmake --build build`.

`Searchlight/`, `PanTilt/`, `LightBar/` are working examples.

## Porting to a new MCU

Implement the 8 functions in `hal/hal.h` against your SDK and add a
`cmake/<mcu>.cmake` back-end mirroring `rp2040.cmake`. The portable core
in `core/rs485_slave.c` does not include any MCU header — verified by
compiling it with `-ffreestanding` against a stub HAL.

## MCU back-ends

| MCU | UART | DE pin | Time | Build prereq |
|---|---|---|---|---|
| RP2040 | `uart0` GP0/GP1 | GP2 (GPIO) | `to_ms_since_boot` | `PICO_SDK_PATH` env or `-DPICO_SDK_PATH=…` |
| STM32G031 | USART1 PA9/PA10 | PA8 (GPIO) | SysTick @ 1 kHz | `arm-none-eabi-gcc` on PATH; CMSIS pulled via FetchContent |
| ESP32 | `UART1` configurable | RTS pin (driver auto-toggle) | `esp_timer_get_time` | ESP-IDF v5.x sourced (`IDF_PATH` set) |

Pin defaults are overridable: pass `-DHAL_<MCU>_PIN_<…>=<n>` at compile
time, or `#define` before including the HAL header.

## Out of scope (v1)

- CH32V003 HAL (mentioned in the spec; not requested yet).
- Async TX (interrupt/DMA-driven); v1 uses blocking `hal_uart_write`.
- Bootloader / OTA over RS-485.
- Persistent param storage in flash — handlers hold defaults at boot.
- Discovery / hot-plug; the master keeps a static address list.
- Real SK6812 PIO drive in `LightBar` — the app tracks state and exposes
  a power-estimate stream; pixel push is left to a follow-up.
