#pragma once

#include "leds.h"  // For rgbwValue struct

namespace Switches {
    extern rgbwValue onColorValue;   // Color when switch is pressed/active
    extern rgbwValue offColorValue;  // Color when switch is released/inactive
    
    // Global lock state variable
    extern bool isLocked;  // true = case is locked, false = case is unlocked
    extern bool isConfirmed;  // true = switches were confirmed low during unlock, false = need confirmation

    void begin();   // registreert alle knoppen
    void update();  // roept intern SwitchManager::updateAll() aan
    bool allSwitchesLow();  // checks if all switches are in LOW position (not pressed)
}
