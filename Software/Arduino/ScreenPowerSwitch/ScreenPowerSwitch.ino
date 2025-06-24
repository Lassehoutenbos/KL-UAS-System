#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define ST77XX_DARKGREY 0x7BEF

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

enum PowerSource { BATTERY,
                   PLUG };
PowerSource currentPower = BATTERY;

unsigned long lastSwitch = 0;
const unsigned long switchInterval = 4000;

unsigned long lastVoltageUpdate = 0;
const unsigned long voltageUpdateInterval = 1000;

const int centerX = 64;
const int centerY = 60;
const int armLength = 30;
const int batCenterX = 20;
const int plugCenterX = 108;

// Vorige armpositie
float lastArmX = centerX;
float lastArmY = centerY;

// Spanningswaarden
float vBat = 11.4;
float vPlug = 15;

// ------------------------ Spanningsimulatie ------------------------
float simulateVoltage(float base, int variation = 100) {
  return base + (random(-variation, variation) / 1000.0);
}

// ------------------------ Iconen ------------------------
void drawBatteryIcon(int x, int y, bool active) {
  uint16_t color = active ? ST77XX_GREEN : ST77XX_DARKGREY;
  tft.drawRect(x, y, 24, 12, ST77XX_WHITE);
  tft.fillRect(x + 24, y + 4, 2, 4, color);
  tft.fillRect(x + 2, y + 2, 20, 8, color);
}

void drawPlugIcon(int x, int y, bool active) {
  uint16_t color = active ? ST77XX_WHITE : ST77XX_DARKGREY;
  tft.fillRect(x + 4, y, 2, 6, color);
  tft.fillRect(x + 10, y, 2, 6, color);
  tft.fillRect(x + 2, y + 6, 12, 6, color);
  tft.drawLine(x + 8, y + 12, x + 8, y + 20, color);
}

// ------------------------ Spanningsweergave ------------------------
void drawVoltageCentered(float voltage, int centerX) {
  char buf[10];
  dtostrf(voltage, 4, 2, buf);      // bv: "3.70"
  int textWidth = 6 * strlen(buf);  // letterbreedte 6px bij textSize 1
  int x = centerX - (textWidth / 2);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(x, 52);
  tft.print("     ");  // wissen
  tft.setCursor(x, 52);
  tft.print(buf);
}

void drawAllVoltages() {
  drawVoltageCentered(vBat, batCenterX);
  drawVoltageCentered(vPlug, plugCenterX);
}

void drawIcons() {
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

// ------------------------ Arm-animatie ------------------------
void drawSwitchArm(float angleDeg, uint16_t color, bool drawDot = false) {
  // Wis vorige arm
  tft.drawLine(centerX, centerY, lastArmX, lastArmY, ST77XX_BLACK);
  if (drawDot) {
    tft.fillCircle(lastArmX, lastArmY, 3, ST77XX_BLACK);
  }

  // Centrale draad
  tft.drawLine(centerX, centerY + 40, centerX, centerY, ST77XX_WHITE);

  // Nieuwe armpositie
  float angleRad = angleDeg * PI / 180.0;
  int endX = centerX + sin(angleRad) * armLength;
  int endY = centerY - cos(angleRad) * armLength;

  // Teken arm
  tft.drawLine(centerX, centerY, endX, endY, color);
  if (drawDot) {
    tft.fillCircle(endX, endY, 3, ST77XX_YELLOW);
  }

  lastArmX = endX;
  lastArmY = endY;
}

void animateSwitch(PowerSource from, PowerSource to) {
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

// ------------------------ Setup & Loop ------------------------
void setup() {
  randomSeed(analogRead(0));
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  vBat = simulateVoltage(11.6);
  vPlug = simulateVoltage(15.0);
  drawIcons();
  drawSwitchArm(-45, ST77XX_YELLOW, true);
}

void loop() {
  unsigned long now = millis();

  // Wissel power source
  if (now - lastSwitch > switchInterval) {
    PowerSource next = (currentPower == BATTERY) ? PLUG : BATTERY;
    animateSwitch(currentPower, next);
    lastSwitch = now;
  }

  // Spanningen verversen
  if (now - lastVoltageUpdate > voltageUpdateInterval) {
    vBat = simulateVoltage(11.6);
    vPlug = simulateVoltage(15.0);
    drawAllVoltages();
    lastVoltageUpdate = now;
  }
}
