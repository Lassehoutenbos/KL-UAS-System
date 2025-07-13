#include "SwitchHandler.h"
#include "pins.h"
#include <Arduino.h>
#include <vector>

Switch::Switch(int pin, Callback cb)
    : pin(pin), callback(cb)
{
    #if defined(DEBUG_HID) || defined(DEBUG_SCREENTEST)
        // Debug/Screen test modes: read from MCU pin instead of IO expander
        lastState = digitalRead(pin);
    #else
        // Normal mode: read from IO expander
        lastState = IoExp.digitalRead(pin);
    #endif

}

void Switch::update() {
    #if defined(DEBUG_HID) || defined(DEBUG_SCREENTEST)
        // Debug/Screen test modes: read from MCU pin
        bool currentState = digitalRead(pin);
    #else
        // Normal mode: read from IO expander
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
