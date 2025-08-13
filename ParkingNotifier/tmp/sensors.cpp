#include "sensors.h"

void initSensors() {
    // Initialize distance sensor pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    Serial.println("Sensors initialized");
}

int get_distance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(trigPin, LOW);

    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);

    inches = (duration / 2) / 74;
    cm = (duration / 2) / 29;
    
    Serial.print("Distance: ");
    Serial.print(cm);
    Serial.println(" cm");
    
    return cm;
}

bool checkCarDetection() {
    cm = get_distance();
    unsigned long currentMillis = millis();
    
    // Check if car is detected (distance < 30cm and > 0cm for valid reading)
    bool carDetected = (cm < 30 && cm > 0 && currentMillis - lastEventTime > timerDelay);
    
    if (carDetected) {
        carPresent = true;
        lastEventTime = currentMillis;
        Serial.println("Car detected!");
        return true;
    } else {
        carPresent = false;
        return false;
    }
}

void measureData(int log_counter, bool from_command) {
    lightValue = analogRead(ldrPin); // Range: 0 (dark) to 1023 (bright)
    Serial.print("Light level: ");
    Serial.println(lightValue);
    logLightValue(log_counter, from_command);
}

int getLightLevel() {
    lightValue = analogRead(ldrPin);
    return lightValue;
}