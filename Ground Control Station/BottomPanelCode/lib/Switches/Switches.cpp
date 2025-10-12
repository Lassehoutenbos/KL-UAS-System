#include "Switches.h"
#include "SwitchHandler.h"
#include "HID-Project.h"
#include "pins.h"
#include <Arduino.h>
#include "leds.h"
#include "WarningPanel.h"

// High level logic for reading all panel switches and providing HID outputs
// as well as LED feedback.

namespace Switches {
    
    // Define default colors for switch states using rgbwValue (avoiding NeoPixelBus issues)
    rgbwValue onColorValue = {0, 255, 0, 0};   // Green when pressed/active
    rgbwValue offColorValue = {255, 0, 0, 0};  // Red when released/inactive
    rgbwValue allOffValue = {0,0,0,0};

    // Global lock state variable - initially locked (key starts in LOW position)
    bool isLocked = true;
    
    // Global confirmation state - initially not confirmed
    bool isConfirmed = true;
    
    bool armPayload1 = false;  // Payload arm state, initially not armed
    bool armPayload2 = false;  // Payload arm state, initially not armed

    // Worklight (reading light) state tracking
    WorklightState worklightState = WORKLIGHT_OFF;
    unsigned long sw6PressStartTime = 0;
    bool sw6WasPressed = false;
    uint8_t worklightBrightness = 255;  // Full brightness by default
    unsigned long lastDimUpdate = 0;

