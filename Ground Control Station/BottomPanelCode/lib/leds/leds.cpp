#include "leds.h"
#include "pins.h"
#include "Blinker.h"
#include <STM32FreeRTOS.h>

// Implementation for LED effects and helper routines.

Adafruit_NeoPixel strip(ledCount, PB3, NEO_GRBW + NEO_KHZ800);
PCA9685 rgbDriver;

/* | Has IO | IO pin | Has Strip | Index start | Index end |*/
const SwitchLedMapping ledMap[numSwitches] = {
    {false, 0, true, 14, 17},
    {false, 0, true, 19, 22},
    {false, 0, true, 24, 27},
    {true, PINIO_SW3LED, true, 0, 3},
    {true, PINIO_SW4LED, true, 5, 8},
    {true, PINIO_SW5LED, true, 10, 14},
    {true, PINIO_SW6LED, true, 28, 33},
    {true, PINIO_SW7LED, true, 28, 33},
    {false, 0, false, 33, 38},  // PWM RGB
    {false, 0, false, 33, 38},  // PWM RGB
};

// Mapping for reading the switch states from the IO expander
const uint8_t switchPins[numSwitches] = {
    PINIO_SW0, PINIO_SW1, PINIO_SW2, PINIO_SW3, PINIO_SW4,
    PINIO_SW5, PINIO_SW6, PINIO_SW7, PINIO_SW8, PINIO_SW9
};

// Blinker objects for GPIO LEDs only
Blinker blinkerSW3(PINIO_SW3LED, 500);
Blinker blinkerSW4(PINIO_SW4LED, 500);
Blinker blinkerSW5(PINIO_SW5LED, 500);
Blinker blinkerSW6(PINIO_SW6LED, 500);
Blinker blinkerSW7(PINIO_SW7LED, 500);

// Array to easily access blinkers (nullptr for switches without GPIO LEDs)
Blinker* blinkers[numSwitches] = {
    nullptr, nullptr, nullptr, &blinkerSW3, &blinkerSW4,
    &blinkerSW5, &blinkerSW6, &blinkerSW7, nullptr, nullptr  // SW0-2, SW8-9 don't use GPIO blinkers
};

// Blinking state for strip LEDs and PWM LEDs
bool stripBlinking[numSwitches] = {false};
rgbwValue stripBlinkColor[numSwitches] = {{0}};
TickType_t lastBlinkTime[numSwitches] = {0};
bool blinkState[numSwitches] = {false};

// Initialise the NeoPixel strip and PCA9685 driver.
void setupLeds(){
    
    // Led driver setup
    rgbDriver.resetDevices();
    rgbDriver.init();

    //  SK6812 setup
    strip.begin();
    strip.show();

    // Blinker setup for GPIO LEDs
    for(int i = 0; i < numSwitches; i++) {
        if(blinkers[i] != nullptr) {
            blinkers[i]->begin();
        }
        stripBlinking[i] = false;
        stripBlinkColor[i] = {0, 0, 0, 0};
        lastBlinkTime[i] = xTaskGetTickCount();
        blinkState[i] = false;
    }
}

// Set the colour of a switch LED or LED group.
void setLed(int switchId, rgbwValue color, bool blinking){
    if(switchId < 0 || switchId >= numSwitches) return;

    const SwitchLedMapping& m = ledMap[switchId];

    // Handle GPIO LED blinking
    if (m.hasGpioLed && blinkers[switchId] != nullptr) {
        if (blinking && (color.r + color.g + color.b + color.w > 0)) {
            blinkers[switchId]->start();
        } else {
            blinkers[switchId]->stop();
            // Set steady state based on color
            bool active = (color.r + color.g + color.b + color.w) > 0;
            IoExp.digitalWrite(m.gpioPin, active ? HIGH : LOW);
        }
    }

    // Handle PWM LED blinking for switches 8 and 9
    if (switchId == 8) {
        stripBlinking[switchId] = blinking && (color.r + color.g + color.b + color.w > 0);
        stripBlinkColor[switchId] = color;
        if (!blinking) {
            rgbDriver.setChannelPWM(0, color.r);       
            rgbDriver.setChannelPWM(1, color.g);
            rgbDriver.setChannelPWM(2, color.b);
        }
        return;
    }

    if (switchId == 9) {
        stripBlinking[switchId] = blinking && (color.r + color.g + color.b + color.w > 0);
        stripBlinkColor[switchId] = color;
        if (!blinking) {
            rgbDriver.setChannelPWM(3, color.r);
            rgbDriver.setChannelPWM(4, color.g);
            rgbDriver.setChannelPWM(5, color.b);
        }
        return;
    }

    // Handle strip LED blinking
    if (m.hasStripLed) {
        stripBlinking[switchId] = blinking && (color.r + color.g + color.b + color.w > 0);
        stripBlinkColor[switchId] = color;
        
        if (!blinking) {
            RgbwColor col((color.r * 255UL) / 4095,
                         (color.g * 255UL) / 4095,
                         (color.b * 255UL) / 4095,
                         (color.w * 255UL) / 4095);
            for (uint8_t i = m.stripStartIndex; i <= m.stripEndIndex; i++) {
              strip.setPixelColor(i, col.r, col.g, col.b, col.w);
            }
            strip.show();
        }
    }
}

