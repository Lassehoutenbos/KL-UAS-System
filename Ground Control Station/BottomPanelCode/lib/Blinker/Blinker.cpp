#include "Blinker.h"
#include "pins.h"
#include <STM32FreeRTOS.h>

// Implementation of the Blinker class using a FreeRTOS task.

// Constructor stores the pin number and blink interval.
Blinker::Blinker(uint8_t pin, unsigned long interval)
  : _pin(pin), _interval(interval), _state(LOW), _active(false), _taskHandle(nullptr) {}

// Configure the pin and set the starting state.
void Blinker::begin(bool startState) {
  _state = startState;
  IoExp.digitalWrite(_pin, _state);
  _active = true;
  xTaskCreate(taskTrampoline, "Blink", 128, this, 1, &_taskHandle);
}

// Toggle the pin when enough time has passed.
void Blinker::update() {
  // handled by FreeRTOS task
}

// Change how fast the LED blinks.
void Blinker::setInterval(unsigned long interval) {
  _interval = interval;
}

// Stop blinking and turn the LED off.
void Blinker::stop() {
  _active = false;
  IoExp.digitalWrite(_pin, LOW);
}

// Resume blinking from the current time.
void Blinker::start() {
  _active = true;
}

void Blinker::taskTrampoline(void *pv) {
  Blinker *b = static_cast<Blinker *>(pv);
  for (;;) {
    if (b->_active) {
      b->_state = !b->_state;
      IoExp.digitalWrite(b->_pin, b->_state);
      vTaskDelay(pdMS_TO_TICKS(b->_interval));
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
