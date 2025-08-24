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
#define PAUSE               0           // Disabled - only send on car detection or time sync

// Pin Definitions
int trigPin = 2;
int echoPin = 3;
int sck = 5;
int miso = 6;
int mosi = 4;
int cs = 7;
const int ldrPin = 19;

// Create separate SPI instance for SD card using FSPI (different from radio's HSPI)
SPIClass sdSPI(FSPI);

// Global Variables
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time = 0;
uint64_t minimum_pause = 0;

// Distance sensor variables
long duration = 0;
const int max_distance = 500; // cm, maximum distance to consider car present
long cm = 0;
long inches = 0;
long lastEventTime = 0;
long timerDelay = 15 * 1000; // 15 seconds cooldown to prevent double notifications
bool carPresent = false;
int consecutiveDetections = 0;
const int requiredDetections = 1; // Require 1 consecutive readings under threshold

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

// Time sync variables
bool timeReceived = false;
unsigned long lastTimeSyncRequest = 0;
const unsigned long timeSyncInterval = 86400000; // Request time every 24 hours
bool timeSyncRequested = false;
volatile bool rxFlag = false;

// SD card paths
const char* parking_data_folder = "/parking_data";
const char* light_data_folder = "/light_data";

// ===== TIME SYNC FUNCTIONS =====
void setFlag(void) {
    rxFlag = true;
}

void updateTimeFromPacket(String timeData) {
    // Parse time data: "YYYY-MM-DD HH:MM:SS,timestamp"
    int commaIndex = timeData.indexOf(',');
    if (commaIndex == -1) return;
    
    String dateTimeStr = timeData.substring(0, commaIndex);
    unsigned long timestamp = timeData.substring(commaIndex + 1).toInt();
    
    // Parse date and time
    int year = dateTimeStr.substring(0, 4).toInt();
    int month = dateTimeStr.substring(5, 7).toInt();
    int day = dateTimeStr.substring(8, 10).toInt();
    int hour = dateTimeStr.substring(11, 13).toInt();
    int minute = dateTimeStr.substring(14, 16).toInt();
    int second = dateTimeStr.substring(17, 19).toInt();
    
    // Set system time
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    // Update system time
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);
    
    timeReceived = true;
    timeSyncRequested = false; // Allow future time sync requests
    Serial.printf("Time updated: %04d-%02d-%02d %02d:%02d:%02d\n", year, month, day, hour, minute, second);
}

String createTimeSyncRequest() {
    return "{\"type\":\"time_request\",\"id\":" + String(counter) + "}";
}

void handleReceivedPacket() {
    String rxdata = "";
    int state = radio.readData(rxdata);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("RX [%s]\n", rxdata.c_str());
        
        // Check if it's a time sync packet
        if (rxdata.indexOf("\"type\":\"time_sync\"") != -1) {
            // Parse time sync packet: {"type":"time_sync","time":"YYYY-MM-DD HH:MM:SS,timestamp"}
            int startIndex = rxdata.indexOf("\"time\":\"") + 8;
            int endIndex = rxdata.indexOf("\"", startIndex);
            if (startIndex != -1 && endIndex != -1) {
                String timeData = rxdata.substring(startIndex, endIndex);
                updateTimeFromPacket(timeData);
            }
        }
    }
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

void setupTransmitter() {
    initRadio();
    
    Serial.printf("TX power: %i dBm\n", TRANSMIT_POWER);
    RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
    
    // Set up interrupt for receiving
    radio.setPacketReceivedAction(setFlag);
    
    // Start listening
    Serial.printf("Starting to listen...\n");
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Listening for time sync packets");
    } else {
        Serial.printf("Failed to start listening, code %d\n", state);
    }
    
    Serial.println("LoRa Transmitter Ready (with RX capability)");
}

bool transmitData(String data) {
    // Check if transmission is legal (1% duty cycle limit)
    bool tx_legal = millis() > last_tx + minimum_pause;
    if (!tx_legal) {
        Serial.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
        return false;
    }
    
    Serial.printf("=== TRANSMITTING ===\n");
    Serial.printf("Data: %s\n", data.c_str());
    Serial.printf("Length: %d bytes\n", data.length());
    Serial.printf("TX [%s] ", data.c_str());
    
    // Stop receiving before transmitting
    radio.standby();
    
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
        Serial.printf("fail (%i) - ", state);
        switch(state) {
            case -1: Serial.println("UNKNOWN_ERROR"); break;
            case -2: Serial.println("CHIP_NOT_FOUND or PACKET_TOO_LONG"); break;
            case -3: Serial.println("PACKET_TOO_LONG"); break;
            case -4: Serial.println("TX_TIMEOUT"); break;
            case -5: Serial.println("RX_TIMEOUT"); break;
            case -6: Serial.println("CRC_MISMATCH"); break;
            default: Serial.printf("ERROR_CODE_%d\n", state); break;
        }
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
    
    // Always check if we're still in cooldown period
    if (currentMillis - lastEventTime < timerDelay) {
        return false; // Still in cooldown, don't check detection
    }
    
    // Check if current reading indicates car presence (distance < 30cm and > 0cm for valid reading)
    bool currentReading = (cm < max_distance && cm > 0);
    
    if (currentReading) {
        consecutiveDetections++;
        Serial.printf("Detection reading %d/%d: %ld cm\n", consecutiveDetections, requiredDetections, cm);
        
        if (consecutiveDetections >= requiredDetections) {
            // Car confirmed detected after multiple readings
            carPresent = true;
            lastEventTime = currentMillis; // Set cooldown period
            consecutiveDetections = 0; // Reset counter
            Serial.printf("Car CONFIRMED detected after %d readings! Distance: %ld cm - Starting %ld sec cooldown\n", 
                         requiredDetections, cm, timerDelay/1000);
            return true;
        }
    } else {
        // No detection, reset counter
        consecutiveDetections = 0;
        carPresent = false;
    }
    
    return false;
}

