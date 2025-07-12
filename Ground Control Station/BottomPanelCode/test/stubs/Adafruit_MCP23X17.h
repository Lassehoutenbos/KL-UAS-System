#ifndef ADAFRUIT_MCP23X17_H
#define ADAFRUIT_MCP23X17_H
class Adafruit_MCP23X17 {
public:
    void begin_I2C(int addr = 0){ (void)addr; }
    void pinMode(int pin, int mode){ (void)pin; (void)mode; }
    void digitalWrite(int pin, int val){ (void)pin; (void)val; }
    int digitalRead(int pin){ (void)pin; return 0; }
};
#endif
