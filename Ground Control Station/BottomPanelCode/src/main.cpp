#include <Arduino.h>
#include <stdint.h>
#include <pins.h>


void setup() {
  Serial.begin(115200);
  setupPins();


}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}