# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

PICODE is a **Qt 6 / QML ground control station (GCS) application** running on a Raspberry Pi with two 7" touchscreens. It communicates with:
- An **RP2040 Pico** over USB CDC (`/dev/ttyACM0`) — controls all physical hardware (LEDs, switches, brightness, TFT, RS-485 peripherals)
- A **MAVLink drone** over serial or UDP (auto-detected)

## Build

Build with **Qt Creator** using one of two configured kits:
- `Desktop Qt 6.11.0 MinGW 64-bit` — local development/testing on Windows
- `6.11.0-raspberrypi-armv8` — cross-compile for deployment on the Pi

There are no test suites. Manual testing is done by running the app on-device or on desktop.

Build directory layout mirrors `build/<kit-name>-<Debug|Release>/`.

## Architecture

### State layer — `backend/gcsstate.h/.cpp`

`GCSState` is a **QObject singleton** registered in `main.cpp` via `qmlRegisterSingletonInstance`. It is the **single source of truth** for all hardware and drone state in QML. Every property is read-only in QML (emit signals to update), except brightness/worklight which have setters. QML accesses it as `GCSState.propertyName` after `import PICODE`.

Do **not** store UI-local state in GCSState. It belongs in the QML component.

### Hardware bridge — `backend/picolink.h/.cpp`

Parses the CDC binary packet protocol from the Pico. Packet format: `[0xAA][type][len_lo][len_hi][payload...][XOR checksum]`. All incoming packet types call `GCSState::update*()` methods. All outgoing commands are triggered by `GCSState` signals (e.g. `cmdBrightnessChanged`, `cmdWorklightChanged`, `cmdPeriphCmd`).

Key methods to know: `sendFrame()`, `processPacket()`, `updateTftMode()`.

### MAVLink bridge — `backend/mavlinklink.h/.cpp`

Auto-detects MAVLink over serial ports or UDP 14550. Parses raw MAVLink v1/v2 frames manually (no external MAVLink library). Calls `GCSState::updateDroneState()`, `updateBattery()`, etc.

### QML layer

`main.cpp` creates **two separate `QQmlApplicationEngine` instances**, one per screen. Both load `Main.qml` with different `screenIndex` (0/1) and `initialPage` context properties. `GCSState` is shared between both engines via the singleton.

- `Theme.qml` — QML singleton (`QT_QML_SINGLETON_TYPE TRUE`), all colors and font sizes. Always use `Theme.*` tokens, never hard-coded colors.
- `components/` — reusable widgets: `NavBar`, `StatusBar`, `DataCard`, `BigButton`, `SliderRow`, `StatusDot`, `QuickPanel`, `CaseView`, `CaseView3D`
- `pages/` — full-page views loaded by `NavBar`

### Case Digital Twin (`CasePage` + `CaseView` / `CaseView3D`)

The Case page has a 2D canvas view (`CaseView.qml`) and a 3D model view (`CaseView3D.qml`) toggled with a 2D/3D button. Hotspot positions and sensor bindings are loaded from `case_twin_config.json`. The JSON has both `x_norm`/`y_norm` (2D canvas, 0–1 normalised) and `x3d`/`y3d`/`z3d` (glTF world-space metres) per hotspot.

The 3D model is at `qrc:/models/Case v13.gltf` (bounding box: X ±0.232 m, Y −0.419 to +0.170 m, Z −0.015 to +0.374 m; front face at Y ≈ +0.170, front-to-back is the Y axis).

## Key conventions

- **Adding new hardware state**: add a `Q_PROPERTY` + backing member + `NOTIFY` signal to `GCSState`, update it in the relevant `PicoLink` packet handler.
- **Adding a new QML page**: create `pages/Foo.qml`, add it to `qt_add_qml_module` in `CMakeLists.txt`, and add a nav entry in `NavBar.qml`.
- **New QML files must be registered** in the `QML_FILES` list in `CMakeLists.txt` — Qt's module system won't find them otherwise.
- **Non-QML resources** (fonts, model files) use `qt_add_resources` with a prefix.
- `Theme.qml` is a singleton — reference it directly as `Theme.accentYellow`, not as an import alias.

## Hardware reference highlights

Temperatures: `tempCaseA` = PCB power, `tempCaseB` = Pi zone, `tempCaseC` = charger, `tempCaseD` = VRX module, `tempCpuPi` = Pi CPU (read from `/sys/class/thermal/`).

LED chains: SK6812 strip (`chain=0x00`) — switch backlights + warning panel (LEDs 61–69) + worklights (LEDs 70–92). WS2811 RGB (`chain=0x01`) — payload buttons. MCP23017 LEDs (`chain=0x02`) — SW2 button indicators.

RS-485 peripherals: send via `GCSState::sendPeriphCmd(address, cmd, payload)`, receive via `GCSState::peripherals` (QVariantList, each entry has `address`, `name`, `online`, and device-specific fields like `temp`).

## Debug log

The app writes all `qDebug`/`qWarning` output to `C:/Users/houte/Desktop/picode_log.txt` (hardcoded in `main.cpp`).
