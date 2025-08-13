#include <Arduino.h>

// Pause between transmitted packets in seconds.
// Set to zero to only transmit when pressing the user button
#define PAUSE               30          // Send every 30 seconds

// Frequency in MHz. Keep the decimal point to designate float.
#define FREQUENCY           866.3       // for Europe
// #define FREQUENCY           905.2     // for US

// LoRa parameters
#define BANDWIDTH           250.0       // kHz
#define SPREADING_FACTOR    9           // 5-12, higher = longer range
#define TRANSMIT_POWER      0           // dBm, 0 = 1mW

// Global variables
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;
bool notificationsEnabled = false;

// SD card
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;
const char* parking_data_folder = "/parking_data";
const char* light_data_folder = "/light_data";

// distance sensor
int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;
long lastEventTime = millis();
long timerDelay = 15 * 1000;
bool carPresent = false;

// real time clock
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5 * 60 * 60;
const int   daylightOffset_sec = 3600;
int event_counter = 0;
struct tm timeinfo;
char datestr[20];       // e.g. "2025-05-27"
char timestr[10];       // e.g. "23:12:15"

// Photoresistor
const int ldrPin = 33;
int lightValue;
int log_counter = 0;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 30000; // 1 hour in milliseconds = 3600000UL
char dateDatastr[20];       // e.g. "2025-05-27"
char timeDatastr[10];       // e.g. "23:12:15"