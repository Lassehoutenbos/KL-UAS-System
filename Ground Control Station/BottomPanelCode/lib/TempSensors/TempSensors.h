#pragma once

// Driver for reading temperature sensors and controlling fans accordingly.

#include <Arduino.h>
#include <pins.h>
#include <leds.h>

class TempSensors {
public:
    // Initialise ADC pins and fan outputs.
    void begin();
    // Read sensors and update fan speeds.
    void update();
    // Retrieve the last measured temperature for sensor 0..3.
    float getTemperatureC(uint8_t index) const;

    struct FanSensorConfig {
        uint8_t sensorIndex;   // linked temperature sensor
        float nominalTemp;     // °C where fan starts increasing
        float maxTemp;         // °C for full speed
    };

    struct FanMapping {
        uint8_t pwmChannel;                 // PCA9685 channel
        const FanSensorConfig* sensors;     // list of sensors for this fan
        uint8_t numSensors;                 // number of linked sensors
    };

private:
    static constexpr uint8_t NUM_SENSORS = 4;
    static constexpr uint8_t NUM_FANS = 2;

    static const uint8_t sensorPins[NUM_SENSORS];
    static const FanMapping fanMap[NUM_FANS];

    uint16_t fanSpeeds[NUM_FANS];
    float temperatures[NUM_SENSORS];

    void setFanSpeed(uint8_t index, uint16_t pwm);
};

