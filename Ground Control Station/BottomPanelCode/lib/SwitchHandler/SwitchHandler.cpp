#include "SwitchHandler.h"
#include "pins.h"
#include <Arduino.h>
#include <vector>

Switch::Switch(int pin, Callback cb)
    : pin(pin), callback(cb)
{
    #ifdef DEBUG_HID
        // Debug mode: Use standard GPIO pins instead of IoExp
        pinMode(pin, INPUT_PULLUP);  // Use internal pull-up for debugging
        lastState = digitalRead(pin);
    #else
        // Normal mode: Use IoExp
        pinMode(pin, INPUT);
        lastState = IoExp.digitalRead(pin);
    #endif
}

void Switch::update() {
    #ifdef DEBUG_HID
        // Debug mode: Use standard GPIO pins
        bool currentState = digitalRead(pin);
    #else
        // Normal mode: Use IoExp
        bool currentState = IoExp.digitalRead(pin);
    #endif
    
    if (currentState != lastState) {
        lastState = currentState;
        callback(currentState);
    }
}

// Static switch management
namespace SwitchHandler {
    static std::vector<Switch> switches;
    
    void addSwitch(int pin, Switch::Callback callback) {
        switches.emplace_back(pin, callback);
    }
    
    void updateAll() {
        for (auto& sw : switches) {
            sw.update();
        }
    }
}
