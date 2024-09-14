//TEST CODE


#include <SoftwareSerial.h>

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;

void setup() {
  
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(5000);
  Serial.println("Patrol Mode Initiated...");
}

void loop() {
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(1);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  inches = (duration / 2) / 74;
  Serial.println(inches);
  if (inches < 20 || inches > 500) {
    Serial.println("Intruder Detected!");
    Serial.println("Sending text Notification...");
    delay(5000);
    Serial.println("Patrol Mode Initiated...");
  }

}
