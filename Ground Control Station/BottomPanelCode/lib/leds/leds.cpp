#include "leds.h"
#include "pins.h"
#include "Blinker.h"

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
    {true, PINIO_SW6LED, true, 28, 38},
    {true, PINIO_SW7LED, true, 28, 38},
    {false, 0, false, 0, 0},  // PWM RGB
    {false, 0, false, 0, 0},  // PWM RGB
};

Blinker blinkerSW6(PINIO_SW6LED, 500);
Blinker blinkerSW7(PINIO_SW7LED, 500);

void setupLeds(){
    
    // Led driver setup
    rgbDriver.resetDevices();
    rgbDriver.init();

    //  SK6812 setup
    strip.begin();
    strip.show();

    // Blinker setup
    blinkerSW6.begin();
    blinkerSW7.begin();

}

void setLed(int switchId, rgbwValue color){
    if(switchId < 0 || switchId >= numSwitches) return;

    const SwitchLedMapping& m = ledMap[switchId];

    if (switchId == 6) {
        if (color.r + color.g + color.b + color.w > 0)
        {
            blinkerSW6.start();
        } 
        else {
            blinkerSW6.stop();
        }
    }

    if (switchId == 7) {
        if (color.r + color.g + color.b + color.w > 0)
        {
        blinkerSW7.start();
        } 
        else {
            blinkerSW7.stop();
        }
    }

    if (switchId == 8) {
        rgbDriver.setChannelPWM(0, color.r);
        rgbDriver.setChannelPWM(1, color.g);
        rgbDriver.setChannelPWM(2, color.b);
        return;
    }

    if (switchId == 9) {
        rgbDriver.setChannelPWM(3, color.r);
        rgbDriver.setChannelPWM(4, color.g);
        rgbDriver.setChannelPWM(5, color.b);
        return;
    }

    


    if (m.hasGpioLed) {
        bool active = (color.r + color.g + color.b + color.w) > 0;
        IoExp.digitalWrite(m.gpioPin, active ? HIGH : LOW);
    }

    if (m.hasStripLed) {
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

        delay(interval);  // korte pauze voor de animatie
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

void switchPositionAlert() {
    static uint32_t lastUpdate = 0;
    static bool blinkState = false;
    static uint32_t lastBlink = 0;
    
    const uint32_t blinkInterval = 300;  // Blink interval for pressed switches
    const uint32_t pulseInterval = 50;   // Pulse update interval for unpressed switches
    
    uint32_t currentTime = millis();
    
    // Update blink state for pressed switches
    if (currentTime - lastBlink >= blinkInterval) {
        blinkState = !blinkState;
        lastBlink = currentTime;
    }
    
    // Update pulse brightness for unpressed switches
    if (currentTime - lastUpdate >= pulseInterval) {
        lastUpdate = currentTime;
        
        // Calculate pulsing red brightness (sine wave)
        float t = (currentTime % 2000) / 2000.0f;  // 2 second cycle
        float brightness = 0.3f + 0.7f * (0.5f * (1.0f + sin(t * 2.0f * PI)));  // 30%-100% brightness
        uint8_t redLevel = brightness * 255;
        
        // Check each switch and set appropriate LED behavior
        for (int i = 0; i < numSwitches; ++i) {
            const auto& m = ledMap[i];
            bool switchPressed = false;
            
            #ifdef DEBUG_HID
            // Debug mode: only check PA0 for switch 0
            if (i == 0) {
                switchPressed = digitalRead(PA0);
            }
            #else
            // Normal mode: check corresponding IoExp pins
            switch(i) {
                #ifdef DEBUG_SCREENTEST
                case 0: switchPressed = digitalRead(PA0); break;
                #else
                case 0: switchPressed = IoExp.digitalRead(PINIO_SW0); break;
                #endif
                case 1: switchPressed = IoExp.digitalRead(PINIO_SW1); break;
                case 2: switchPressed = IoExp.digitalRead(PINIO_SW2); break;
                case 3: switchPressed = IoExp.digitalRead(PINIO_SW3); break;
                case 4: switchPressed = IoExp.digitalRead(PINIO_SW4); break;
                case 5: switchPressed = IoExp.digitalRead(PINIO_SW5); break;
                case 6: switchPressed = IoExp.digitalRead(PINIO_SW6); break;
                case 7: switchPressed = IoExp.digitalRead(PINIO_SW7); break;
                case 8: switchPressed = IoExp.digitalRead(PINIO_SW8); break;
                case 9: switchPressed = IoExp.digitalRead(PINIO_SW9); break;
            }
            #endif
            
            if (switchPressed) {
                // Switch is pressed - blink the LED
                if (m.hasGpioLed) {
                    IoExp.digitalWrite(m.gpioPin, blinkState ? HIGH : LOW);
                }
                
                if (m.hasStripLed) {
                    // Blink white for pressed switches
                    RgbwColor col = blinkState ? RgbwColor(255, 255, 255, 0)
                                              : RgbwColor(0, 0, 0, 0);
                    for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; j++) {
                        strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                    }
                }
                
                // Handle special cases for switches 8 and 9 (DAC RGB)
                if (i == 8) {
                    uint8_t blinkLevel = blinkState ? 255 : 0;
                    rgbDriver.setChannelPWM(0, blinkLevel);  // Red
                    rgbDriver.setChannelPWM(1, blinkLevel);  // Green  
                    rgbDriver.setChannelPWM(2, blinkLevel);  // Blue
                }
                
                if (i == 9) {
                    uint8_t blinkLevel = blinkState ? 255 : 0;
                    rgbDriver.setChannelPWM(3, blinkLevel);  // Red
                    rgbDriver.setChannelPWM(4, blinkLevel);  // Green
                    rgbDriver.setChannelPWM(5, blinkLevel);  // Blue
                }
                
            } else {
                // Switch is not pressed - red pulsing
                if (m.hasGpioLed) {
                    // GPIO LEDs can't pulse, so just turn them on with red indication
                    IoExp.digitalWrite(m.gpioPin, HIGH);
                }
                
                if (m.hasStripLed) {
                    // Red pulsing for LED strips
                    RgbwColor col(redLevel, 0, 0, 0);  // Red only
                    for (uint8_t j = m.stripStartIndex; j <= m.stripEndIndex; j++) {
                        strip.setPixelColor(j, col.r, col.g, col.b, col.w);
                    }
                }
                
                // Handle special cases for switches 8 and 9 (DAC RGB) - red pulsing
                if (i == 8) {
                    rgbDriver.setChannelPWM(0, redLevel);  // Red pulsing
                    rgbDriver.setChannelPWM(1, 0);         // Green off
                    rgbDriver.setChannelPWM(2, 0);         // Blue off
                }
                
                if (i == 9) {
                    rgbDriver.setChannelPWM(3, redLevel);  // Red pulsing
                    rgbDriver.setChannelPWM(4, 0);         // Green off
                    rgbDriver.setChannelPWM(5, 0);         // Blue off
                }
            }
        }
        
        // Update the LED strip
        strip.show();
    }
}
