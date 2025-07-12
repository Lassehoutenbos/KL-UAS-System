#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <PCA9685.h>
#include <Adafruit_NeoPixel.h>

// Simple RGBW color struct used with Adafruit_NeoPixel
struct RgbwColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
    RgbwColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t w = 0)
        : r(r), g(g), b(b), w(w) {}
};

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

// Number of LEDs on the RGBW strip
#define ledCount 38
#define numSwitches 10

// Adafruit NeoPixel strip instance
extern Adafruit_NeoPixel strip;
extern PCA9685 rgbDriver;

extern const SwitchLedMapping switchMap[numSwitches];

void setupLeds();
void setLed(int switchId, rgbwValue color);
bool startup();
void switchPositionAlert();  // Visual feedback for switches not in low position

#endif // LEDS_H