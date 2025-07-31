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


ScreenPowerSwitch powerDisplay;
TempSensors tempSensors;

static void uiTask(void *);
static void tempTask(void *);
static void blinkTask(void *);

// Set up peripherals, create tasks and start the scheduler.
void setup() {
  USB_Begin();  // Wrapper rond USBD_Init() + connect
BootKeyboard.begin();  // Init HID class
  setupPins();
  powerDisplay.begin();
  Switches::begin();
  tempSensors.begin();  // Initialize temperature sensors

  pinMode(PC13, OUTPUT);

  xTaskCreate(uiTask, "UI", 512, nullptr, 2, nullptr);
  xTaskCreate(tempTask, "TEMP", 512, nullptr, 1, nullptr);
  xTaskCreate(blinkTask, "LED", 256, nullptr, 1, nullptr);

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

// Dedicated LED update task
static void blinkTask(void *) {
  for (;;) {
    updateLeds();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


