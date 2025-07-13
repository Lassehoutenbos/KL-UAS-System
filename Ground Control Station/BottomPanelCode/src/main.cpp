#include <Arduino.h>
#include <stdint.h>
#include <pins.h>
#include <Switches.h>
#include "HID-Project.h"
#include <ScreenPowerSwitch.h>

ScreenPowerSwitch powerDisplay;

void setup() {
  USB_Begin();  // Wrapper rond USBD_Init() + connect
  BootKeyboard.begin();  // Init HID class
  setupPins();
  powerDisplay.begin();
  // Initialize HID keyboard (this will be the primary USB function)
  Switches::begin();
  
  

}

void loop() {
  powerDisplay.update();
  Switches::update();  // Check all switches for state changes
  
  // Show appropriate screen based on lock state and confirmation
  if(Switches::isLocked) {
    powerDisplay.showLockScreen();  // Show lock icon when locked
  } else {
    // Case is unlocked - check if switches were confirmed during unlock
    if(!Switches::isConfirmed) {
      powerDisplay.showWarning();  // Show warning when switches were not confirmed safe during unlock
    } else {
      powerDisplay.showMainScreen();  // Show normal screen when switches were confirmed safe
    }
  }
  
  #ifdef DEBUG_LED
  startup();
  #endif

  // Optional: Add a small delay to prevent excessive polling
  delay(1);
}

