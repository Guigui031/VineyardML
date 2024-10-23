
#include "RTC.h"

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;
long lastTime = millis();
long timerDelay = 60 * 1000;

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  //  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(1);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  inches = (duration / 2) / 74;
  Serial.println(inches);

  if inches < 10 && inches > 0) {
    Serial.println("Une auto a traversé!");
    lastTime = millis();
  } 
}
