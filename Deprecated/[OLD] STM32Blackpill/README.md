# Bottom Panel Firmware

This PlatformIO project contains the firmware for the ground control station bottom panel.

## Running unit tests

Hardware is not required for running the tests. After installing [PlatformIO](https://platformio.org/install) run:

```bash
cd "Ground Control Station/BottomPanelCode"
pio test -e native
```

The `native` test environment builds the code for the host computer and uses
stub implementations located in `test/stubs` to replace the hardware specific
functions. Tests are located under `test/test_temp_sensors` and verify the
temperature sensor and fan control logic.
