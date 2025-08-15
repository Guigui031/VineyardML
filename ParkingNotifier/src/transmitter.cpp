/**
 * LoRa Transmitter - Parking Notifier System
 * Standalone version with all functions included
 */

#define HELTEC_POWER_BUTTON
#include <Arduino.h>
#include <heltec_unofficial.h>
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
int trigPin = 2;
int echoPin = 4;
int sck = 45;
int miso = 42;
int mosi = 32;
int cs = 5;
const int ldrPin = 33;

// Global Variables
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time = 0;
uint64_t minimum_pause = 0;

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

void setupTransmitter() {
    initRadio();
    
    Serial.printf("TX power: %i dBm\n", TRANSMIT_POWER);
    RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
    
    Serial.println("LoRa Transmitter Ready");
}

bool transmitData(String data) {
    // Check if transmission is legal (1% duty cycle limit)
    bool tx_legal = millis() > last_tx + minimum_pause;
    if (!tx_legal) {
        Serial.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
        return false;
    }
    
    Serial.printf("TX [%s] ", data.c_str());
    
    // Turn on LED during transmission
    heltec_led(50);
    
    // Transmit the packet
    tx_time = millis();
    int state = radio.transmit(data.c_str());
    tx_time = millis() - tx_time;
    
    // Turn off LED
    heltec_led(0);
    
    // Calculate minimum pause for 1% duty cycle compliance
    minimum_pause = tx_time * 100;
    last_tx = millis();
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("OK (%i ms)\n", (int)tx_time);
        return true;
    } else {
        Serial.printf("fail (%i)\n", state);
        return false;
    }
}

String createDataPacket() {
    // JSON-like data with multiple values including car detection
    String packet = "{";
    packet += "\"id\":" + String(counter) + ",";
    packet += "\"bat\":" + String(heltec_vbat(), 2) + ",";
    packet += "\"time\":" + String(millis()/1000) + ",";
    packet += "\"distance\":" + String(cm) + ",";
    packet += "\"light\":" + String(lightValue) + ",";
    packet += "\"car\":" + String(carPresent ? "true" : "false");
    packet += "}";
    
    return packet;
}

// ===== DISPLAY FUNCTIONS =====
void init_display() {
    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
}

void displayTransmitterStatus() {
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

void show_transmission(int counter, String data) {
    display.clear();
    display.drawString(0, 0, "Transmitting...");
    display.drawString(0, 10, "Count: " + String(counter));
    display.drawString(0, 20, "Data: " + data);
    display.display();
}

void show_transmission_fail(int state) {
    display.clear();
    display.drawString(0, 0, "TX Failed!");
    display.drawString(0, 10, "Error: " + String(state));
    display.display();
}

void show_transmission_success() {
    display.clear();
    display.drawString(0, 0, "TX Success!");
    display.drawString(0, 10, "Time: " + String((int)tx_time) + "ms");
    display.drawString(0, 20, "Count: " + String(counter));
    display.drawString(0, 30, "Next in " + String(PAUSE) + "s");
    display.display();
}

void displayTransmitterInfo() {
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
}

// ===== SENSOR FUNCTIONS =====
void initSensors() {
    // Initialize distance sensor pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    Serial.println("Sensors initialized");
}

int get_distance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(trigPin, LOW);

    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);

    inches = (duration / 2) / 74;
    cm = (duration / 2) / 29;
    
    Serial.print("Distance: ");
    Serial.print(cm);
    Serial.println(" cm");
    
    return cm;
}

bool checkCarDetection() {
    cm = get_distance();
    unsigned long currentMillis = millis();
    
    // Check if car is detected (distance < 30cm and > 0cm for valid reading)
    bool carDetected = (cm < 30 && cm > 0 && currentMillis - lastEventTime > timerDelay);
    
    if (carDetected) {
        carPresent = true;
        lastEventTime = currentMillis;
        Serial.println("Car detected!");
        return true;
    } else {
        carPresent = false;
        return false;
    }
}

// ===== SD CARD FUNCTIONS =====
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

void logEvent(int value) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time");
        return;
    }

    // Create filename: /YYYY-MM-DD.csv
    char filename[40];
    strftime(datestr, sizeof(datestr), "%Y-%m-%d", &timeinfo);
    snprintf(filename, sizeof(filename), "%s/%s.csv", parking_data_folder, datestr);
    Serial.print("Creating/using file: ");
    Serial.println(filename);

    // Create header if file doesn't exist
    if (!SD.exists(filename)) {
        writeFile(SD, filename, "index,date,time,distance(cm),millis,notification\n");
    }

    // Create data line
    char line[64];
    strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
    snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,1\n", value, datestr, timestr, cm, lastEventTime);

    // Append to file
    appendFile(SD, filename, line);

    // Print to Serial
    Serial.print("Logged: ");
    Serial.print(line);
}

void logLightValue(int value, bool from_command) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time");
        return;
    }

    // Create filename: /YYYY-MM-DD.csv
    char filename[40];
    strftime(dateDatastr, sizeof(dateDatastr), "%Y-%m-%d", &timeinfo);
    snprintf(filename, sizeof(filename), "%s/%s.csv", light_data_folder, dateDatastr);
    Serial.print("Creating/using file: ");
    Serial.println(filename);

    // Create header if file doesn't exist
    if (!SD.exists(filename)) {
        writeFile(SD, filename, "index,date,time,light,millis,command\n");
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

int getLightLevel() {
    lightValue = analogRead(ldrPin);
    return lightValue;
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
    
    // Initialize LoRa radio for transmission
    setupTransmitter();
    
    // Initialize sensors
    initSensors();
    
    // Initialize SD card
    connectSDCardModule();
    
    // Initialize time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Show setup complete on display
    displayTransmitterInfo();
    
    // Initial sensor readings
    lastLogTime = millis();
    measureData(log_counter++, false);
    
    Serial.println("LoRa Transmitter - Patrol Mode Initiated...");
}

void loop() {
    heltec_loop();
    
    unsigned long currentMillis = millis();
    
    // Check for car detection
    if (checkCarDetection()) {
        // Car detected - transmit immediately
        String data = createDataPacket();
        if (transmitData(data)) {
            show_transmission_success();
            Serial.println("Car detection transmitted successfully");
        } else {
            show_transmission_fail(-1);
            Serial.println("Car detection transmission failed");
        }
        
        // Log the event
        logEvent(event_counter++);
        counter++;
    }
    
    // Regular periodic transmission (every PAUSE seconds) or button press
    bool tx_legal = millis() > last_tx + minimum_pause;
    if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
        
        String data = createDataPacket();
        show_transmission(counter, data);
        
        if (transmitData(data)) {
            show_transmission_success();
        } else {
            show_transmission_fail(-1);
        }
        
        counter++;
    }
    
    // Periodic light measurement logging
    if (currentMillis - lastLogTime >= logInterval) {
        lastLogTime = currentMillis;
        measureData(log_counter++, false);
    }
    
    // Update display status periodically
    static uint64_t last_display_update = 0;
    if (millis() - last_display_update > 5000 && millis() - last_tx > 3000) {
        last_display_update = millis();
        displayTransmitterStatus();
    }
}