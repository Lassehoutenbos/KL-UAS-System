#pragma once
#include <cstdint>

struct PCA9685 {
    uint16_t channels[16] = {0};
    void setChannelPWM(uint8_t ch, uint16_t val) { channels[ch] = val; }
};

extern PCA9685 rgbDriver;

