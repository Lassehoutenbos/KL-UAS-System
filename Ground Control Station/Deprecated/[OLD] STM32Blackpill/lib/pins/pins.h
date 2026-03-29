// pins.h - central pin definitions and helper declarations
#ifndef PINS_H
#define PINS_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>


// -------- ANALOG INPUTS --------

#if !defined(DEBUG_SCREENTEST) && !defined(DEBUG_HID)
constexpr uint8_t PIN_BAT_VIN     = PA0;  // ADC0 BAT VIN
constexpr uint8_t PIN_EXT_VIN     = PA1;  // ADC1 EXT VIN
#endif
// -------- UNUSED ANALOG/GPIO PINS --------
constexpr uint8_t PIN_UNUSED_PA2  = PA2;
constexpr uint8_t PIN_UNUSED_PA3  = PA3;
constexpr uint8_t PIN_UNUSED_PA4  = PA4;

constexpr uint8_t PIN_UNUSED_PA11 = PA11;
constexpr uint8_t PIN_UNUSED_PA12 = PA12;
constexpr uint8_t PIN_UNUSED_PA15 = PA15;

// -------- GPIO OUTPUTS --------
constexpr uint8_t PIN_ICLEDS       = PB3;   // To level shifter for IC LEDs
constexpr uint8_t PIN_RESET_PA8  = PA8;
constexpr uint8_t PIN_CS_PA9  = PA9;
constexpr uint8_t PIN_DC_PA10 = PA10;


// -------- UNUSED GPIO PINS --------
constexpr uint8_t PIN_UNUSED_PB2  = PB2;
constexpr uint8_t PIN_UNUSED_PB4  = PB4;
constexpr uint8_t PIN_UNUSED_PB5  = PB5;
constexpr uint8_t PIN_UNUSED_PB6  = PB6;
constexpr uint8_t PIN_UNUSED_PB9  = PB9;
constexpr uint8_t PIN_UNUSED_PB10 = PB10;
constexpr uint8_t PIN_UNUSED_PB12 = PB12;

// -------- I2C1 --------
constexpr uint8_t PIN_I2C_SCL     = PB7;   // SCL
constexpr uint8_t PIN_I2C_SDA     = PB8;   // SDA

// -------- SPI2 --------
constexpr uint8_t PIN_SPI2_SCK    = PB13;
constexpr uint8_t PIN_SPI2_MISO   = PB14;
constexpr uint8_t PIN_SPI2_MOSI   = PB15;

// -------- UNUSED PC PINS --------
constexpr uint8_t PIN_UNUSED_PC13 = PC13;
constexpr uint8_t PIN_UNUSED_PC14 = PC14;
constexpr uint8_t PIN_UNUSED_PC15 = PC15;

// -------- IO Expander Led pins --------
constexpr uint8_t PINIO_SW3LED = 0;
constexpr uint8_t PINIO_SW4LED = 1;
constexpr uint8_t PINIO_SW5LED = 2;
constexpr uint8_t PINIO_SW6LED = 3;
constexpr uint8_t PINIO_SW7LED = 4;

// -------- IO Expander Switch pins --------
constexpr uint8_t PINIO_SW0 = 8;
constexpr uint8_t PINIO_SW1 = 9;
constexpr uint8_t PINIO_SW2 = 10;

constexpr uint8_t PINIO_SW3 = 11;
constexpr uint8_t PINIO_SW4 = 12;
constexpr uint8_t PINIO_SW5 = 13;

constexpr uint8_t PINIO_SW6 = 14;
constexpr uint8_t PINIO_SW7 = 15;

constexpr uint8_t PINIO_SW8 = 6;
constexpr uint8_t PINIO_SW9 = 7;

// ----------- Key Pin --------------

constexpr uint8_t PINIO_KEY = 5;

// ----------- Temp Sensor Pins -----------
constexpr uint8_t PIN_SENS2 = PB0;  
constexpr uint8_t PIN_SENS3 = PB1;  
constexpr uint8_t PIN_SENS4  = PA6;
constexpr uint8_t PIN_SENS5  = PA7;

// ----------- Fan PWM channels on PCA9685 -----------
constexpr uint8_t FAN1_PWM_CH = 6;  // shares PCA9685 with analog RGB LEDs
constexpr uint8_t FAN2_PWM_CH = 7;

extern SPIClass SPI_2;
extern Adafruit_MCP23X17 IoExp;


// Configure all MCU and IO expander pins.
void setupPins();


#endif // PINS_H