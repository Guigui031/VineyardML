/**
 * LoRa Receiver - Parking Notifier System
 * Standalone version with all functions included
 */

#define HELTEC_POWER_BUTTON
#include <Arduino.h>
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

// LoRa Parameters
#define FREQUENCY           866.3       // for Europe
#define BANDWIDTH           250.0       // kHz
#define SPREADING_FACTOR    9           // 5-12
#define TRANSMIT_POWER      0           // dBm, 0 = 1mW
#define PAUSE               30          // Send every 30 seconds

// Global Variables - Shared between transmitter and receiver
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time = 0;
uint64_t minimum_pause = 0;
bool notificationsEnabled = true;

// Distance sensor variables (received from transmitter)
long duration = 0;
long cm = 0;
long inches = 0;
long lastEventTime = 0;
long timerDelay = 15 * 1000; // 15 seconds
bool carPresent = false;

// Real time clock
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 60 * 60;
const int daylightOffset_sec = 3600;
int event_counter = 0;
struct tm timeinfo;
char datestr[20];
char timestr[10];

// Photoresistor (received from transmitter)
int lightValue = 0;
int log_counter = 0;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 30000; // 30 seconds
char dateDatastr[20];
char timeDatastr[10];

// Receiver-specific variables
String rxdata = "";
volatile bool rxFlag = false;
int packets_received = 0;
uint64_t last_packet_time = 0;
float last_rssi = 0;
float last_snr = 0;

// WiFi and Telegram
const char* WIFI_SSID = "SM-S921W6681";
const char* WIFI_PASS = "qwer1234";
const char* BOTtoken = "8069531606:AAGbX_1IGLndlqWLSrzhMnUugFq2B06N8nw";
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications";
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);
const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime = 0;

void rx() {
    rxFlag = true;
}

// ===== RADIO FUNCTIONS =====
void initRadio() {
    Serial.println("Initializing LoRa radio...");
    RADIOLIB_OR_HALT(radio.begin());
    
    Serial.printf("Frequency: %.2f MHz\n", FREQUENCY);
    RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
    
    Serial.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
    RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
    
    Serial.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
    RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
}

void setupReceiver() {
    initRadio();
    
    // Set the callback function for received packets
    radio.setDio1Action(rx);
    
    // Start receiving immediately
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    
    Serial.println("LoRa Receiver Ready - Listening for packets...");
}

