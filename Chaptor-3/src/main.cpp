#include <Arduino.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(A1));
  delay(1000);
}