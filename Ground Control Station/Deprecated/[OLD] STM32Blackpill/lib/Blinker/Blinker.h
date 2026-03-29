#ifndef BLINKER_H
#define BLINKER_H

// Small utility class to blink a GPIO pin at a configurable interval.

#include <Arduino.h>

class Blinker {
public:
  // Create a new blinker on the given GPIO pin.
  Blinker(uint8_t pin, unsigned long interval);

  // Prepare the pin and optionally set the initial state.
  void begin(bool startState = LOW);

  // Toggle the pin when the interval elapsed.
  void update();

  // Change the blinking interval.
  void setInterval(unsigned long interval);

  // Disable blinking and force the pin low.
  void stop();

  // Resume blinking.
  void start();

private:
  uint8_t _pin;
  unsigned long _interval;
  unsigned long _lastToggle;
  bool _state;
  bool _active;
};

#endif
