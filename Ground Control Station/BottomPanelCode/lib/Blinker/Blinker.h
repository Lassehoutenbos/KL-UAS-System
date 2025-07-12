#ifndef BLINKER_H
#define BLINKER_H

#include <Arduino.h>

class Blinker {
public:
  Blinker(uint8_t pin, unsigned long interval);

  void begin(bool startState = LOW);
  void update();
  void setInterval(unsigned long interval);
  void stop();
  void start();

private:
  uint8_t _pin;
  unsigned long _interval;
  unsigned long _lastToggle;
  bool _state;
  bool _active;
};

#endif