void startReceiving() {
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void parseReceivedData(String data) {
    // Try to parse JSON-formatted data
    if (data.startsWith("{") && data.endsWith("}")) {
        Serial.println("--- Parsed JSON Data ---");
        
        // Simple JSON parsing
        int idPos = data.indexOf("\"id\":");
        int batPos = data.indexOf("\"bat\":");
        int rssiPos = data.indexOf("\"rssi\":");
        int timePos = data.indexOf("\"time\":");
        int distancePos = data.indexOf("\"distance\":");
        int lightPos = data.indexOf("\"light\":");
        int carPos = data.indexOf("\"car\":");
        
        if (idPos != -1) {
            int start = data.indexOf(":", idPos) + 1;
            int end = data.indexOf(",", start);
            if (end == -1) end = data.indexOf("}", start);
            String id = data.substring(start, end);
            Serial.println("Transmitter ID: " + id);
            counter = id.toInt();
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
            int end = data.indexOf(",", start);
            if (end == -1) end = data.indexOf("}", start);
            String timestamp = data.substring(start, end);
            Serial.println("Transmitter Uptime: " + timestamp + "s");
        }
        
        if (distancePos != -1) {
            int start = data.indexOf(":", distancePos) + 1;
            int end = data.indexOf(",", start);
            if (end == -1) end = data.indexOf("}", start);
            String distance = data.substring(start, end);
            cm = distance.toInt();
            Serial.println("Distance: " + distance + " cm");
        }
        
        if (lightPos != -1) {
            int start = data.indexOf(":", lightPos) + 1;
            int end = data.indexOf(",", start);
            if (end == -1) end = data.indexOf("}", start);
            String light = data.substring(start, end);
            lightValue = light.toInt();
            Serial.println("Light Level: " + light);
        }
        
        if (carPos != -1) {
            int start = data.indexOf(":", carPos) + 1;
            int end = data.indexOf("}", start);
            String carStatus = data.substring(start, end);
            carPresent = carStatus.indexOf("true") != -1;
            Serial.println("Car Present: " + String(carPresent ? "Yes" : "No"));
        }
        
        Serial.println("--- End Parsed Data ---");
    }
}

// ===== DISPLAY FUNCTIONS =====
void init_display() {
    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
}

void displayReceiverStatus() {
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

void displayReceiverInfo() {
    display.clear();
    display.drawString(0, 0, "LoRa RX Ready");
    display.drawString(0, 10, "Freq: " + String(FREQUENCY, 1) + "MHz");
    display.drawString(0, 20, "SF" + String(SPREADING_FACTOR) + " BW" + String(BANDWIDTH, 0));
    display.drawString(0, 30, "Listening...");
    display.drawString(0, 40, "Packets: 0");
    display.display();
}

// ===== NOTIFICATION FUNCTIONS =====
void connectwifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to Wi-Fi:");
    Serial.print(WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
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

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
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
            bot.sendMessage(chat_id, 
                "Données reçues du transmetteur:\n"
                "Dernière intensité lumineuse : " + String(lightValue) + "\n"
                "Dernière distance : " + String(cm) + " cm"
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

void processTelegramCommands() {
    unsigned long currentMillis = millis();
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

void sendCarDetectionNotification() {
    if (notificationsEnabled) {
        bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
    }
}

void sendStatusUpdate() {
    String status = "Status Update:\n";
    status += "Battery: " + String(heltec_vbat(), 2) + "V\n";
    status += "Light: " + String(lightValue) + "\n";
    status += "Distance: " + String(cm) + "cm\n";
    status += "Packets received: " + String(packets_received);
    
    bot.sendMessage(Mychat_id, status, "");
}

// ===== MAIN PROGRAM =====
void setup() {
    heltec_setup();
    
    // Initialize display
    init_display();
    
    // Show radio initialization on display
    display.clear();
    display.drawString(0, 0, "Radio init...");
    display.display();
    
    // Initialize LoRa radio for reception
    setupReceiver();
    
    // Show receiver ready on display
    displayReceiverInfo();
    
    // Connect to WiFi and Telegram
    connectwifi();
    connectTelegramBot();
    
    Serial.println("LoRa Receiver - Setup complete. Listening for packets...");
}

void loop() {
    heltec_loop();
    
    // Process received LoRa packets
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
            
            // Print packet info to serial
            Serial.println("=== PACKET RECEIVED ===");
            Serial.printf("Data: %s\n", rxdata.c_str());
            Serial.printf("RSSI: %.2f dBm\n", last_rssi);
            Serial.printf("SNR: %.2f dB\n", last_snr);
            Serial.printf("Packet #: %d\n", packets_received);
            Serial.printf("Length: %d bytes\n", rxdata.length());
            Serial.println();
            
            // Display received data on screen
            displayReceivedData();
            
            // Parse the received data
            parseReceivedData(rxdata);
            
            // Check if car detection notification should be sent
            if (notificationsEnabled && rxdata.indexOf("\"car\":true") != -1) {
                sendCarDetectionNotification();
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
        startReceiving();
    }
    
    // Update display with receiver status periodically
    static uint64_t last_display_update = 0;
    if (millis() - last_display_update > 3000) {
        last_display_update = millis();
        
        // If no recent packet, show listening status
        if (millis() - last_packet_time > 5000) {
            displayReceiverStatus();
        }
    }
    
    // Button press shows detailed stats
    if (button.isSingleClick()) {
        showDetailedStats();
    }
    
    // Process Telegram commands
    processTelegramCommands();
}