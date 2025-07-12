#include <Arduino.h>
#include <stdint.h>
#include <pins.h>
#include <Switches.h>
#include "HID-Project.h"

void setup() {
  USB_Begin();  // Wrapper rond USBD_Init() + connect
  BootKeyboard.begin();  // Init HID class
  setupPins();
  // Initialize HID keyboard (this will be the primary USB function)
  Switches::begin();
  

}

void loop() {
  Switches::update();  // Check all switches for state changes
  
  #ifdef DEBUG_LED
  startup();
  #endif

  // Optional: Add a small delay to prevent excessive polling
  delay(1);
}

