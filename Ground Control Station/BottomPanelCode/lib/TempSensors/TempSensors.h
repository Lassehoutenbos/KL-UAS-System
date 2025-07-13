#pragma once

#include <Arduino.h>
#include <pins.h>
#include <leds.h>

class TempSensors {
public:
    void begin();               // setup ADC pins
    void update();              // read all sensors and adjust fans
    float getTemperatureC(uint8_t index) const;  // get last temperature for sensor 0..3

private:
    struct FanMapping {
        uint8_t pwmChannel;    // PCA9685 channel
        uint8_t sensorIndex;   // linked temperature sensor
        float nominalTemp;     // °C where fan starts increasing
        float maxTemp;         // °C for full speed
    };

    static constexpr uint8_t NUM_SENSORS = 4;
    static constexpr uint8_t NUM_FANS = 2;

    static const uint8_t sensorPins[NUM_SENSORS];
    static const FanMapping fanMap[NUM_FANS];

    uint16_t fanSpeeds[NUM_FANS];
    float temperatures[NUM_SENSORS];

    void setFanSpeed(uint8_t index, uint16_t pwm);
};