// Update blinking LEDs (called from a FreeRTOS task)
void updateLeds() {
    unsigned long currentTime = xTaskGetTickCount();
    bool stripNeedsUpdate = false;
    
    for (int i = 0; i < numSwitches; i++) {
        if (stripBlinking[i]) {
            // Check if it's time to toggle (500ms interval)
            if (currentTime - lastBlinkTime[i] >= pdMS_TO_TICKS(500)) {
                blinkState[i] = !blinkState[i];
                lastBlinkTime[i] = currentTime;
                
                const SwitchLedMapping& m = ledMap[i];
                
                // Handle PWM LEDs (switches 8 and 9)
                if (i == 8) {
                    if (blinkState[i]) {
                        rgbDriver.setChannelPWM(0, stripBlinkColor[i].r);
                        rgbDriver.setChannelPWM(1, stripBlinkColor[i].g);
                        rgbDriver.setChannelPWM(2, stripBlinkColor[i].b);
                    } else {
                        rgbDriver.setChannelPWM(0, 0);
                        rgbDriver.setChannelPWM(1, 0);
                        rgbDriver.setChannelPWM(2, 0);
                    }
                } else if (i == 9) {
                    if (blinkState[i]) {
                        rgbDriver.setChannelPWM(3, stripBlinkColor[i].r);
                        rgbDriver.setChannelPWM(4, stripBlinkColor[i].g);
                        rgbDriver.setChannelPWM(5, stripBlinkColor[i].b);
                    } else {
                        rgbDriver.setChannelPWM(3, 0);
                        rgbDriver.setChannelPWM(4, 0);
                        rgbDriver.setChannelPWM(5, 0);
                    }
                }
                // Handle strip LEDs
                else if (m.hasStripLed) {
                    if (blinkState[i]) {
                        RgbwColor col((stripBlinkColor[i].r * 255UL) / 4095,
                                     (stripBlinkColor[i].g * 255UL) / 4095,
                                     (stripBlinkColor[i].b * 255UL) / 4095,
                                     (stripBlinkColor[i].w * 255UL) / 4095);
                        for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; j++) {
                            strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                        }
                    } else {
                        for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; j++) {
                            strip.setPixelColor(j, 0, 0, 0, 0);
                        }
                    }
                    stripNeedsUpdate = true;
                }
            }
        }
    }
    
    // Update strip if any strip LED changed
    if (stripNeedsUpdate) {
        strip.show();
    }
}

