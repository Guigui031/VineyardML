/**
 * LoRa Receiver Only - Receives packets and displays data
 * 
 * This code only receives data and displays it on screen and serial.
 * Perfect for a base station that collects data from sensor nodes.
 */

// Turns the 'PRG' button into the power button, long press is off 
#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
#include <Arduino.h>
#include <heltec_unofficial.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

// Frequency in MHz - MUST match transmitter frequency
#define FREQUENCY           866.3       // for Europe
// #define FREQUENCY           905.2     // for US

// LoRa parameters - MUST match transmitter settings
#define BANDWIDTH           250.0       // kHz
#define SPREADING_FACTOR    9           // 5-12

// Global variables
String rxdata;
volatile bool rxFlag = false;
int packets_received = 0;
uint64_t last_packet_time = 0;
float last_rssi = 0;
float last_snr = 0;

// WiFi
#define WIFI_SSID "SM-S921W6681"
#define WIFI_PASS "qwer1234"

// Telegram bot
#define BOTtoken "8069531606:AAGbX_1IGLndlqWLSrzhMnUugFq2B06N8nw" // your Bot Token (Get from Botfather)
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications"; // you can change this to whichever is your bot name

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

const unsigned long BOT_MTBS = 1000; // mean time between scan messages
unsigned long bot_lasttime; // last time messages' scan has been done
bool notificationsEnabled = true;

// Can't do Serial or display operations in interrupt, just set flag
void rx() {
  rxFlag = true;
}

void displayReceivedData() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  
  // Show packet info
  display.drawString(0, 0, "Packet #" + String(packets_received));
  display.drawString(0, 10, "RSSI: " + String(last_rssi, 1) + " dBm");
  display.drawString(0, 20, "SNR: " + String(last_snr, 1) + " dB");
  
  // Show received data (truncated for display)
  String displayData = rxdata;
  if (displayData.length() > 16) {
    displayData = displayData.substring(0, 16) + "...";
  }
  display.drawString(0, 30, "Data: " + displayData);
  
  // Show signal quality indicator
  String quality = "Poor";
  if (last_rssi > -80) quality = "Good";
  if (last_rssi > -60) quality = "Excellent";
  display.drawString(0, 40, "Signal: " + quality);
  
  display.display();
}

void parseReceivedData(String data) {
  // Try to parse JSON-formatted data
  if (data.startsWith("{") && data.endsWith("}")) {
    Serial.println("--- Parsed JSON Data ---");
    
    // Simple JSON parsing (you could use ArduinoJson library for complex data)
    int idPos = data.indexOf("\"id\":");
    int batPos = data.indexOf("\"bat\":");
    int rssiPos = data.indexOf("\"rssi\":");
    int timePos = data.indexOf("\"time\":");
    
    if (idPos != -1) {
      int start = data.indexOf(":", idPos) + 1;
      int end = data.indexOf(",", start);
      if (end == -1) end = data.indexOf("}", start);
      String id = data.substring(start, end);
      Serial.println("Transmitter ID: " + id);
    }
    
    if (batPos != -1) {
      int start = data.indexOf(":", batPos) + 1;
      int end = data.indexOf(",", start);
      if (end == -1) end = data.indexOf("}", start);
      String battery = data.substring(start, end);
      Serial.println("Battery Voltage: " + battery + "V");
    }
    
    if (timePos != -1) {
      int start = data.indexOf(":", timePos) + 1;
      int end = data.indexOf("}", start);
      String timestamp = data.substring(start, end);
      Serial.println("Transmitter Uptime: " + timestamp + "s");
    }
    
    Serial.println("--- End Parsed Data ---");
  }
  
  // Try to parse CSV-formatted data
  else if (data.indexOf(",") != -1) {
    Serial.println("--- Parsed CSV Data ---");
    
    int pos = 0;
    int field = 0;
    String fieldNames[] = {"Counter", "Battery", "Sensor1", "Timestamp"};
    
    while (pos < data.length() && field < 4) {
      int nextComma = data.indexOf(",", pos);
      if (nextComma == -1) nextComma = data.length();
      
      String value = data.substring(pos, nextComma);
      Serial.println(fieldNames[field] + ": " + value);
      
      pos = nextComma + 1;
      field++;
    }
    
    Serial.println("--- End Parsed Data ---");
  }
}

void showDetailedStats() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  
  display.drawString(0, 0, "=== STATISTICS ===");
  display.drawString(0, 10, "Total packets: " + String(packets_received));
  
  if (last_packet_time > 0) {
    int minutes_ago = (millis() - last_packet_time) / 60000;
    int seconds_ago = ((millis() - last_packet_time) % 60000) / 1000;
    
    display.drawString(0, 20, "Last RX: " + String(minutes_ago) + "m" + String(seconds_ago) + "s");
    display.drawString(0, 30, "Last RSSI: " + String(last_rssi, 1));
    display.drawString(0, 40, "Last SNR: " + String(last_snr, 1));
  } else {
    display.drawString(0, 20, "No packets received");
  }
  
  display.display();
  
  // Show for 3 seconds then return to normal display
  delay(3000);
}

// Optional: Log data to SD card or send to internet
void logReceivedData(String data) {
  // Example: Save to SD card with timestamp
  /*
  File logFile = SD.open("/received_data.txt", FILE_APPEND);
  if (logFile) {
    logFile.print(millis());
    logFile.print(",");
    logFile.print(last_rssi);
    logFile.print(",");
    logFile.print(last_snr);
    logFile.print(",");
    logFile.println(data);
    logFile.close();
  }
  */
  
  // Example: Send to web server
  /*
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://your-server.com/api/data");
    http.addHeader("Content-Type", "application/json");
    
    String jsonPayload = "{\"data\":\"" + data + "\",\"rssi\":" + String(last_rssi) + ",\"snr\":" + String(last_snr) + "}";
    int httpResponseCode = http.POST(jsonPayload);
    
    http.end();
  }
  */
}

void connectwifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // WiFi.begin(WIFI_SSID) or WiFi.begin(WIFI_SSID, WIFI_PASS); depending on if password is necessary
  Serial.print("Connecting to Wi-Fi:");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.print(" Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void connectTelegramBot() {
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected, attempting to send Telegram message...");
    bool success = bot.sendMessage(Mychat_id, "Bot connecté! \n/help pour afficher les commandes disponibles.", "");
    if (!success) {
      Serial.println("Failed to send message.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    bot.sendMessage(chat_id, text, "");
    if (text == "/status") {
      bot.sendMessage(chat_id, "Système connecté et en marche.", "");
    } else if (text == "/log") {
      bot.sendMessage(chat_id, 
        "Dernière auto détectée : " + String(timestr) + ".\n"
        "Dernier enregistrement de données : " + String(timeDatastr) + ".\n"
        "Dernière intensité lumineuse enregistrée : " + String(lightValue) + "."
        , ""
      );
    } else if (text == "/data") {
      measureData(log_counter++, true);
      bot.sendMessage(chat_id, 
        "Présente intensité lumineuse : " + String(lightValue) + "."
        , ""
      );
    } else if (text == "/off") {
      notificationsEnabled = false;
      bot.sendMessage(chat_id, "Notifications désactivées.", "Markdown");
    } else if (text == "/on") {
      notificationsEnabled = true;
      bot.sendMessage(chat_id, "Notifications activées.", "Markdown");
    } else if (text == "/help") {
      bot.sendMessage(chat_id,
        "*Commandes disponibles:*\n"
        "/status - Affiche l'état du système\n"
        "/log - Affiche l'heure de la dernière auto détectée\n"
        "/data - Affiche les valeurs des différentes sondes présentement\n"
        "/on - Active les notifications lors d'une détection d'auto\n"
        "/off - Désactive les notifications jusqu'à réactivation avec /on. Enregistre tout de même les détections\n"
        "/help - Affiche cette aide\n"
        , "Markdown"
      );
    } else {
      bot.sendMessage(chat_id, "Commande inconnue. Tapez /help pour la liste des commandes.", "");
    }
  }
}

void setup() {  
  heltec_setup();
  
  // Re-initialize display after heltec_setup
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  // Initialize radio
  display.drawString(0, 0, "Radio init...");
  display.display();
  
  Serial.println("LoRa Receiver Starting...");
  RADIOLIB_OR_HALT(radio.begin());
  
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  
  // Set radio parameters - must match transmitter
  Serial.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  
  Serial.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  
  Serial.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  
  // Start receiving immediately
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  
  // Display setup complete
  display.clear();
  display.drawString(0, 0, "LoRa RX Ready");
  display.drawString(0, 10, "Freq: " + String(FREQUENCY, 1) + "MHz");
  display.drawString(0, 20, "SF" + String(SPREADING_FACTOR) + " BW" + String(BANDWIDTH, 0));
  display.drawString(0, 30, "Listening...");
  display.drawString(0, 40, "Packets: 0");
  display.display();
  
  connectwifi();
  connectTelegramBot();

  Serial.println("Setup complete. Listening for packets...");
}

void loop() {
  heltec_loop();
  
  // If a packet was received, process it
  if (rxFlag) {
    rxFlag = false;
    
    // Read the received data
    int state = radio.readData(rxdata);
    
    if (state == RADIOLIB_ERR_NONE) {
      // Successful reception
      packets_received++;
      last_packet_time = millis();
      last_rssi = radio.getRSSI();
      last_snr = radio.getSNR();
      
      // Print to serial
      Serial.println("=== PACKET RECEIVED ===");
      Serial.printf("Data: %s\n", rxdata.c_str());
      Serial.printf("RSSI: %.2f dBm\n", last_rssi);
      Serial.printf("SNR: %.2f dB\n", last_snr);
      Serial.printf("Packet #: %d\n", packets_received);
      Serial.printf("Length: %d bytes\n", rxdata.length());
      Serial.println();
      
      // Process and display the received data
      displayReceivedData();
      
      // Parse JSON data if applicable
      parseReceivedData(rxdata);

      if (notificationsEnabled) {
        bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
      }
      
    } else {
      // Reception failed
      Serial.printf("RX failed, code %d\n", state);
      
      display.clear();
      display.drawString(0, 0, "RX Error!");
      display.drawString(0, 10, "Code: " + String(state));
      display.display();
    }
    
    // Start receiving again
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
  
  // Update display with status every few seconds
  static uint64_t last_display_update = 0;
  if (millis() - last_display_update > 3000) {
    last_display_update = millis();
    
    // If no recent packet, show listening status
    if (millis() - last_packet_time > 5000) {
      display.clear();
      display.drawString(0, 0, "LoRa Receiver");
      display.drawString(0, 10, "Listening...");
      display.drawString(0, 20, "Packets: " + String(packets_received));
      
      if (last_packet_time > 0) {
        int seconds_ago = (millis() - last_packet_time) / 1000;
        display.drawString(0, 30, "Last: " + String(seconds_ago) + "s ago");
        display.drawString(0, 40, "RSSI: " + String(last_rssi, 1) + "dBm");
      } else {
        display.drawString(0, 30, "No packets yet");
      }
      
      display.display();
    }
  }
  
  // Button press shows detailed stats
  if (button.isSingleClick()) {
    showDetailedStats();
  }

  // handle Telegram messages
  if (currentMillis - bot_lasttime > BOT_MTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = currentMillis;
    }
}

