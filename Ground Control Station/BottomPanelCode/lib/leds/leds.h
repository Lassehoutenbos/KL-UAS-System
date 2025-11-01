#ifndef LEDS_H
#define LEDS_H

// LED control utilities for the bottom panel. Handles both GPIO LEDs and the
// RGBW LED strip via the PCA9685 driver.

#include <Arduino.h>
#include <PCA9685.h>
#include <Adafruit_NeoPixel.h>
#include <STM32FreeRTOS.h>

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
// LED Strip Layout (71 LEDs total):
// - LEDs 0-37:  Switch LEDs (38 LEDs total, distributed among 10 switches)
// - LEDs 38-47: Warning Panel (10 LEDs for status indicators)  
// - LEDs 48-70: Worklight (23 LEDs for illumination)
#define ledCount 71
#define numSwitches 10

// Adafruit NeoPixel strip instance
extern Adafruit_NeoPixel strip;
extern PCA9685 rgbDriver;

extern const SwitchLedMapping ledMap[numSwitches];

// Initialise LED drivers.
void setupLeds();
// Set the colour for a particular switch LED.
void setLed(int switchId, rgbwValue color, bool blinking = false);
// Update blinking LEDs (call this regularly in main loop)
void updateLeds();
// Startup animation played once at boot.
bool startup();
// Visual feedback for switches not in low position.
void switchPositionAlert();
void failSafe(int switchId);

#endif // LEDS_H