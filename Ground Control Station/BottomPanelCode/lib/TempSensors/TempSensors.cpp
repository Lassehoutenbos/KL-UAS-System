#include "TempSensors.h"
#include "leds.h"

const uint8_t TempSensors::sensorPins[NUM_SENSORS] = {
    PIN_SENS2,
    PIN_SENS3,
    PIN_SENS4,
    PIN_SENS5
};

const TempSensors::FanMapping TempSensors::fanMap[NUM_FANS] = {
    {FAN1_PWM_CH, 0, 40.0f, 60.0f},
    {FAN2_PWM_CH, 1, 40.0f, 60.0f}
};

void TempSensors::begin() {
    for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
        pinMode(sensorPins[i], INPUT_ANALOG);
        temperatures[i] = 0.0f;
    }
    for (uint8_t i = 0; i < NUM_FANS; ++i) {
        fanSpeeds[i] = 0;
        rgbDriver.setChannelPWM(fanMap[i].pwmChannel, 0);
    }
}

void TempSensors::update() {
    for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
        uint16_t raw = analogRead(sensorPins[i]);
        float temperature = (raw * 330.0f) / 4095.0f; // LM35: 10mV per degC, 3.3V ref
        temperatures[i] = temperature;
    }

    for (uint8_t i = 0; i < NUM_FANS; ++i) {
        const FanMapping &m = fanMap[i];
        float t = temperatures[m.sensorIndex];
        float factor;
        if (t <= m.nominalTemp) {
            factor = 0.0f;
        } else if (t >= m.maxTemp) {
            factor = 1.0f;
        } else {
            factor = (t - m.nominalTemp) / (m.maxTemp - m.nominalTemp);
        }
        uint16_t pwm = static_cast<uint16_t>(factor * 4095.0f);
        setFanSpeed(i, pwm);
    }
}

float TempSensors::getTemperatureC(uint8_t index) const {
    if (index >= NUM_SENSORS) return NAN;
    return temperatures[index];
}

void TempSensors::setFanSpeed(uint8_t index, uint16_t pwm) {
    if (index >= NUM_FANS) return;
    if (pwm > 4095) pwm = 4095;
    fanSpeeds[index] = pwm;
    rgbDriver.setChannelPWM(fanMap[index].pwmChannel, pwm);
}

