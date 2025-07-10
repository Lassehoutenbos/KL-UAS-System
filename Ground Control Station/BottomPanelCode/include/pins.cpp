// pins.cpp
#include "pins.h"

// SPI2 instantie (MOSI, MISO, SCK)





void setupPins() {

    Wire.begin();  // Start I2C bus

    // -------- SPI2 (PB13 = SCK, PB14 = MISO, PB15 = MOSI) --------
    SPI_2.begin();

    Serial.println("SPI & I2C Enabled");

    // --------- IO Expander setup --------- 
    IoExp.begin_I2C(0x20);
  
    // ------- Expander pin defs ---------
    
    for (int i = 2; i <= 15; i++){
        IoExp.pinMode(i, INPUT);
    }
    IoExp.pinMode(PINIO_SW3, OUTPUT);
    IoExp.pinMode(PINIO_SW4, OUTPUT);
    IoExp.pinMode(PINIO_SW5, OUTPUT);  

    // -------- STM32 pin defs --------


    pinMode(PIN_BAT_VIN, INPUT_ANALOG);
    pinMode(PIN_EXT_VIN, INPUT_ANALOG);

    pinMode(PIN_U6_OE, OUTPUT);
    digitalWrite(PIN_U6_OE, LOW);

    pinMode(PIN_U1_RESET, OUTPUT);
    digitalWrite(PIN_U1_RESET, HIGH);

    pinMode(PIN_U5_A, OUTPUT);

    pinMode(PIN_I2C_SCL, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_I2C_SDA, OUTPUT_OPEN_DRAIN);

    Serial.println("Pin setup complete");


}

