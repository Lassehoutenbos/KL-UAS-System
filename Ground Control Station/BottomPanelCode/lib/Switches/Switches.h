#pragma once

// High level API for all panel switches. Provides LED feedback and exposes
// the global locked/confirmed state used by the main firmware.

#include "leds.h"  // For rgbwValue struct

namespace Switches {
    extern rgbwValue onColorValue;   // Color when switch is pressed/active
    extern rgbwValue offColorValue;  // Color when switch is released/inactive
    extern rgbwValue allOffValue;
    
    // Global lock state variable
    extern bool isLocked;  // true = case is locked, false = case is unlocked
    extern bool isConfirmed;  // true = switches were confirmed low during unlock, false = need confirmation
    extern bool armPayload1;  // Payload arm state, initially not armed
    extern bool armPayload2;  // Payload arm state, initially not armed

    // Worklight (reading light) state variables
    enum WorklightState {
        WORKLIGHT_OFF = 0,
        WORKLIGHT_WHITE = 1,
        WORKLIGHT_RED = 2
    };
    extern WorklightState worklightState;
    extern unsigned long sw6PressStartTime;
    extern bool sw6WasPressed;
    extern uint8_t worklightBrightness;
    extern unsigned long lastDimUpdate;

    // Initialise all switch callbacks and LEDs.
    void begin();
    // Poll all switches for state changes.
    void update();
    // Utility to verify that every switch is in the low position.
    bool allSwitchesLow();
    // Reset LEDs to their default off state.
    void setLedDefault(bool off = false);
    // Update worklight based on current state
    void updateWorklight();
}
