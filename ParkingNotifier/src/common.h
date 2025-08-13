#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>

// LoRa Parameters
#define FREQUENCY           866.3       // for Europe
#define BANDWIDTH           250.0       // kHz
#define SPREADING_FACTOR    9           // 5-12
#define TRANSMIT_POWER      0           // dBm, 0 = 1mW
#define PAUSE               30          // Send every 30 seconds

// Pin Definitions
extern int trigPin;
extern int echoPin;
extern int sck;
extern int miso;
extern int mosi;
extern int cs;
extern const int ldrPin;

// Global Variables - Shared between transmitter and receiver
extern long counter;
extern uint64_t last_tx;
extern uint64_t tx_time;
extern uint64_t minimum_pause;
extern bool notificationsEnabled;

// Distance sensor variables
extern long duration, cm, inches;
extern long lastEventTime;
extern long timerDelay;
extern bool carPresent;

// Real time clock
extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;
extern int event_counter;
extern struct tm timeinfo;
extern char datestr[20];
extern char timestr[10];

// Photoresistor
extern int lightValue;
extern int log_counter;
extern unsigned long lastLogTime;
extern const unsigned long logInterval;
extern char dateDatastr[20];
extern char timeDatastr[10];

// SD card paths
extern const char* parking_data_folder;
extern const char* light_data_folder;

// Receiver-specific variables
extern String rxdata;
extern volatile bool rxFlag;
extern int packets_received;
extern uint64_t last_packet_time;
extern float last_rssi;
extern float last_snr;

// WiFi and Telegram (receiver only)
#ifdef RECEIVER_MODE
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* BOTtoken;
extern String Mychat_id;
extern String bot_name;
extern WiFiClientSecure secured_client;
extern UniversalTelegramBot bot;
extern const unsigned long BOT_MTBS;
extern unsigned long bot_lasttime;
#endif

// Function Declarations
String createDataPacket();
void rx();
void parseReceivedData(String data);
void showDetailedStats();
void logEvent(int value);
void logLightValue(int value, bool from_command);

#endif