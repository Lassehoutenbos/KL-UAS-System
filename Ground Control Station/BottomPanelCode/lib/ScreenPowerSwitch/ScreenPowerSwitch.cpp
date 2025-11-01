#include "ScreenPowerSwitch.h"
#include "bitmaps.h"

// Implementation of the ScreenPowerSwitch class which renders
// various screens on the TFT and animates the selector arm.

// Helper generating a pseudo random voltage around the given base value.
float ScreenPowerSwitch::simulateVoltage(float base, int variation) {
    return base + (random(-variation, variation) / 1000.0f);
}

// Draw the battery icon at the supplied coordinates.
void ScreenPowerSwitch::drawBatteryIcon(int x, int y, bool active) {
    uint16_t color = active ? ST77XX_GREEN : ST77XX_DARKGREY;
    tft.drawRect(x, y, 24, 12, ST77XX_WHITE);
    tft.fillRect(x + 24, y + 4, 2, 4, color);
    tft.fillRect(x + 2, y + 2, 20, 8, color);
}

// Draw the plug icon at the supplied coordinates.
void ScreenPowerSwitch::drawPlugIcon(int x, int y, bool active) {
    uint16_t color = active ? ST77XX_WHITE : ST77XX_DARKGREY;
    tft.fillRect(x + 4, y, 2, 6, color);
    tft.fillRect(x + 10, y, 2, 6, color);
    tft.fillRect(x + 2, y + 6, 12, 6, color);
    tft.drawLine(x + 8, y + 12, x + 8, y + 20, color);
}

// Utility to print a voltage value centred around a coordinate.
void ScreenPowerSwitch::drawVoltageCentered(float voltage, int cx) {
    char buf[10];
    dtostrf(voltage, 4, 2, buf);
    int textWidth = 6 * strlen(buf);
    int x = cx - (textWidth / 2);

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(x, 52);
    tft.print("     ");
    tft.setCursor(x, 52);
    tft.print(buf);
}

// Refresh all voltage readouts on screen.
void ScreenPowerSwitch::drawAllVoltages() {
    drawVoltageCentered(vBat, batCenterX);
    drawVoltageCentered(vPlug, plugCenterX);
}

// Draw labels, icons and the current voltage values.
void ScreenPowerSwitch::drawIcons() {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(batCenterX - 10, 5);
    tft.print("BAT");
    tft.setCursor(plugCenterX - 10, 5);
    tft.print("PLUG");

    drawBatteryIcon(batCenterX - 12, 20, currentPower == BATTERY);
    drawPlugIcon(plugCenterX - 12, 20, currentPower == PLUG);
    drawAllVoltages();
}

// Draw the moving selector arm. When drawDot is true a yellow marker is shown
// at the arm tip to emphasise movement.
void ScreenPowerSwitch::drawSwitchArm(float angleDeg, uint16_t color, bool drawDot) {
    tft.drawLine(centerX, centerY, lastArmX, lastArmY, ST77XX_BLACK);
    if (drawDot) {
        tft.fillCircle(lastArmX, lastArmY, 3, ST77XX_BLACK);
    }

    tft.drawLine(centerX, centerY + 40, centerX, centerY, ST77XX_WHITE);

    float angleRad = angleDeg * PI / 180.0f;
    int endX = centerX + sin(angleRad) * armLength;
    int endY = centerY - cos(angleRad) * armLength;

    tft.drawLine(centerX, centerY, endX, endY, color);
    if (drawDot) {
        tft.fillCircle(endX, endY, 3, ST77XX_YELLOW);
    }

    lastArmX = endX;
    lastArmY = endY;
}

