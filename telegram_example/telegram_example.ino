// ArduinoJson - Version: 5.13.5
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>


/*
  MKR IoT Cloud Kit - Interact with MKR1010/10000, MKR ENV shield and Proto Shield
  using Telegram

  The circuit:
  - Arduino MKR board
  - Arduino MKR ENV Shield attached
  - Arduino MKR Proto Shield

  This example is heavily inspired by FlashledBot example for the Universal Telegram Bot Library
  Big Thanks to Giacarlo Bacchio (Gianbacchio on Github) and Brian Lough

*/

#include <Arduino_MKRENV.h>


#ifdef ARDUINO_SAMD_MKRWIFI1010
#include <WiFiNINA.h>
#elif ARDUINO_SAMD_MKR1000
#include <WiFi101.h>
#else
#error unsupported board
#endif

#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
char ssid[] = "SSID"; // your network SSID (name)
char password[] = "PASSWORD";  // your network key
#define BOTtoken "BOT_Token" // your Bot Token (Get from Botfather)

String Mychat_id = "My_C";

String bot_name = "Envibot"; // you can change this to whichever is your bot name

WiFiSSLClient client;

UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done

bool Start = false;
int ledStatus = 0;

const int ledPin = LED_BUILTIN;

// ENV Variables

float t;
float h;
float p;
float l;

// Temperature Treschold. change this data to be warned
// of the overriding of a limit and be notified

float temp_limit = 35.5;

// RElAYs and other proto stuff

int relay1 = 1;
int relay2 = 2;
int rel1Status = 0;
int rel2Status = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1);
  }

  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  String list = "Hi, I just woke up and this is what can I do for you \n";
  list = list + "/ledon : to switch the Led ON \n";
  list = list + "/ledoff : to switch the Led OFF \n";
  list = list + "/status : Returns current status of LED \n";
  list = list + "/rel1on : to switch Relay 1 ON \n";
  list = list + "/rel1off : to switch Relay 1 OFF \n";
  list = list + "/rel1status : Returns current status of LED \n";
  list = list + "/rel2on : to switch Relay 2 ON \n";
  list = list + "/rel2off : to switch Relay 2 OFF \n";
  list = list + "/rel2status : Returns current status of LED \n";
  list = list + "/temperature : Returns current value Temperature\n";
  list = list + "/humidity : Returns current Humidity value\n";
  list = list + "/pressure : Returns current Pressure value\n";
  list = list + "/light : Returns current LUX value\n";
  list = list + "Moreover, I'll warn you if Temperature overrides  " + temp_limit + " Celsius Degrees \n";

  bot.sendMessage(Mychat_id, list, "Markdown");


}

void loop() {

  if (millis() > Bot_lasttime + Bot_mtbs)  {

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {

      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    }

    Bot_lasttime = millis();

  }

  // Read sensors for the notification

  float t = ENV.readTemperature();
  float h = ENV.readHumidity();
  float p = ENV.readPressure();
  float l = ENV.readLux();

  // check if the temperature is overriding
  // the limit you set at he beginning of the sketch

  if (t > temp_limit) {
    bot.sendMessage(Mychat_id, "Temperature " + String(t) + "°C");
  }

}

