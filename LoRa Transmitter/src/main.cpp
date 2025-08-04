/**
 * LoRa Transmitter Only - Sends packets with sensor data or counter
 * 
 * This code only transmits data and doesn't listen for incoming packets.
 * Perfect for a sensor node that sends data to a central receiver.
 */

// Turns the 'PRG' button into the power button, long press is off 
#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
#include <Arduino.h>
#include <heltec_unofficial.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

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


// Create data packet to transmit
String createDataPacket() {
  // You can customize this function to send different types of data
  
  // Option 1: Simple counter
  // return String(counter);
  
  // Option 2: JSON-like data with multiple values
  String packet = "{";
  packet += "\"id\":" + String(counter) + ",";
  packet += "\"bat\":" + String(heltec_vbat(), 2) + ",";
  packet += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  packet += "\"time\":" + String(millis()/1000);
  packet += "}";
  
  return packet;
  
  // Option 3: Sensor data (uncomment and modify as needed)
  /*
  String packet = String(counter) + ",";
  packet += String(heltec_vbat(), 2) + ",";
  packet += String(analogRead(A0)) + ",";
  packet += String(millis()/1000);
  return packet;
  */
}

void measureData(int log_counter, bool from_command) {
  lightValue = analogRead(ldrPin); // Range: 0 (dark) to 1023 (bright)
  Serial.print("Light level: ");
  Serial.println(lightValue);
  logLightValue(log_counter, from_command);
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

void setup() {
  // Manual display initialization for clone board
  pinMode(21, OUTPUT);           // Display power
  digitalWrite(21, HIGH);        // Enable display power
  delay(100);
  
  pinMode(18, OUTPUT);           // Reset pin
  digitalWrite(18, LOW);         // Reset display
  delay(10);
  digitalWrite(18, HIGH);        // Release reset
  delay(50);
  
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
  
  Serial.println("LoRa Transmitter Starting...");
  RADIOLIB_OR_HALT(radio.begin());
  
  // Set radio parameters
  Serial.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  
  Serial.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  
  Serial.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  
  Serial.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  
  // Display setup complete
  display.clear();
  display.drawString(0, 0, "LoRa TX Ready");
  display.drawString(0, 10, "Freq: " + String(FREQUENCY, 1) + "MHz");
  display.drawString(0, 20, "SF" + String(SPREADING_FACTOR) + " BW" + String(BANDWIDTH, 0));
  display.drawString(0, 30, "Power: " + String(TRANSMIT_POWER) + "dBm");
  if (PAUSE > 0) {
    display.drawString(0, 40, "Auto: " + String(PAUSE) + "s");
  } else {
    display.drawString(0, 40, "Manual mode");
  }
  display.display();
  
  Serial.println("Setup complete. Ready to transmit.");

  // Serial port for debugging purposes
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
  
  lastLogTime = millis();
  measureData(log_counter++, false);
  
  Serial.println("Patrol Mode Initiated...");

  delay(2000);
}

void loop() {
  heltec_loop();
  
  bool tx_legal = millis() > last_tx + minimum_pause;
  
  // Transmit a packet every PAUSE seconds or when the button is pressed
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    
    // Check if transmission is legal (1% duty cycle limit)
    if (!tx_legal) {
      int wait_time = (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1;
      Serial.printf("Legal limit, wait %i sec.\n", wait_time);
      
      display.clear();
      display.drawString(0, 0, "Duty cycle limit");
      display.drawString(0, 10, "Wait: " + String(wait_time) + "s");
      display.display();
      return;
    }
    
    // Prepare data to send
    String data = createDataPacket();
    
    Serial.printf("TX [%s] ", data.c_str());
    
    // Show transmission on display
    display.clear();
    display.drawString(0, 0, "Transmitting...");
    display.drawString(0, 10, "Count: " + String(counter));
    display.drawString(0, 20, "Data: " + data);
    display.display();
    
    // Turn on LED during transmission
    heltec_led(50); // 50% brightness
    
    // Transmit the packet
    tx_time = millis();
    int state = radio.transmit(data.c_str());
    tx_time = millis() - tx_time;
    
    // Turn off LED
    heltec_led(0);
    
    // Check transmission result
    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("OK (%i ms)\n", (int)tx_time);
      
      display.clear();
      display.drawString(0, 0, "TX Success!");
      display.drawString(0, 10, "Time: " + String((int)tx_time) + "ms");
      display.drawString(0, 20, "Count: " + String(counter));
      display.drawString(0, 30, "Next in " + String(PAUSE) + "s");
      display.display();
      
    } else {
      Serial.printf("fail (%i)\n", state);
      
      display.clear();
      display.drawString(0, 0, "TX Failed!");
      display.drawString(0, 10, "Error: " + String(state));
      display.display();
    }
    
    // Calculate minimum pause for 1% duty cycle compliance
    minimum_pause = tx_time * 100;
    last_tx = millis();
    counter++;
  }
  
  // Update display with status every few seconds
  static uint64_t last_display_update = 0;
  if (millis() - last_display_update > 5000 && millis() - last_tx > 3000) {
    last_display_update = millis();
    
    display.clear();
    display.drawString(0, 0, "LoRa Transmitter");
    display.drawString(0, 10, "Count: " + String(counter));
    
    if (PAUSE > 0) {
      int next_tx = PAUSE - ((millis() - last_tx) / 1000);
      if (next_tx > 0) {
        display.drawString(0, 20, "Next TX: " + String(next_tx) + "s");
      } else {
        display.drawString(0, 20, "Ready to TX");
      }
    } else {
      display.drawString(0, 20, "Press button to TX");
    }
    
    // Show battery voltage
    float vbat = heltec_vbat();
    display.drawString(0, 30, "Bat: " + String(vbat, 2) + "V");
    display.drawString(0, 40, "(" + String(heltec_battery_percent(vbat)) + "%)");
    
    display.display();
  }

  // unsigned long currentMillis = millis();
  // // put your main code here, to run repeatedly:
  // digitalWrite(trigPin, LOW);
  // delayMicroseconds(2);
  // digitalWrite(trigPin, HIGH);
  // delayMicroseconds(5);
  // digitalWrite(trigPin, LOW);

  // pinMode(echoPin, INPUT);
  // duration = pulseIn(echoPin, HIGH);

  // inches = (duration / 2) / 74;
  // cm = (duration / 2) / 29;
  // Serial.println(cm);

  // // Detection logic
  // // if (inches < 10 && inches > 0) {
  // //   // Car detected
  // //   if (!carPresent) {
  // //     Serial.println("Car entered.");
  // //     carPresent = true;
  // //   }
  // // } else if (inches > 0) {
  // //   // Car not detected
  // //   if (carPresent) {
  // //     Serial.println("Car left.");
  // //     if (notificationsEnabled) {
  // //       bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
  // //     }
  // //     logEvent(event_counter++);
  // //     carPresent = false;
  // //   }
  // // }
  // if (cm < 30 && cm > 0  && currentMillis - lastEventTime > timerDelay) {
  //     if (notificationsEnabled) {
  //       bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
  //     }
  //     logEvent(event_counter++);
  //     lastEventTime = currentMillis;
  // }

  // // Measure data
  // if (currentMillis - lastLogTime >= logInterval) {
  //   lastLogTime = currentMillis;
  //   measureData(log_counter++, false);
  // }

  // if (currentMillis - bot_lasttime > BOT_MTBS) {
  //   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  //   while (numNewMessages) {
  //     Serial.println("got response");
  //     handleNewMessages(numNewMessages);
  //     numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  //   }
  //   bot_lasttime = currentMillis;
  // }
}
