
#include "WiFiS3.h"


#define WIFI_SSID "genois"
#define WIFI_PASS "master13"

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();
  
  // put your setup code here, to run once:
  connectwifi();

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