// Animate the arm transitioning between power sources.
void ScreenPowerSwitch::animateSwitch(PowerSource from, PowerSource to) {
    int startAngle = (from == BATTERY) ? -45 : 45;
    int endAngle = (to == BATTERY) ? -45 : 45;
    int steps = 10;

    for (int i = 0; i <= steps; i++) {
        float t = i / (float)steps;
        float angle = startAngle + (endAngle - startAngle) * t;
        drawSwitchArm(angle, ST77XX_YELLOW, true);
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    currentPower = to;
    drawIcons();
    drawSwitchArm(endAngle, ST77XX_YELLOW, true);
}

// Initialise the TFT and clear the screen.
void ScreenPowerSwitch::begin() {
    randomSeed(1);
    tft.initR(INITR_144GREENTAB);
    tft.setSPISpeed(16000000);  // Set SPI to 16MHz (safe speed for ST7735)
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(ST77XX_BLACK);
    prevVBat = vBat;
    prevVPlug = vPlug;
}

// Render the warning bitmap in red.
void ScreenPowerSwitch::drawWarningIcon() {
    tft.fillScreen(ST77XX_RED);
    tft.drawBitmap(0, 0, warningBitmap, 128, 128, ST77XX_BLACK);
}

// Render the lock bitmap on a white background.
void ScreenPowerSwitch::drawLockIcon() {
    tft.fillScreen(ST77XX_WHITE);
    tft.drawBitmap(0, 0, lockBitmap, 128, 128, ST77XX_BLACK);
}

// Render the battery warning bitmap.
void ScreenPowerSwitch::drawBatWarningIcon(){
    tft.fillScreen(ST7735_YELLOW);
    tft.drawBitmap(0,0, batWarningBitmap, 128, 128, ST77XX_BLACK);
}

// Switch display to the generic warning screen.
void ScreenPowerSwitch::showWarningScreen() {
    if (currentMode == MODE_WARNING) {
        return;
    }

    warningMode = true;
    batWarningMode = false;
    lockMode = false;
    tft.fillScreen(ST77XX_BLACK);
    drawWarningIcon();
    currentMode = MODE_WARNING;
}

// Display a battery specific warning overlay.
void ScreenPowerSwitch::showBatWarningScreen() {
    if (currentMode == MODE_BATWARNING){
        return;
    }
    warningMode = false;
    batWarningMode = true;
    lockMode = false;
    drawBatWarningIcon();
    currentMode = MODE_BATWARNING;
    
}

// Restore the normal main screen layout.
void ScreenPowerSwitch::showMainScreen() {
    if (currentMode == MODE_MAIN) {
        return;
    }

    warningMode = false;
    lockMode = false;
    batWarningMode = false;
    tft.fillScreen(ST77XX_BLACK);
    drawIcons();
    int angle = (currentPower == BATTERY) ? -45 : 45;
    drawSwitchArm(angle, ST77XX_YELLOW, true);
    currentMode = MODE_MAIN;
}

// Display the lock overlay to indicate the system is locked.
void ScreenPowerSwitch::showLockScreen() {
    if (currentMode == MODE_LOCK) {
        return;
    }

    warningMode = false;
    lockMode = true;
    batWarningMode = false;
    tft.fillScreen(ST77XX_BLACK);
    drawLockIcon();
    currentMode = MODE_LOCK;
}

// Periodic update handling animation timing and voltage refresh.
void ScreenPowerSwitch::update() {
    unsigned long now = millis();

    if (warningMode || lockMode || batWarningMode) {
        return;
    }

    if (now - lastSwitch > switchInterval) {
        PowerSource next = (currentPower == BATTERY) ? PLUG : BATTERY;
        animateSwitch(currentPower, next);
        lastSwitch = now;
    }

    if (now - lastVoltageUpdate > voltageUpdateInterval) {
        float newVBat = simulateVoltage(11.6);
        float newVPlug = simulateVoltage(15.0);

        bool changed = (fabs(newVBat - prevVBat) > 0.01f) ||
                       (fabs(newVPlug - prevVPlug) > 0.01f);

        if (changed) {
            vBat = newVBat;
            vPlug = newVPlug;
            prevVBat = newVBat;
            prevVPlug = newVPlug;
            drawAllVoltages();
        }

        lastVoltageUpdate = now;
    }
}

