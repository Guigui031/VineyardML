#include "common.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Pin Definitions
int trigPin = 2;
int echoPin = 4;
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;
const int ldrPin = 33;

// Global Variables - Shared between transmitter and receiver
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time = 0;
uint64_t minimum_pause = 0;
bool notificationsEnabled = false;

// Distance sensor variables
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

// Photoresistor
int lightValue = 0;
int log_counter = 0;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 30000; // 30 seconds
char dateDatastr[20];
char timeDatastr[10];

// SD card paths
const char* parking_data_folder = "/parking_data";
const char* light_data_folder = "/light_data";

// Receiver-specific variables
String rxdata = "";
volatile bool rxFlag = false;
int packets_received = 0;
uint64_t last_packet_time = 0;
float last_rssi = 0;
float last_snr = 0;

// WiFi and Telegram (receiver only)
const char* WIFI_SSID = "SM-S921W6681";
const char* WIFI_PASS = "qwer1234";
const char* BOTtoken = "8069531606:AAGbX_1IGLndlqWLSrzhMnUugFq2B06N8nw";
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications";
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);
const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime = 0;