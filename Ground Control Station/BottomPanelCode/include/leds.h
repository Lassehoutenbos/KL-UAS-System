#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <PCA9685.h>
#include <NeoPixelBus.h>

struct rgbwValue{
    u_int16_t r;
    u_int16_t g;
    u_int16_t b;
    u_int16_t w;
};

struct SwitchLedMapping {
    bool hasGpioLed;
    uint8_t gpioPin;
  
    bool hasStripLed;
    uint8_t stripStartIndex;
    uint8_t stripEndIndex;  // Inclusief!
};

#define ledCount 14
#define numSwitches 10

typedef NeoGrbwFeature NeoPixelColorFeature;
typedef NeoSk6812Method NeoPixelMethod;

// Objectem aanmaken
extern NeoPixelBus<NeoPixelColorFeature, NeoPixelMethod> strip;
extern PCA9685 rgbDriver;

extern const SwitchLedMapping switchMap[numSwitches];

void setupLeds();
void setLed(int switchId, rgbwValue color);

#endif // LEDS_H