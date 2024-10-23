
#include "WiFiS3.h"
#include <UniversalTelegramBot.h>
#include "RTC.h"

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;
long lastTime = millis();
long timerDelay = 60 * 1000;

#define WIFI_SSID "genois"
#define WIFI_PASS "master13"
#define BOTtoken "8069531606:AAED72IvCjagstKsUoyzy9gmV4vImy-sISU" // your Bot Token (Get from Botfather)
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications"; // you can change this to whichever is your bot name

WiFiSSLClient client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  //  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // put your setup code here, to run once:
  connectwifi();
  bot.sendMessage(Mychat_id, "Bot connecté!", "Markdown");

  Serial.println("Patrol Mode Initiated...");
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

  if ((millis() - lastTime) > timerDelay && inches < 10 && inches > 0) {
    bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
    lastTime = millis();
  } 
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
