#include "Blinker.h"
#include "pins.h"

Blinker::Blinker(uint8_t pin, unsigned long interval)
  : _pin(pin), _interval(interval), _lastToggle(0), _state(LOW), _active(false) {}

void Blinker::begin(bool startState) {
  _state = startState;
  IoExp.digitalWrite(_pin, _state);
  _lastToggle = millis();
  _active = true;
}

void Blinker::update() {
  if (!_active) return;

  unsigned long now = millis();
  if (now - _lastToggle >= _interval) {
    _state = !_state;
    IoExp.digitalWrite(_pin, _state);
    _lastToggle = now;
  }
}

void Blinker::setInterval(unsigned long interval) {
  _interval = interval;
}

void Blinker::stop() {
  _active = false;
  IoExp.digitalWrite(_pin, LOW);
}

void Blinker::start() {
  _lastToggle = millis();
  _active = true;
}
