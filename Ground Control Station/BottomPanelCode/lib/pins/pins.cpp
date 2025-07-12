// pins.cpp
#include <pins.h>

// SPI2 instantie (MOSI, MISO, SCK)
SPIClass SPI_2(PIN_SPI2_MOSI, PIN_SPI2_MISO, PIN_SPI2_SCK);

// IO Expander instance
Adafruit_MCP23X17 IoExp;


void setupPins() {

    Wire.begin();  // Start I2C bus

    // -------- SPI2 (PB13 = SCK, PB14 = MISO, PB15 = MOSI) --------
    SPI_2.begin();

    // --------- IO Expander setup --------- 
    IoExp.begin_I2C(0x20);
  
    // ------- Expander pin defs ---------
    
    IoExp.pinMode(PINIO_SW0, INPUT);
    IoExp.pinMode(PINIO_SW1, INPUT);
    IoExp.pinMode(PINIO_SW2, INPUT);
    IoExp.pinMode(PINIO_SW3, INPUT);
    IoExp.pinMode(PINIO_SW4, INPUT);
    IoExp.pinMode(PINIO_SW5, INPUT);
    IoExp.pinMode(PINIO_SW6, INPUT);
    IoExp.pinMode(PINIO_SW7, INPUT);
    IoExp.pinMode(PINIO_SW8, INPUT);
    IoExp.pinMode(PINIO_SW9, INPUT);

    IoExp.pinMode(PINIO_KEY, INPUT);

    IoExp.pinMode(PINIO_SW3LED, OUTPUT);
    IoExp.pinMode(PINIO_SW4LED, OUTPUT);
    IoExp.pinMode(PINIO_SW5LED, OUTPUT);
    IoExp.pinMode(PINIO_SW6LED, OUTPUT);
    IoExp.pinMode(PINIO_SW7LED, OUTPUT);



    // -------- STM32 pin defs --------
    #ifdef DEBUG_HID
    pinMode(PA0, INPUT_PULLUP);  // Debug switch for KEY_A
    pinMode(PC13, OUTPUT);
    #endif

    pinMode(PIN_RESET_PA8, OUTPUT);
    pinMode(PIN_CS_PA9, OUTPUT);
    pinMode(PIN_DC_PA10, OUTPUT);

    //pinMode(PIN_BAT_VIN, INPUT_ANALOG);
    pinMode(PIN_EXT_VIN, INPUT_ANALOG);

    pinMode(PIN_U6_OE, OUTPUT);
    digitalWrite(PIN_U6_OE, LOW);

    pinMode(PIN_U1_RESET, OUTPUT);
    digitalWrite(PIN_U1_RESET, HIGH);

    pinMode(PIN_U5_A, OUTPUT);

    pinMode(PIN_I2C_SCL, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_I2C_SDA, OUTPUT_OPEN_DRAIN);

}

