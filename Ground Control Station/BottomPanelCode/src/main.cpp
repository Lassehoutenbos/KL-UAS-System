#include <Arduino.h>
// Main firmware entry for the bottom panel. This file initialises all
// subsystems and drives the main update loop used on the STM32 board.
#include <stdint.h>
#include <pins.h>
#include <Switches.h>
#include "HID-Project.h"
#include <ScreenPowerSwitch.h>
#include <TempSensors.h>
#include <STM32FreeRTOS.h>
#include <WarningPanel.h>
#include <WarningCommunication.h>


ScreenPowerSwitch powerDisplay;
TempSensors tempSensors;
WarningCommunication warningComm(&warningPanel);

static void uiTask(void *);
static void tempTask(void *);
static void warningTask(void *);
static void blinkTask(void *);

// Set up peripherals, create tasks and start the scheduler.
void setup() {
  USB_Begin();  // Wrapper rond USBD_Init() + connect
BootKeyboard.begin();  // Init HID class
  setupPins();
  setupLeds();          // Initialize LED system first
  powerDisplay.begin();
  Switches::begin();
  tempSensors.begin();  // Initialize temperature sensors
  warningPanel.begin(); // Initialize warning panel (uses existing LED strip)
  warningComm.begin();  // Initialize warning communication

  pinMode(PC13, OUTPUT);

  xTaskCreate(uiTask, "UI", 512, nullptr, 2, nullptr);
  xTaskCreate(tempTask, "TEMP", 512, nullptr, 1, nullptr);
  xTaskCreate(warningTask, "WARNING", 512, nullptr, 1, nullptr);

  vTaskStartScheduler();
}

// Unused. Execution happens in FreeRTOS tasks.
void loop() {}

// UI task handles screen rendering and switch state.
static void uiTask(void *) {
  for (;;) {

#ifdef DEBUG_LED
    powerDisplay.showBatWarningScreen();
#else
    if (Switches::isLocked) {
      powerDisplay.showLockScreen();
    } else {
      if (!Switches::isConfirmed) {
        switchPositionAlert();
        powerDisplay.showWarningScreen();
      } else {
        Switches::setLedDefault();
        powerDisplay.showMainScreen();
      }
    }
#endif

#ifdef DEBUG_LED
    startup();
#endif

    Switches::update();
    powerDisplay.update();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Periodic temperature sensor task.
static void tempTask(void *) {
  for (;;) {
    tempSensors.update();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Warning panel update task - handles LED updates and animations
static void warningTask(void *) {
  // Show startup animation once
  warningPanel.showStartupAnimation();
  
  for (;;) {
    // Update warning panel (handles flashing, animations, etc.)
    // Note: This only updates the warning LEDs, not the entire strip
    warningPanel.update();
    
    // Process warning communication messages
    warningComm.update();
    
    // Check for simple serial commands for warning panel control
    if (Serial.available()) {
      String command = Serial.readStringUntil('\n');
      if (command.startsWith("warn:")) {
        // Commands prefixed with "warn:" are for warning panel
        command = command.substring(5); // Remove "warn:" prefix
        warningPanel.processSerialCommand(command);
      }
    }
    
    // Example: Set warning states based on system status
    // Integrate with temperature sensors
    float maxTemp = 0.0f;
    for (uint8_t i = 0; i < 4; i++) {
      float temp = tempSensors.getTemperatureC(i);
      if (!isnan(temp) && temp > maxTemp) {
        maxTemp = temp;
      }
    }
    
    // Set temperature warning based on max temperature
    if (maxTemp > 60.0f) {
      warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);
    } else if (maxTemp > 45.0f) {
      warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_WARNING, false);
    } else if (maxTemp > 0.0f) {
      warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_NORMAL, false);
    } else {
      warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_OFF, false);
    }
    
    // Set connection warning based on switch states
    if (Switches::isLocked) {
      warningPanel.setWarning(LOCK_WARNING, WARNING_CRITICAL, false);
    } else {
      warningPanel.setWarning(LOCK_WARNING, WARNING_OFF, false);
    }
    
    if (!Switches::isConfirmed) {
      warningPanel.setDualWarning(WARNING_WARNING, true);
    } else {
      warningPanel.setDualWarning(WARNING_OFF, false);
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz update rate
  }
}