void handleNewMessages(int numNewMessages) {

  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = String(bot.messages[i].chat_id);
    Serial.println(chat_id);
    String text = bot.messages[i].text;

    if (text == "/ledon") {

      digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
      ledStatus = 1;
      bot.sendMessage(Mychat_id, "Led is ON", "");

    }

    if (text == "/ledoff") {

      ledStatus = 0;
      digitalWrite(ledPin, LOW);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(Mychat_id, "Led is OFF", "");

    }

    if (text == "/status") {

      if (ledStatus) {
        bot.sendMessage(Mychat_id, "Led is ON", "");
      } else {
        bot.sendMessage(Mychat_id, "Led is OFF", "");
      }
    }

    if (text == "/rel1on") {

      digitalWrite(relay1, HIGH);   // turn the LED on (HIGH is the voltage level)
      rel1Status = 1;
      bot.sendMessage(Mychat_id, "Relay 1 is ON", "");

    }

    if (text == "/rel1off") {

      rel1Status = 0;
      digitalWrite(relay1, LOW);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(Mychat_id, "Relay 1 is OFF", "");

    }

    if (text == "/rel1status") {

      if (rel1Status) {
        bot.sendMessage(Mychat_id, "Relay 1 is ON", "");
      } else {
        bot.sendMessage(Mychat_id, "Relay 1 is OFF", "");
      }
    }

    if (text == "/rel2on") {

      digitalWrite(relay2, HIGH);   // turn the LED on (HIGH is the voltage level)
      rel2Status = 1;
      bot.sendMessage(Mychat_id, "Relay 2 is ON", "");

    }

    if (text == "/rel2off") {

      rel2Status = 0;
      digitalWrite(relay2, LOW);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(Mychat_id, "Relay 2 is OFF", "");

    }

    if (text == "/rel2status") {

      if (rel2Status) {
        bot.sendMessage(Mychat_id, "Relay 2 is ON", "");
      } else {
        bot.sendMessage(Mychat_id, "Relay 2 is OFF", "");
      }
    }

    if (text == "/start") {

      String from_name = bot.messages[i].from_name;
      if (from_name == "") from_name = "Anonymous";

      String welcome = "Welcome, " + from_name + ", from " + bot_name + " , your personal Bot on Arduino MKR1010\n";
      welcome = welcome + "/ledon & ledoff allows you to control the Led \n";
      welcome = welcome + "/rel1on or /rel1off: to control relay 1 \n";
      welcome = welcome + "/rel2on or /rel2off: to control relay 2 \n";
      welcome = welcome + "/status /rel1status or /rel2status will returns the current status of LED or Relays \n";
      welcome = welcome + "/list : Show the complete command list \n";
      bot.sendMessage(Mychat_id, welcome, "Markdown");
    }

    if (text == "/Temperature" || text == "/temperature") {
      float t = ENV.readTemperature();
      bot.sendMessage(Mychat_id, "Temperature " + String(t) + "°C");
    }
    if (text == "/Humidity" || text == "/humidity") {
      float h = ENV.readHumidity();
      bot.sendMessage(Mychat_id, " Humidity " + String(h) + " %");
    }
    if (text == "/Pressure" || text == "/pressure") {
      float p = ENV.readPressure();
      bot.sendMessage(Mychat_id, " Pressure " + String(p) + " kPa");
    }
    if (text == "/Light" || text == "/light") {
      float l = ENV.readLux();
      bot.sendMessage(Mychat_id,  String(l) +  " Lumen"  );
    }

    if (text == "/list" || text == "/help") {
      String from_name = bot.messages[i].from_name;
      String list = "Hi, " + from_name + ", this is what can I do for you \n";
      list = list + "/ledon : to switch the Led ON \n";
      list = list + "/ledoff : to switch the Led OFF \n";
      list = list + "/status : Returns current status of LED \n";
      list = list + "/rel1on : to switch Relay 1 ON \n";
      list = list + "/rel1off : to switch Relay 1 OFF \n";
      list = list + "/rel1status : Returns current status of LED \n";
      list = list + "/rel2on : to switch Relay 2 ON \n";
      list = list + "/rel2off : to switch Relay 2 OFF \n";
      list = list + "/rel2status : Returns current status of LED \n";
      list = list + "/temperature : Returns current value Temperature\n";
      list = list + "/humidity : Returns current Humidity value\n";
      list = list + "/pressure : Returns current Pressure value\n";
      list = list + "/light : Returns current LUX value\n";
      list = list + "Moreover, I'll warn you if Temperature overrides  " + temp_limit + " Celsius Degrees \n";

      bot.sendMessage(Mychat_id, list, "Markdown");
    }

  }
}
