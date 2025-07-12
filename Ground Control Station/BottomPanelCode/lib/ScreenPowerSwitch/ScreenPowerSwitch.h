#ifndef SCREENPOWERSWITCH_H
#define SCREENPOWERSWITCH_H
#define ST77XX_DARKGREY 0x7BEF
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define DISPLAY_ROTATION 3

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Arduino.h>

class ScreenPowerSwitch {
public:
    void begin();
    void update();

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

    float simulateVoltage(float base, int variation = 100);
    void drawBatteryIcon(int x, int y, bool active);
    void drawPlugIcon(int x, int y, bool active);
    void drawVoltageCentered(float voltage, int cx);
    void drawAllVoltages();
    void drawIcons();
    void drawSwitchArm(float angleDeg, uint16_t color, bool drawDot = false);
    void animateSwitch(PowerSource from, PowerSource to);

    Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
};

#endif // SCREENPOWERSWITCH_H
