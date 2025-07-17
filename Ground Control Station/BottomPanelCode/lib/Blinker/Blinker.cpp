#include "Blinker.h"
#include "pins.h"

// Basic non-blocking blinker implementation.

// Constructor stores the pin number and blink interval.
Blinker::Blinker(uint8_t pin, unsigned long interval)
  : _pin(pin), _interval(interval), _lastToggle(0), _state(LOW), _active(false) {}

// Configure the pin and set the starting state.
void Blinker::begin(bool startState) {
  _state = startState;
  IoExp.digitalWrite(_pin, _state);
  _lastToggle = millis();
  _active = true;
}

// Toggle the pin when enough time has passed.
void Blinker::update() {
  if (!_active) return;

  unsigned long now = millis();
  if (now - _lastToggle >= _interval) {
    _state = !_state;
    IoExp.digitalWrite(_pin, _state);
    _lastToggle = now;
  }
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
  _lastToggle = millis();
  _active = true;
}
