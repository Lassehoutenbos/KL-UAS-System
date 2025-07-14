#include <Arduino.h>
#include <stdint.h>
#include <pins.h>
#include <Switches.h>
#include "HID-Project.h"
#include <ScreenPowerSwitch.h>
#include <TempSensors.h>

ScreenPowerSwitch powerDisplay;
TempSensors tempSensors;

void setup() {
  USB_Begin();  // Wrapper rond USBD_Init() + connect
  BootKeyboard.begin();  // Init HID class
  setupPins();
  powerDisplay.begin();
  Switches::begin();
  tempSensors.begin();  // Initialize temperature sensors
}

void loop() {
  digitalWrite(PC13, HIGH);

  #ifdef DEBUG_LED
    powerDisplay.showBatWarningScreen();  // Show lock screen in debug mode
  #else
  // Show appropriate screen based on lock state and confirmation
  

  if(Switches::isLocked) {
    powerDisplay.showLockScreen();  // Show lock icon when locked
  } else {
    // Case is unlocked - check if switches were confirmed during unlock
    if(!Switches::isConfirmed) {
      switchPositionAlert();
      powerDisplay.showWarningScreen();  // Show warning when switches were not confirmed safe during unlock
    } else {
      Switches::setLedDefault();
      powerDisplay.showMainScreen();  // Show normal screen when switches were confirmed safe
    }
  }
  #endif
  #ifdef DEBUG_LED
    startup();
  #endif
  Switches::update();  // Check all switches for state changes
  powerDisplay.update();
  delay(5);
}

