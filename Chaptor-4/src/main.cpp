#include <Arduino.h>

const int triggerPin = 4;
const int echoPin = 5;
const int buzzerPin = 6;

void setup() {
  Serial.begin(9600);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2;

  Serial.print("Distance: ");
  Serial.println(distance);

  int interval = map(constrain(distance, 2, 100), 2, 100, 50, 1000);
  int pitch = map(constrain(distance, 2, 100), 2, 100, 1000, 500);
}