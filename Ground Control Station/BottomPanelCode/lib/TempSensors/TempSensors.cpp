#include "TempSensors.h"

const uint8_t TempSensors::sensorPins[4] = {
    PIN_SENS2,
    PIN_SENS3,
    PIN_SENS4,
    PIN_SENS5
};

void TempSensors::begin() {
    for (uint8_t i = 0; i < 4; ++i) {
        pinMode(sensorPins[i], INPUT_ANALOG);
        temperatures[i] = 0.0f;
    }
}

void TempSensors::update() {
    for (uint8_t i = 0; i < 4; ++i) {
        uint16_t raw = analogRead(sensorPins[i]);
        float temperature = (raw * 330.0f) / 4095.0f; // LM35: 10mV per degC, 3.3V ref
        temperatures[i] = temperature;
    }
}

float TempSensors::getTemperatureC(uint8_t index) const {
    if (index >= 4) return NAN;
    return temperatures[index];
}

