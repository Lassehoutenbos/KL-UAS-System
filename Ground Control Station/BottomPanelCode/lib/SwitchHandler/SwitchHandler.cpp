#include "SwitchHandler.h"
#include "pins.h"

// Implementation of the SwitchHandler helper classes.
#include <Arduino.h>
#include <vector>

// Create a Switch object bound to an IO pin.
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

// Poll the switch and notify when the state changes.
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
    
    // Register a new switch with the handler.
    void addSwitch(int pin, Switch::Callback callback) {
        switches.emplace_back(pin, callback);
    }
    
    // Update all registered switches.
    void updateAll() {
        for (auto& sw : switches) {
            sw.update();
        }
    }
}
