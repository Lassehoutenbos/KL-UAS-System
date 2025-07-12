#ifndef ARDUINO_H
#define ARDUINO_H

#include <map>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 2
#define OUTPUT 1

inline std::map<int,int>& _pinState() {
    static std::map<int,int> state;
    return state;
}

inline void pinMode(int pin, int mode) { (void)pin; (void)mode; }

inline int digitalRead(int pin) {
    auto& s = _pinState();
    auto it = s.find(pin);
    if(it == s.end()) return LOW;
    return it->second;
}

inline void digitalWrite(int pin, int val) {
    _pinState()[pin] = val;
}

inline void setPinState(int pin, int val) { _pinState()[pin] = val; }

#endif // ARDUINO_H