// Play a brief startup animation on all LEDs.
bool startup() {
    const uint32_t duration = 3000;      // 3 seconden animatie
    const uint32_t interval = 30;        // interval tussen frames (voor pulsen)
    const uint32_t blinkInterval = 500;  // GPIO-knipper interval

    uint32_t startTime = millis();
    uint32_t lastBlink = 0;
    bool blinkState = false;

    while (millis() - startTime < duration) {
        float progress = (millis() - startTime) / (float)duration;

        // Pulse brightness (sinus curve → smooth up and down)
        float t = (millis() % 1000) / 1000.0f;
        float brightness = 0.5f * (1.0f + sin(t * 2.0f * PI));  // tussen 0–1
        uint8_t level = brightness * 255;

        // Stel RGBW kleur in (wit blijft uit voor extra effect)
        rgbwValue pulseColor = {level, level, level, 0};
        for (int i = 0; i < numSwitches; ++i) {
            const auto& m = ledMap[i];
            if (m.hasStripLed) {
                RgbwColor col(pulseColor.r, pulseColor.g, pulseColor.b, pulseColor.w);
                for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; j++) {
                    strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                }
            }
        }
        strip.show();

        // Laat GPIO LEDs knipperen
        if (millis() - lastBlink >= blinkInterval) {
            blinkState = !blinkState;
            lastBlink = millis();

            for (int i = 0; i < numSwitches; ++i) {
                const auto& m = ledMap[i];
                if (m.hasGpioLed) {
                    IoExp.digitalWrite(m.gpioPin, blinkState ? HIGH : LOW);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(interval));  // korte pauze voor de animatie
    }

    // Alles uitzetten na animatie
    for (int i = 0; i < ledCount; ++i) {
        strip.setPixelColor(i, 0, 0, 0, 0);
    }
    strip.show();

    for (int i = 0; i < numSwitches; ++i) {
        const auto& m = ledMap[i];
        if (m.hasGpioLed) {
            IoExp.digitalWrite(m.gpioPin, LOW);
        }
    }

    return true;
}


// Warning animation used when a switch is not in the expected position.
void switchPositionAlert() {
    static uint32_t lastPulse = 0;
    static uint32_t lastBlink = 0;
    static bool blinkState = false;
    static uint8_t redLevel = 77;   // start at ~30% brightness
    static int8_t pulseStep = 4;

    const uint32_t blinkInterval = 150; // faster blink for active switches
    const uint32_t pulseInterval = 25;  // smoother pulsing

    uint32_t now = millis();

    // Toggle blink state for pressed switches
    if (now - lastBlink >= blinkInterval) {
        lastBlink = now;
        blinkState = !blinkState;
    }

    // Update pulsing brightness for idle switches
    if (now - lastPulse >= pulseInterval) {
        lastPulse = now;
        int16_t next = redLevel + pulseStep;
        if (next >= 255 || next <= 77) {
            pulseStep = -pulseStep;
            next = redLevel + pulseStep;
        }
        redLevel = static_cast<uint8_t>(next);
    }

    // Update each switch LED based on its current state
    for (int i = 0; i < numSwitches; ++i) {
        const auto& m = ledMap[i];
        bool switchPressed = false;

#ifdef DEBUG_HID
        if (i == 0) {
            switchPressed = digitalRead(PA0);
        }
#else
        switchPressed = IoExp.digitalRead(switchPins[i]);
#endif

        if (switchPressed) {
            if (m.hasGpioLed) {
                IoExp.digitalWrite(m.gpioPin, blinkState ? HIGH : LOW);
            }

            if (m.hasStripLed) {
                RgbwColor col = blinkState ? RgbwColor(255, 255, 255, 0)
                                          : RgbwColor(0, 0, 0, 0);
                for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; ++j) {
                    strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                }
            }

            if (i == 8) {
                uint8_t level = blinkState ? 255 : 0;
                rgbDriver.setChannelPWM(0, level);
                rgbDriver.setChannelPWM(1, level);
                rgbDriver.setChannelPWM(2, level);
            }

            if (i == 9) {
                uint8_t level = blinkState ? 255 : 0;
                rgbDriver.setChannelPWM(3, level);
                rgbDriver.setChannelPWM(4, level);
                rgbDriver.setChannelPWM(5, level);
            }
        } else {
            if (m.hasGpioLed) {
                IoExp.digitalWrite(m.gpioPin, HIGH);
            }

            if (m.hasStripLed) {
                RgbwColor col(redLevel, 0, 0, 0);
                for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; ++j) {
                    strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                }
            }

            if (i == 8) {
                rgbDriver.setChannelPWM(0, redLevel);
                rgbDriver.setChannelPWM(1, 0);
                rgbDriver.setChannelPWM(2, 0);
            }

            if (i == 9) {
                rgbDriver.setChannelPWM(3, redLevel);
                rgbDriver.setChannelPWM(4, 0);
                rgbDriver.setChannelPWM(5, 0);
            }
        }
    }

    strip.show();
}

void failSafe(int switchId){
    if(switchId == 8 || switchId == 9){
        setLed(switchId, {255, 0, 0, 0});
        setLed(switchId == 8 ? 6 : 7, {255, 0, 0, 0});  // Set SW7 or SW6 to red as well
    }
}