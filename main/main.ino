//TEST CODE

#include "RTC.h"
#include <WiFiS3.h>

#include <SoftwareSerial.h>

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;


int testHour = 12, timeUpdateHour = 9;
bool testSent = false, timeUpdated = false;
bool sendMessage = true; // send a notification on powerup. 
unsigned long unixTime;
unsigned long unixTimeLastSend = 0;
unsigned long minutesSinceLastSend;
unsigned long minMinutesBetweenSends = 15;

WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

#define WIFI_SSID "genois"
#define WIFI_PASS "master13" // not needed with MAC address filtering


void setup() {
  
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  connectwifi();
  WiFi.disconnect();
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
  /*
  if (inches < 20 || inches > 500) {
    Serial.println("Intruder Detected!");
    Serial.println("Sending text Notification...");
    delay(5000);
    Serial.println("Patrol Mode Initiated...");
  }
  */

}

void connectwifi(){
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // WiFi.begin(WIFI_SSID) or WiFi.begin(WIFI_SSID, WIFI_PASS); depending on if password is necessary
  Serial.print("Connecting to Wi-Fi.");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.print(" Connected with IP: ");
  Serial.println(WiFi.localIP());
}
