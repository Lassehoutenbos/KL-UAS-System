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

    // Initialise all switch callbacks and LEDs.
    void begin();
    // Poll all switches for state changes.
    void update();
    // Utility to verify that every switch is in the low position.
    bool allSwitchesLow();
    // Reset LEDs to their default off state.
    void setLedDefault();
}
