#ifndef SCREENPOWERSWITCH_H
#define SCREENPOWERSWITCH_H
#define ST77XX_DARKGREY 0x7BEF
#define TFT_CS PA9
#define TFT_DC PA10
#define TFT_RST PA8
#define DISPLAY_ROTATION 3

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <pins.h>
#include <Arduino.h>
#include "bitmaps.h"
#include <math.h>

// ScreenPowerSwitch manages the small TFT display on the bottom panel.
// It shows power information and warning/lock screens and animates a
// mechanical style switch graphic between battery and plug sources.

class ScreenPowerSwitch {
public:
    // Initialise the display and internal state.
    void begin();

    // Periodic update handling animations and voltage readings.
    void update();

    // Display a generic warning overlay.
    void showWarningScreen();

    // Display the normal power view.
    void showMainScreen();

    // Display the locked icon.
    void showLockScreen();

    // Display a battery specific warning.
    void showBatWarningScreen();

private:
    enum PowerSource { BATTERY, PLUG };
    PowerSource currentPower = BATTERY;

    unsigned long lastSwitch = 0;
    static const unsigned long switchInterval = 4000;

    unsigned long lastVoltageUpdate = 0;
    static const unsigned long voltageUpdateInterval = 1000;

    static const int centerX = 64;
    static const int centerY = 60;
    static const int armLength = 30;
    static const int batCenterX = 20;
    static const int plugCenterX = 108;

    float lastArmX = centerX;
    float lastArmY = centerY;

    float vBat = 11.4;
    float vPlug = 15;

    // Warning and lock screen state
    bool warningMode = false;
    bool batWarningMode = false; 
    bool lockMode = false;

    enum DisplayMode { MODE_MAIN, MODE_WARNING, MODE_LOCK, MODE_BATWARNING };
    DisplayMode currentMode = MODE_MAIN;

    float prevVBat = -1.0f;
    float prevVPlug = -1.0f;

    // static const uint8_t lockBitmap[];
    // static const uint8_t warningBitmap[];
    // static const uint8_t batWarningBitmap[];


    // Helpers to render static bitmaps for the various overlays.
    void drawWarningIcon();
    void drawLockIcon();
    void drawBatWarningIcon();

    // Generate pseudo random voltage values for demo purposes.
    float simulateVoltage(float base, int variation = 100);

    // Drawing utilities for the main screen contents.
    void drawBatteryIcon(int x, int y, bool active);
    void drawPlugIcon(int x, int y, bool active);
    void drawVoltageCentered(float voltage, int cx);
    void drawAllVoltages();
    void drawIcons();
    void drawSwitchArm(float angleDeg, uint16_t color, bool drawDot = false);
    void animateSwitch(PowerSource from, PowerSource to);


    // Hardware interface for the 1.44" TFT screen.
    Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, PIN_SPI2_MOSI ,PIN_SPI2_SCK , TFT_RST);

};

#endif 
