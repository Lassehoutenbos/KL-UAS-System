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
  Switches::update();  // Check all switches for state changes
  
  #ifdef DEBUG_LED
  powerDisplay.showBatWarningScreen();  // Show lock screen in debug mode
  #else
  // Show appropriate screen based on lock state and confirmation
  if(Switches::isLocked) {
    powerDisplay.showLockScreen();  // Show lock icon when locked
  } else {
    // Case is unlocked - check if switches were confirmed during unlock
    if(!Switches::isConfirmed) {
      powerDisplay.showWarningScreen();  // Show warning when switches were not confirmed safe during unlock
    } else {
      powerDisplay.showMainScreen();  // Show normal screen when switches were confirmed safe
    }
  }
#endif
#ifdef DEBUG_LED
  startup();
#endif

  powerDisplay.update();


  // Optional: Add a small delay to prevent excessive polling
  delay(1);
}

