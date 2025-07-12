#pragma once

#include "leds.h"  // For rgbwValue struct

namespace Switches {
    extern rgbwValue onColorValue;   // Color when switch is pressed/active
    extern rgbwValue offColorValue;  // Color when switch is released/inactive

    void begin();   // registreert alle knoppen
    void update();  // roept intern SwitchManager::updateAll() aan
    bool allSwitchesLow();  // checks if all switches are in LOW position (not pressed)
}
