#include "display.h"

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