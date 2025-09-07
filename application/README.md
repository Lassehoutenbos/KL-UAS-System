# Ground Control Station Application

This is a minimal ground control station (GCS) example built with **Qt 6**. It demonstrates how to
integrate different subsystems such as serial communication and HID devices in a cross‑platform GUI.

## Features

* **Serial Port** – Uses `QSerialPort` to send/receive data from flight hardware.
* **HID** – Uses `hidapi` to read generic USB HID devices such as gamepads.
* **Qt Widgets UI** – Basic `QMainWindow` with log output.

## Build Instructions

```bash
cmake -S application -B build
cmake --build build
./build/gcs
```

Make sure `qt6-base-dev`, `qt6-serialport-dev`, and `libhidapi-dev` are installed.
