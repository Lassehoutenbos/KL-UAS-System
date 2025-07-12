#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <PCA9685.h>

// Forward declarations to avoid NeoPixelBus compilation issues
template<typename T_COLOR_FEATURE, typename T_METHOD> class NeoPixelBus;
class NeoGrbwFeature;
class NeoSk6812Method;
class RgbwColor;

struct rgbwValue{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t w;
};

struct SwitchLedMapping {
    bool hasGpioLed;
    uint8_t gpioPin;
  
    bool hasStripLed;
    uint8_t stripStartIndex;
    uint8_t stripEndIndex;
};

#define ledCount 38
#define numSwitches 10

typedef NeoGrbwFeature NeoPixelColorFeature;
typedef NeoSk6812Method NeoPixelMethod;

// Objectem aanmaken
extern NeoPixelBus<NeoPixelColorFeature, NeoPixelMethod> strip;
extern PCA9685 rgbDriver;

extern const SwitchLedMapping switchMap[numSwitches];

void setupLeds();
void setLed(int switchId, rgbwValue color);
bool startup();
void switchPositionAlert();  // Visual feedback for switches not in low position

#endif // LEDS_H