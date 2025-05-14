
#include "WiFiS3.h"
#include <UniversalTelegramBot.h>

#define WIFI_SSID "genois"
#define WIFI_PASS "master13"
#define BOTtoken "8069531606:AAED72IvCjagstKsUoyzy9gmV4vImy-sISU" // your Bot Token (Get from Botfather)
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications"; // you can change this to whichever is your bot name

WiFiSSLClient client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  // put your setup code here, to run once:
  connectwifi();
  bot.sendMessage(Mychat_id, "Bot connecté!", "Markdown");
}

void loop() {
  // put your main code here, to run repeatedly:

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