// ===== SD CARD FUNCTIONS =====
void connectSDCardModule() {
    // Initialize separate SPI instance with custom pins for SD card
    Serial.println("Initializing SD card on separate SPI bus...");
    Serial.printf("SD pins - SCK:%d, MISO:%d, MOSI:%d, CS:%d\n", sck, miso, mosi, cs);
    
    // Initialize FSPI with custom pins (separate from radio's HSPI)
    sdSPI.begin(sck, miso, mosi, cs);
    delay(100); // Give SPI time to initialize
    
    // Try to initialize SD card with lower frequency for stability
    if (!SD.begin(cs, sdSPI, 1000000)) { // 1MHz for stability
        Serial.println("SD Card Mount Failed - continuing without SD logging");
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
    char filename[40];
    char line[64];
    
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time - using null values");
        
        // Use default filename when time is not available
        snprintf(filename, sizeof(filename), "%s/no_time.csv", parking_data_folder);
        
        // Create header if file doesn't exist
        if (!SD.exists(filename)) {
            writeFile(SD, filename, "index,date,time,distance(cm),millis,notification\n");
        }
        
        // Create data line with null time values
        snprintf(line, sizeof(line), "%d,NULL,NULL,%d,%d,1\n", value, cm, lastEventTime);
    } else {
        // Create filename: /YYYY-MM-DD.csv
        strftime(datestr, sizeof(datestr), "%Y-%m-%d", &timeinfo);
        snprintf(filename, sizeof(filename), "%s/%s.csv", parking_data_folder, datestr);
        
        // Create header if file doesn't exist
        if (!SD.exists(filename)) {
            writeFile(SD, filename, "index,date,time,distance(cm),millis,notification\n");
        }
        
        // Create data line with actual time values
        strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
        snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,1\n", value, datestr, timestr, cm, lastEventTime);
    }
    
    Serial.print("Creating/using file: ");
    Serial.println(filename);

    // Append to file
    appendFile(SD, filename, line);

    // Print to Serial
    Serial.print("Logged: ");
    Serial.print(line);
}

void logLightValue(int value, bool from_command) {
    char filename[40];
    char line[64];
    
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time - using null values");
        
        // Use default filename when time is not available
        snprintf(filename, sizeof(filename), "%s/no_time.csv", light_data_folder);
        
        // Create header if file doesn't exist
        if (!SD.exists(filename)) {
            writeFile(SD, filename, "index,date,time,light,millis,command\n");
        }
        
        // Create data line with null time values
        snprintf(line, sizeof(line), "%d,NULL,NULL,%d,%d,%d\n", value, lightValue, lastLogTime, from_command);
    } else {
        // Create filename: /YYYY-MM-DD.csv
        strftime(dateDatastr, sizeof(dateDatastr), "%Y-%m-%d", &timeinfo);
        snprintf(filename, sizeof(filename), "%s/%s.csv", light_data_folder, dateDatastr);
        
        // Create header if file doesn't exist
        if (!SD.exists(filename)) {
            writeFile(SD, filename, "index,date,time,light,millis,command\n");
        }
        
        // Create data line with actual time values
        strftime(timeDatastr, sizeof(timeDatastr), "%H:%M:%S", &timeinfo);
        snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,%d\n", value, dateDatastr, timeDatastr, lightValue, lastLogTime, from_command);
    }

    Serial.print("Creating/using file: ");
    Serial.println(filename);

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
    
    // Initialize SD card (after radio is set up)
    connectSDCardModule();
    
    // Initialize time (local only - no NTP for transmitter)
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Show setup complete on display
    displayTransmitterInfo();
    
    // Initial sensor readings
    lastLogTime = millis();
    measureData(log_counter++, false); // Disabled - SD logging
    
    // Start listening for time sync responses
    radio.startReceive();
    
    Serial.println("LoRa Transmitter - Patrol Mode Initiated...");
}

void loop() {
    heltec_loop();
    
    unsigned long currentMillis = millis();
    
    // Handle received packets (time sync)
    if (rxFlag) {
        rxFlag = false;
        handleReceivedPacket();
        
        // Resume listening after handling packet
        radio.startReceive();
    }
    
    // Request time sync periodically if not received yet or if it's been a while
    if ((!timeReceived || (currentMillis - lastTimeSyncRequest > timeSyncInterval)) && !timeSyncRequested) {
        if (millis() > last_tx + minimum_pause) {  // Respect duty cycle
            String timeSyncRequest = createTimeSyncRequest();
            if (transmitData(timeSyncRequest)) {
                Serial.println("Time sync request sent");
                lastTimeSyncRequest = currentMillis;
                timeSyncRequested = true; // Prevent continuous requests
            } else {
                Serial.println("Time sync request failed");
            }
            // Resume listening after transmission
            radio.startReceive();
        }
    }
    
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
        
        // Resume listening after transmission
        radio.startReceive();
    }
    
    // Manual transmission on button press only (no periodic transmission)
    if (button.isSingleClick()) {
        String data = createDataPacket();
        show_transmission(counter, data);
        
        if (transmitData(data)) {
            show_transmission_success();
        } else {
            show_transmission_fail(-1);
        }
        
        counter++;
        
        // Resume listening after transmission
        radio.startReceive();
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