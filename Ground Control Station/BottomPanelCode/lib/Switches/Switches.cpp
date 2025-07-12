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
            if(state) BootKeyboard.write(KEY_F13);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(0, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW1, [](bool state) {
            if(state) BootKeyboard.write(KEY_F14);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(1, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW2, [](bool state) {
            if(state) BootKeyboard.write(KEY_F15);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(2, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW3, [](bool state) {
            static bool toggleSW3 = false;  // ✅ FIXED: static keeps state between calls
            if (state) { // Active high: trigger when pressed
                toggleSW3 = !toggleSW3;

                // optioneel: stuur HID
                if (toggleSW3) BootKeyboard.press(KEY_F16);
                else BootKeyboard.release(KEY_F16);
            }
            // Set LED based on toggle state (not momentary state)
            setLed(3, toggleSW3 ? onColorValue : offColorValue);
        });


        SwitchHandler::addSwitch(PINIO_SW4, [](bool state) {
            if(state) BootKeyboard.write(KEY_F17);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(4, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW5, [](bool state) {
            if(state) BootKeyboard.write(KEY_F18);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(5, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW6, [](bool state) {
            if(state) BootKeyboard.write(KEY_F19);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(6, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW7, [](bool state) {
            if(state) BootKeyboard.write(KEY_F20);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(7, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW8, [](bool state) {
            if(state) BootKeyboard.write(KEY_F21);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(8, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_SW9, [](bool state) {
            if(state) BootKeyboard.write(KEY_F22);  // Active high: trigger when pressed
            // Set LED based on state
            setLed(9, state ? onColorValue : offColorValue);
        });

        SwitchHandler::addSwitch(PINIO_KEY, [](bool state) {
            if(state) {
                while(!allSwitchesLow()) {
                    switchPositionAlert();
                }

                if(allSwitchesLow()){
                    // code for boot pi and top lid

                }
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