#pragma once
#include <cmath>
#include <cstdint>

#define INPUT_ANALOG 0

extern uint16_t analogValues[32];
inline void pinMode(uint8_t, int) {}
inline uint16_t analogRead(uint8_t pin) { return analogValues[pin]; }

