#include "Blinker.h"
#include "pins.h"

// FreeRTOS powered non-blocking blinker implementation.

Blinker::Blinker(uint8_t pin, unsigned long interval)
    : _pin(pin), _interval(interval), _taskHandle(nullptr),
      _state(LOW), _active(false) {}

void Blinker::begin(bool startState) {
    _state = startState;
    IoExp.digitalWrite(_pin, _state);
    _active = true;

    if (_taskHandle == nullptr) {
        xTaskCreate(taskFunc, "BLNK", 128, this, 1, &_taskHandle);
    }
}

void Blinker::update() {
    // Handled by the FreeRTOS task
}

void Blinker::setInterval(unsigned long interval) {
    _interval = interval;
}

void Blinker::stop() {
    _active = false;
    IoExp.digitalWrite(_pin, LOW);
}

void Blinker::start() {
    _active = true;
}

void Blinker::taskFunc(void *arg) {
    Blinker *self = static_cast<Blinker *>(arg);
    for (;;) {
        if (self->_active) {
            self->_state = !self->_state;
            IoExp.digitalWrite(self->_pin, self->_state);
            vTaskDelay(pdMS_TO_TICKS(self->_interval));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
