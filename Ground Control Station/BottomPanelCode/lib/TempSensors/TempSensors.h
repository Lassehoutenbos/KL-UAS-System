#pragma once

#include <Arduino.h>
#include <pins.h>

class TempSensors {
public:
    void begin();               // setup ADC pins
    void update();              // read all sensors
    float getTemperatureC(uint8_t index) const;  // get last temperature for sensor 0..3

private:
    static const uint8_t sensorPins[4];
    float temperatures[4];
};

