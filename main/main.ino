
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

// SD card
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

// distance sensor
int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;
long lastTime = millis();
long timerDelay = 15 * 1000;

// WiFi
#define WIFI_SSID "SM-S921W6681"
#define WIFI_PASS "qwer1234"

// Telegram bot
#define BOTtoken "8069531606:AAGbX_1IGLndlqWLSrzhMnUugFq2B06N8nw" // your Bot Token (Get from Botfather)
String Mychat_id = "7727548064";
String bot_name = "AuxVoletsNoirs_notifications"; // you can change this to whichever is your bot name

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

// real time clock
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5 * 60 * 60;
const int   daylightOffset_sec = 3600;
int counter = 0;


void setup() {
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.println();
  Serial.println("Starting...");

  // init pins for distance sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // init pins for sd card module
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card Mount Succeded");

  // Init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // put your setup code here, to run once:
  connectwifi();
  connectTelegramBot();
  
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
    logEvent(counter++);
  } 
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
    bool success = bot.sendMessage(Mychat_id, "Bot connecté!", "");
    if (!success) {
      Serial.println("Failed to send message.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}

void logEvent(int value) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;  //TODO: enregistrer autre chose au lieu
  }

  // Create filename: /YYYY-MM-DD.csv
  char filename[20];
  strftime(filename, sizeof(filename), "/%Y-%m-%d.csv", &timeinfo);

  // Create header if file doesn't exist
  if (!SD.exists(filename)) {
    writeFile(SD, filename, "index,distance,time,millis\n"); // CSV header
  }

  // Create data line: value,HH:MM:SS\n
  char line[32];
  char timestr[10];
  strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
  snprintf(line, sizeof(line), "%d,%d,%s,%d\n", value, inches, timestr, lastTime);

  // Append to file
  appendFile(SD, filename, line);

  // Print to Serial
  Serial.print("Logged: ");
  Serial.print(line);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %lu ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
  file.close();
}
