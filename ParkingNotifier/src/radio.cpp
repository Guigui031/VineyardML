#include "radio.h"

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

void setupReceiver() {
    initRadio();
    
    // Set the callback function for received packets
    radio.setDio1Action(rx);
    
    // Start receiving immediately
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    
    Serial.println("LoRa Receiver Ready - Listening for packets...");
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

void startReceiving() {
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void rx() {
    rxFlag = true;
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

String createDataPacket() {
    // JSON-like data with multiple values including car detection
    String packet = "{";
    packet += "\"id\":" + String(counter) + ",";
    packet += "\"bat\":" + String(heltec_vbat(), 2) + ",";
    packet += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    packet += "\"time\":" + String(millis()/1000) + ",";
    packet += "\"distance\":" + String(cm) + ",";
    packet += "\"light\":" + String(lightValue) + ",";
    packet += "\"car\":" + String(carPresent ? "true" : "false");
    packet += "}";
    
    return packet;
}