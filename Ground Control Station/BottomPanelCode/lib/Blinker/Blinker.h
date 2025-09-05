#ifndef BLINKER_H
#define BLINKER_H

// Small utility class to blink a GPIO pin using a dedicated FreeRTOS task.

#include <Arduino.h>
#include <STM32FreeRTOS.h>

class Blinker {
public:
  // Create a new blinker on the given GPIO pin.
  Blinker(uint8_t pin, unsigned long interval);

  // Prepare the pin and optionally set the initial state.
  void begin(bool startState = LOW);

  // (Legacy) manual update, kept for compatibility. No-op when using FreeRTOS.
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
  bool _state;
  bool _active;
  TaskHandle_t _taskHandle;

  static void taskTrampoline(void *);
};

#endif