    // Configure all switch callbacks and initial LED states.
    void begin() {
        

       
        #ifdef DEBUG_HID
        SwitchHandler::addSwitch(PA0, [](bool state) {
            bool pressed = !state; // PA0 uses pull-up, LOW means pressed
            if(pressed) {
                BootKeyboard.print("Nee");  // Trigger when actually pressed
                digitalWrite(PC13, HIGH);
            } else {
                digitalWrite(PC13, LOW);
            }
            // Set LED based on actual press state
            setLed(0, pressed ? onColorValue : offColorValue);
        });
        #else

        // Initialize all switches with their respective pins and callbacks
        #if !defined(DEBUG_HID) && !defined(DEBUG_SCREENTEST) 

        // Voeg hier alle knoppen toe
        SwitchHandler::addSwitch(PINIO_SW0, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F13);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(0, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW1, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F14);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(1, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW2, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F15);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(2, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW3, [](bool state) {
            static bool toggleSW3 = false;  // ✅ FIXED: static keeps state between calls
            if (state && !isLocked && isConfirmed) { // Only respond when unlocked and confirmed
                toggleSW3 = !toggleSW3;

                // optioneel: stuur HID
                if (toggleSW3) BootKeyboard.press(KEY_F16);
                else BootKeyboard.release(KEY_F16);
            }
            // Set LED based on toggle state only when unlocked
            if(!isLocked) setLed(3, toggleSW3 ? onColorValue : offColorValue);
        });


        SwitchHandler::addSwitch(PINIO_SW4, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F17);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(4, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW5, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F18);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(5, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW6, [](bool state) {
            // SW6 controls the worklight (reading light) and also serves as payload arm
            unsigned long currentTime = millis();
            
            if (state) {
                // Button pressed
                if (!sw6WasPressed) {
                    // Record the start time of the press
                    sw6PressStartTime = currentTime;
                    sw6WasPressed = true;
                }
                
                // Check for hold (dimming mode) - activate after 500ms hold
                if (currentTime - sw6PressStartTime > 500 && worklightState != WORKLIGHT_OFF) {
                    // Dimming mode - update brightness periodically
                    if (currentTime - lastDimUpdate > 50) {  // Update every 50ms
                        lastDimUpdate = currentTime;
                        
                        // Decrease brightness by 5 each update (wraps around at 0)
                        if (worklightBrightness > 10) {
                            worklightBrightness -= 5;
                        } else {
                            worklightBrightness = 255;  // Wrap to full brightness
                        }
                        
                        updateWorklight();
                    }
                }
                
                // Still send HID command for payload arming if needed
                if(!isLocked && isConfirmed) BootKeyboard.write(KEY_F19);
                armPayload1 = state;
            } else {
                // Button released
                if (sw6WasPressed) {
                    unsigned long pressDuration = currentTime - sw6PressStartTime;
                    
                    // Only cycle state if it was a short press (< 500ms)
                    if (pressDuration < 500) {
                        // Cycle through states: OFF -> WHITE -> RED -> OFF
                        switch (worklightState) {
                            case WORKLIGHT_OFF:
                                worklightState = WORKLIGHT_WHITE;
                                worklightBrightness = 255;  // Reset to full brightness
                                break;
                            case WORKLIGHT_WHITE:
                                worklightState = WORKLIGHT_RED;
                                worklightBrightness = 255;  // Reset to full brightness
                                break;
                            case WORKLIGHT_RED:
                                worklightState = WORKLIGHT_OFF;
                                worklightBrightness = 255;  // Reset to full brightness
                                break;
                        }
                        
                        updateWorklight();
                    }
                    
                    sw6WasPressed = false;
                }
                
                armPayload1 = false;
            }
            
            // Set LED based on state only when unlocked
            if(!isLocked){
                setLed(6, state ? onColorValue : offColorValue);
                setLed(8, state ? onColorValue : offColorValue, true);  // Also set SW8 LED to match SW6
            }
        });

        SwitchHandler::addSwitch(PINIO_SW7, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F20);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            armPayload2 = state;  // Update payload arm state
            if(!isLocked){ 
                setLed(7, state ? onColorValue : offColorValue);
                setLed(9, state ? onColorValue : offColorValue, true);  // Also set SW9 LED to match SW7
            }
        });

        SwitchHandler::addSwitch(PINIO_SW8, [](bool state) {
            if(state && !isLocked && isConfirmed){ 
                if (armPayload1){
                    BootKeyboard.write(KEY_F21);  // Only respond when unlocked and confirmed
                    if(!isLocked) setLed(8, state ? onColorValue : offColorValue);
                }
                else{
                failSafe(8);  // Trigger fail-safe if not armed
                }
            }
        });

        SwitchHandler::addSwitch(PINIO_SW9, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F22);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(9, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_KEY, [](bool state) {
            if(state) {
                // Key is HIGH/pressed - unlock the case
                isLocked = false;
                // Check if all switches are in low position to confirm safe operation
                if(allSwitchesLow()) {
                    isConfirmed = true;  // Switches confirmed safe, enable full functionality
                } else {
                    isConfirmed = false; // Switches not safe, keep functionality disabled
                }
            } else {
                // Key is LOW/unpressed - lock the case immediately
                digitalWrite(PC13, LOW);
                isLocked = true;
                isConfirmed = false;  // Reset confirmation when locking
                
                // Turn off worklight when locking
                worklightState = WORKLIGHT_OFF;
                updateWorklight();
            }
        });
        #endif
        #ifdef DEBUG_SCREENTEST
        SwitchHandler::addSwitch(PA1, [](bool state) {
            if(!state) {
                // Key is HIGH/pressed - unlock the case
                isLocked = false;
                // Check if all switches are in low position to confirm safe operation
                if(allSwitchesLow()) {

                } else {
                }
            } else {
                // Key is LOW/unpressed - lock the case immediately
                digitalWrite(PC13, LOW);
                isLocked = true;
                isConfirmed = false;  // Reset confirmation when locking
                for(int i = 0; i < 10; i++) {
                   setLed(i, allOffValue);
                }
                
                // Turn off worklight when locking
                worklightState = WORKLIGHT_OFF;
                updateWorklight();
            }
        });
        #endif

        #endif
        
    }

    // Poll switches and maintain confirmation state.
    void update() {
        SwitchHandler::updateAll();
            if(allSwitchesLow()) {
                isConfirmed = true;  // Switches confirmed safe, enable full functionality
            }
    }

    // Reset every switch LED to the off colour.
    void setLedDefault(bool off){
        // Initialize all LEDs to off state first
        for(int i = 0; i < 10; i++) {
            setLed(i, off ? allOffValue : offColorValue);  // Use allOffValue for complete off state
        }

    }

    // Check whether all switches are in the low (off) position.
    bool allSwitchesLow() {
        #ifdef DEBUG_SCREENTEST
        // Debug mode: Check PA0
        return !digitalRead(PA0);  // Return true if switch is LOW (not pressed)
        #else
        // Normal mode: Check all switches via IoExp
        bool allLow = true;
        // Check switches SW0 through SW9
        allLow &= !IoExp.digitalRead(PINIO_SW0);
        allLow &= !IoExp.digitalRead(PINIO_SW1);
        allLow &= !IoExp.digitalRead(PINIO_SW2);
        allLow &= !IoExp.digitalRead(PINIO_SW3);
        allLow &= !IoExp.digitalRead(PINIO_SW4);
        allLow &= !IoExp.digitalRead(PINIO_SW5);
        allLow &= !IoExp.digitalRead(PINIO_SW6);
        allLow &= !IoExp.digitalRead(PINIO_SW7);
        allLow &= !IoExp.digitalRead(PINIO_SW8);
        allLow &= !IoExp.digitalRead(PINIO_SW9);
        return allLow;
        #endif
    }

    // Update worklight based on current state
    void updateWorklight() {
        uint32_t color = 0;
        bool enabled = false;
        
        switch (worklightState) {
            case WORKLIGHT_OFF:
                enabled = false;
                break;
            case WORKLIGHT_WHITE:
                enabled = true;
                color = strip.Color(255, 255, 255);  // White
                break;
            case WORKLIGHT_RED:
                enabled = true;
                color = strip.Color(255, 0, 0);  // Red
                break;
        }
        
        // Update worklight through WarningPanel
        warningPanel.setWorklight(enabled, 48, 70, worklightBrightness, color, false);
    }

}