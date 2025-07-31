#ifndef BLINKER_H
#define BLINKER_H

// Small utility class to blink a GPIO pin at a configurable interval.

#include <Arduino.h>
#include <STM32FreeRTOS.h>

class Blinker {
public:
  // Create a new blinker on the given GPIO pin.
  Blinker(uint8_t pin, unsigned long interval);

  // Prepare the pin and start the FreeRTOS task. Optionally set the
  // initial output state.
  void begin(bool startState = LOW);

  // Legacy update method kept for compatibility. No longer needed as the
  // blinking is handled inside the FreeRTOS task.
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
  TaskHandle_t _taskHandle;
  bool _state;
  bool _active;

  static void taskFunc(void *arg);
};

#endif
