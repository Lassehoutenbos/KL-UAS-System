# Code Documentation

This document provides an overview of the source code found in the
`Ground Control Station/BottomPanelCode` directory.

## File Structure

- **src/main.cpp** – entry point for the firmware
- **lib/Blinker** – simple class to blink GPIO LEDs
- **lib/SwitchHandler** – generic switch abstraction
- **lib/Switches** – initialization and processing for all panel switches
- **lib/leds** – LED driver and startup animations
- **lib/pins** – board pin assignments and setup helpers
- **lib/ScreenPowerSwitch** – graphical display for power source state

Each section below briefly describes the classes and functions in these modules.

## main.cpp

Sets up the USB HID interface, initializes hardware and runs the main update loop. The screen display and switch logic are updated continuously:

```cpp
ScreenPowerSwitch powerDisplay;

void setup() {
  USB_Begin();            // enable USB
  BootKeyboard.begin();   // start HID keyboard
  setupPins();            // configure MCU and IO expander pins
  powerDisplay.begin();   // initialize display
  Switches::begin();      // register all switches
}

void loop() {
  powerDisplay.update();  // handle screen state
  Switches::update();     // poll switch events
  powerDisplay.showWarning();  // show warning when needed
  delay(1);               // short delay
}
```

Source: `src/main.cpp` lines 1–34.

## Blinker

Located in `lib/Blinker`. Provides basic LED blinking using the IO expander. Relevant parts:

```cpp
class Blinker {
  Blinker(uint8_t pin, unsigned long interval);
  void begin(bool startState = LOW);
  void update();
  void setInterval(unsigned long interval);
  void stop();
  void start();
};
```

— `Blinker.h` lines 1–22.

```cpp
void Blinker::update() {
  if (!_active) return;
  unsigned long now = millis();
  if (now - _lastToggle >= _interval) {
    _state = !_state;
    IoExp.digitalWrite(_pin, _state);
    _lastToggle = now;
  }
}
```

— `Blinker.cpp` lines 12–22.

## SwitchHandler

Generic helper to manage multiple switches with callbacks. Key API:

```cpp
class Switch {
  using Callback = std::function<void(bool)>;
  Switch(int pin, Callback cb);
  void update();
};

namespace SwitchHandler {
  void addSwitch(int pin, Switch::Callback callback);
  void updateAll();
}
```

— `SwitchHandler.h` lines 1–23.

Switch states are read either via the IO expander or directly from GPIO depending on debug options. The `updateAll()` function iterates over all registered `Switch` objects.

## Switches

Defines colours and handles registration of all panel switches. Example snippet:

```cpp
rgbwValue onColorValue  = {0, 255, 0, 0};   // green when pressed
rgbwValue offColorValue = {255, 0, 0, 0};   // red when released

void begin() {
  SwitchHandler::addSwitch(PINIO_SW0, [](bool state){
    if (state) BootKeyboard.write(KEY_F13);
    setLed(0, state ? onColorValue : offColorValue);
  });
  // remaining switches configured similarly...
}
```

— `Switches.cpp` lines 10–99.

Function `allSwitchesLow()` checks that every switch connected to the IO expander is in the low position before powering other devices.

## Pins

Lists all microcontroller and IO expander pins. The `setupPins()` function configures the required modes for SPI, I2C and the expander pins.

```cpp
SPIClass SPI_2(PIN_SPI2_MOSI, PIN_SPI2_MISO, PIN_SPI2_SCK);
Adafruit_MCP23X17 IoExp;

void setupPins() {
  Wire.begin();
  SPI_2.begin();
  IoExp.begin_I2C(0x20);
  IoExp.pinMode(PINIO_SW0, INPUT);
  // ...
}
```

— `pins.cpp` lines 1–41.

## leds

Controls both the RGB LED strip and discrete GPIO LEDs. It also contains startup animations and warnings when switches are not in the correct position.

Important structures:

```cpp
struct rgbwValue { uint16_t r, g, b, w; };
struct SwitchLedMapping {
  bool hasGpioLed;
  uint8_t gpioPin;
  bool hasStripLed;
  uint8_t stripStartIndex;
  uint8_t stripEndIndex;
};
```

— `leds.h` lines 8–31.

Functions include `setupLeds()`, `setLed()`, `startup()` for the boot animation and `switchPositionAlert()` which now quickly blinks pressed switches and pulses unpressed ones.

## ScreenPowerSwitch

Provides a small UI on a 1.44" ST7735 display showing the current power source. The class can also display warnings and a lock screen. Key methods:

```cpp
class ScreenPowerSwitch {
  void begin();            // initialise display
  void update();           // handle animations
  void showWarning();      // flash warning screen
  void showMainScreen();   // default view
  void showLockScreen();   // static lock icon
};
```

— `ScreenPowerSwitch.h` lines 15–23.

The implementation draws simple icons for battery and plug sources, animates a selector arm and simulates voltage values for demonstration purposes. See `ScreenPowerSwitch.cpp` for details of the drawing and animation logic.

## Additional Files

The repository also contains Arduino example code under `Ground Control Station/Arduino testcode` which replicates the display logic in a single `.ino` file. Backup configuration files for the flight controller are stored in `Software/BackupsFC`.

### Disclaimer

This documentation is provided for reference only. Use the code
responsibly and ensure your hardware meets all applicable safety
requirements.
