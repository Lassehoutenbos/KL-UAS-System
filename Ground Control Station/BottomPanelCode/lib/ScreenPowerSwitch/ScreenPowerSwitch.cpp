#include "ScreenPowerSwitch.h"

float ScreenPowerSwitch::simulateVoltage(float base, int variation) {
    return base + (random(-variation, variation) / 1000.0f);
}

void ScreenPowerSwitch::drawBatteryIcon(int x, int y, bool active) {
    uint16_t color = active ? ST77XX_GREEN : ST77XX_DARKGREY;
    tft.drawRect(x, y, 24, 12, ST77XX_WHITE);
    tft.fillRect(x + 24, y + 4, 2, 4, color);
    tft.fillRect(x + 2, y + 2, 20, 8, color);
}

void ScreenPowerSwitch::drawPlugIcon(int x, int y, bool active) {
    uint16_t color = active ? ST77XX_WHITE : ST77XX_DARKGREY;
    tft.fillRect(x + 4, y, 2, 6, color);
    tft.fillRect(x + 10, y, 2, 6, color);
    tft.fillRect(x + 2, y + 6, 12, 6, color);
    tft.drawLine(x + 8, y + 12, x + 8, y + 20, color);
}

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

void ScreenPowerSwitch::drawAllVoltages() {
    drawVoltageCentered(vBat, batCenterX);
    drawVoltageCentered(vPlug, plugCenterX);
}

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

void ScreenPowerSwitch::animateSwitch(PowerSource from, PowerSource to) {
    int startAngle = (from == BATTERY) ? -45 : 45;
    int endAngle = (to == BATTERY) ? -45 : 45;
    int steps = 10;

    for (int i = 0; i <= steps; i++) {
        float t = i / (float)steps;
        float angle = startAngle + (endAngle - startAngle) * t;
        drawSwitchArm(angle, ST77XX_YELLOW, true);
        delay(40);
    }

    currentPower = to;
    drawIcons();
    drawSwitchArm(endAngle, ST77XX_YELLOW, true);
}

void ScreenPowerSwitch::begin() {
    randomSeed(analogRead(0));
    tft.initR(INITR_144GREENTAB);
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(ST77XX_BLACK);

    vBat = simulateVoltage(11.6);
    vPlug = simulateVoltage(15.0);
    drawIcons();
    drawSwitchArm(-45, ST77XX_YELLOW, true);
}

void ScreenPowerSwitch::drawWarningIcon(bool visible) {
    uint16_t color = visible ? ST77XX_RED : ST77XX_BLACK;
    tft.setTextSize(10);
    tft.setTextColor(color, ST77XX_BLACK);
    tft.setCursor(centerX - 22, centerY - 20);
    tft.print("!");
}

void ScreenPowerSwitch::drawLockIcon() {
    int bodyW = 30;
    int bodyH = 20;
    int bodyX = centerX - bodyW / 2;
    int bodyY = centerY - bodyH / 2 + 10;
    tft.drawRect(bodyX, bodyY, bodyW, bodyH, ST77XX_WHITE);
    tft.drawRoundRect(bodyX - 4, bodyY - 16, bodyW + 8, 16, 8, ST77XX_WHITE);
}

void ScreenPowerSwitch::showWarning() {
    warningMode = true;
    lockMode = false;
    tft.fillScreen(ST77XX_BLACK);
    warningVisible = true;
    lastBlink = millis();
    lastPulse = millis();
    pulseWhite = false;
    drawWarningIcon(true);
}

void ScreenPowerSwitch::showMainScreen() {
    warningMode = false;
    lockMode = false;
    tft.fillScreen(ST77XX_BLACK);
    drawIcons();
    int angle = (currentPower == BATTERY) ? -45 : 45;
    drawSwitchArm(angle, ST77XX_YELLOW, true);
}

void ScreenPowerSwitch::showLockScreen() {
    warningMode = false;
    lockMode = true;
    tft.fillScreen(ST77XX_BLACK);
    drawLockIcon();
}

void ScreenPowerSwitch::update() {
    unsigned long now = millis();

    if (warningMode) {
        bool redraw = false;
        if (now - lastPulse > warningPulseInterval) {
            pulseWhite = !pulseWhite;
            redraw = true;
            lastPulse = now;
        }
        if (now - lastBlink > warningBlinkInterval) {
            warningVisible = !warningVisible;
            redraw = true;
            lastBlink = now;
        }
        if (redraw) {
            tft.fillScreen(pulseWhite ? ST77XX_WHITE : ST77XX_BLACK);
            drawWarningIcon(warningVisible);
        }
        return;
    }

    if (lockMode) {
        return;
    }

    if (now - lastSwitch > switchInterval) {
        PowerSource next = (currentPower == BATTERY) ? PLUG : BATTERY;
        animateSwitch(currentPower, next);
        lastSwitch = now;
    }

    if (now - lastVoltageUpdate > voltageUpdateInterval) {
        vBat = simulateVoltage(11.6);
        vPlug = simulateVoltage(15.0);
        drawAllVoltages();
        lastVoltageUpdate = now;
    }
}

