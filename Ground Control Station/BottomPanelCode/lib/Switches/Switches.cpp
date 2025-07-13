#include "Switches.h"
#include "SwitchHandler.h"
#include "HID-Project.h"
#include "pins.h"
#include <Arduino.h>
#include "leds.h"

namespace Switches {
    
    // Define default colors for switch states using rgbwValue (avoiding NeoPixelBus issues)
    rgbwValue onColorValue = {0, 255, 0, 0};   // Green when pressed/active
    rgbwValue offColorValue = {255, 0, 0, 0};  // Red when released/inactive
    
    // Global lock state variable - initially locked (key starts in LOW position)
    bool isLocked = true;
    
    // Global confirmation state - initially not confirmed
    bool isConfirmed = false;
    

    // Dummy RgbwColor objects for compatibility (not actually used)
    RgbwColor onColor(0, 255, 0, 0);
    RgbwColor offColor(255, 0, 0, 0);

    void begin() {
        

        #ifndef DEBUG_LED
        #ifdef DEBUG_HID
        SwitchHandler::addSwitch(PA0, [](bool state) {
            if(state) {
                BootKeyboard.print("Nee");  // Active high: trigger when pressed
                digitalWrite(PA13, HIGH);
            }
            // Set LED based on state
            setLed(0, state ? onColorValue : offColorValue);
        });
        #else


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
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F19);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(6, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW7, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F20);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(7, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW8, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F21);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(8, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW9, [](bool state) {
            if(state && !isLocked && isConfirmed) BootKeyboard.write(KEY_F22);  // Only respond when unlocked and confirmed
            // Set LED based on state only when unlocked
            if(!isLocked) setLed(9, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_KEY, [](bool state) {
            if(state) {
                // Key is HIGH - unlock the case
                isLocked = false;
                // Check if all switches are in low position to confirm safe operation
                if(allSwitchesLow()) {
                    isConfirmed = true;  // Switches confirmed safe, enable full functionality
                } else {
                    isConfirmed = false; // Switches not safe, keep functionality disabled
                }
            } else {
                // Key is LOW - lock the case immediately
                isLocked = true;
                isConfirmed = false;  // Reset confirmation when locking
            }
        });
        
        #endif
        #endif
    }

    void update() {
        SwitchHandler::updateAll();
    }

    bool allSwitchesLow() {
        #ifdef DEBUG_HID
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

}