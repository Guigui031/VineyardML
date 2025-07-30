#include <Arduino.h>
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
const char* parking_data_folder = "/parking_data";
const char* light_data_folder = "/light_data";

// distance sensor
int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;
long lastEventTime = millis();
long timerDelay = 15 * 1000;
bool carPresent = false;

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

void connectSDCardModule() {
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card Mount Succeeded");

  // Ensure parking_data folder exists
  if (!SD.exists(parking_data_folder)) {
    Serial.printf("Folder %s does not exist. Creating...\n", parking_data_folder);
    if (SD.mkdir(parking_data_folder)) {
      Serial.println("Folder created successfully.");
    } else {
      Serial.println("Failed to create folder.");
    }
  } else {
    Serial.printf("Folder %s already exists.\n", parking_data_folder);
  }
  
  // Ensure light_data folder exists
  if (!SD.exists(light_data_folder)) {
    Serial.printf("Folder %s does not exist. Creating...\n", light_data_folder);
    if (SD.mkdir(light_data_folder)) {
      Serial.println("Folder created successfully.");
    } else {
      Serial.println("Failed to create folder.");
    }
  } else {
    Serial.printf("Folder %s already exists.\n", light_data_folder);
  }
}

void logEvent(int value) {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;  //TODO: enregistrer autre chose au lieu
  }

  // Create filename: /YYYY-MM-DD.csv
  char filename[40];      // File path string
  strftime(datestr, sizeof(datestr), "%Y-%m-%d", &timeinfo);
  snprintf(filename, sizeof(filename), "%s/%s.csv", parking_data_folder, datestr);
  Serial.print("Creating/using file: ");
  Serial.println(filename);

  // Create header if file doesn't exist
  if (!SD.exists(filename)) {
    writeFile(SD, filename, "index,date,time,distance(cm),millis,notification\n"); // CSV header
  }

  // Create data line: value,HH:MM:SS\n
  char line[64];
  strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
  snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,%d\n", value, datestr, timestr, cm, lastEventTime, notificationsEnabled);

  // Append to file
  appendFile(SD, filename, line);

  // Print to Serial
  Serial.print("Logged: ");
  Serial.print(line);
}

void logLightValue(int value, bool from_command) {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;  //TODO: enregistrer autre chose au lieu
  }

  // Create filename: /YYYY-MM-DD.csv
  char filename[40];      // File path string
  strftime(dateDatastr, sizeof(dateDatastr), "%Y-%m-%d", &timeinfo);
  snprintf(filename, sizeof(filename), "%s/%s.csv", light_data_folder, dateDatastr);
  Serial.print("Creating/using file: ");
  Serial.println(filename);

  // Create header if file doesn't exist
  if (!SD.exists(filename)) {
    writeFile(SD, filename, "index,date,time,light,millis,command\n"); // CSV header
  }

  // Create data line
  char line[64];
  strftime(timeDatastr, sizeof(timeDatastr), "%H:%M:%S", &timeinfo);
  snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,%d\n", value, dateDatastr, timeDatastr, lightValue, lastLogTime, from_command);

  // Append to file
  appendFile(SD, filename, line);

  // Print to Serial
  Serial.print("Logged: ");
  Serial.print(line);
}


void measureData(int log_counter, bool from_command) {
  lightValue = analogRead(ldrPin); // Range: 0 (dark) to 1023 (bright)
  Serial.print("Light level: ");
  Serial.println(lightValue);
  logLightValue(log_counter, from_command);
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
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.println();
  Serial.println("Starting...");

  // init pins for distance sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // init pins for sd card module
  connectSDCardModule();

  // Init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // put your setup code here, to run once:
  connectwifi();
  connectTelegramBot();
  lastLogTime = millis();
  measureData(log_counter++, false);
  
  Serial.println("Patrol Mode Initiated...");
}

void loop() {
  unsigned long currentMillis = millis();
  // put your main code here, to run repeatedly:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  inches = (duration / 2) / 74;
  cm = (duration / 2) / 29;
  Serial.println(cm);

  // Detection logic
  // if (inches < 10 && inches > 0) {
  //   // Car detected
  //   if (!carPresent) {
  //     Serial.println("Car entered.");
  //     carPresent = true;
  //   }
  // } else if (inches > 0) {
  //   // Car not detected
  //   if (carPresent) {
  //     Serial.println("Car left.");
  //     if (notificationsEnabled) {
  //       bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
  //     }
  //     logEvent(event_counter++);
  //     carPresent = false;
  //   }
  // }
  if (cm < 30 && cm > 0  && currentMillis - lastEventTime > timerDelay) {
      if (notificationsEnabled) {
        bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
      }
      logEvent(event_counter++);
      lastEventTime = currentMillis;
  }

  // Measure data
  if (currentMillis - lastLogTime >= logInterval) {
    lastLogTime = currentMillis;
    measureData(log_counter++, false);
  }

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

