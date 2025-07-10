#include "leds.h"
#include "pins.h"

/* | Has IO | IO pin | Has Strip | Index start | Index end |*/
const SwitchLedMapping ledMap[numSwitches] = {
    {false, 0, true, 0, 3},
    {false, 0, true, 5, 8},
    {false, 0, true, 10, 14},
    {true, PINIO_SW3LED, true, 15, 18},
    {true, PINIO_SW4LED, true, 20, 23},
    {true, PINIO_SW5LED, true, 25, 28}
};



void setupLeds(){
    
    // Led driver setup
    rgbDriver.resetDevices();
    rgbDriver.init();
    Serial.println("DAC setup completed");

    //  SK6812 setup
    NeoPixelBus<NeoPixelColorFeature, NeoPixelMethod> strip(ledCount, PB3);
    strip.Begin();
    strip.Show();
    Serial.println("SK6812 Setup completed");
}

void setLed(int switchId, rgbwValue color){
    if(switchId < 0 || switchId >= numSwitches) return;

    const SwitchLedMapping& m = ledMap[switchId];

    if (switchId == 8) {
        rgbDriver.setChannelPWM(0, color.r);
        rgbDriver.setChannelPWM(1, color.g);
        rgbDriver.setChannelPWM(2, color.b);
        return;
    }

    if (switchId == 9) {
        rgbDriver.setChannelPWM(3, color.r);
        rgbDriver.setChannelPWM(4, color.g);
        rgbDriver.setChannelPWM(5, color.b);
        return;
    }
    


    if (m.hasGpioLed) {
        bool active = (color.r + color.g + color.b + color.w) > 0;
        IoExp.digitalWrite(m.gpioPin, active ? HIGH : LOW);
    }

    if (m.hasStripLed) {
        RgbwColor col(color.r, color.g, color.b, color.w);
        for (uint8_t i = m.stripStartIndex; i <= m.stripEndIndex; i++) {
          strip.SetPixelColor(i, col);
        }
        strip.Show();
      }

} 