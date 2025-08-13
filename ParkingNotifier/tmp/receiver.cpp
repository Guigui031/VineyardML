/**
 * LoRa Receiver - Parking Notifier System
 * 
 * This receiver listens for vehicle detection packets from the transmitter
 * and sends Telegram notifications when cars are detected.
 */

#include <Arduino.h>

#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"

#include <heltec_unofficial.h>
#include "common.h"
#include "radio.h"
#include "display.h"
#include "notifications.h"



void setup() {
  heltec_setup();
  
  // Initialize display
  init_display();
  
  // Show radio initialization on display
  display.clear();
  display.drawString(0, 0, "Radio init...");
  display.display();
  
  // Initialize LoRa radio for reception
  setupReceiver();
  
  // Show receiver ready on display
  displayReceiverInfo();
  
  // Connect to WiFi and Telegram
  connectwifi();
  connectTelegramBot();
  
  Serial.println("LoRa Receiver - Setup complete. Listening for packets...");
}

void loop() {
  heltec_loop();
  
  // Process received LoRa packets
  if (rxFlag) {
    rxFlag = false;
    
    // Read the received data
    int state = radio.readData(rxdata);
    
    if (state == RADIOLIB_ERR_NONE) {
      // Successful reception
      packets_received++;
      last_packet_time = millis();
      last_rssi = radio.getRSSI();
      last_snr = radio.getSNR();
      
      // Print packet info to serial
      Serial.println("=== PACKET RECEIVED ===");
      Serial.printf("Data: %s\n", rxdata.c_str());
      Serial.printf("RSSI: %.2f dBm\n", last_rssi);
      Serial.printf("SNR: %.2f dB\n", last_snr);
      Serial.printf("Packet #: %d\n", packets_received);
      Serial.printf("Length: %d bytes\n", rxdata.length());
      Serial.println();
      
      // Display received data on screen
      displayReceivedData();
      
      // Parse the received data
      parseReceivedData(rxdata);
      
      // Check if car detection notification should be sent
      if (notificationsEnabled && rxdata.indexOf("\"car\":true") != -1) {
        sendCarDetectionNotification();
      }
      
    } else {
      // Reception failed
      Serial.printf("RX failed, code %d\n", state);
      
      display.clear();
      display.drawString(0, 0, "RX Error!");
      display.drawString(0, 10, "Code: " + String(state));
      display.display();
    }
    
    // Start receiving again
    startReceiving();
  }
  
  // Update display with receiver status periodically
  static uint64_t last_display_update = 0;
  if (millis() - last_display_update > 3000) {
    last_display_update = millis();
    
    // If no recent packet, show listening status
    if (millis() - last_packet_time > 5000) {
      displayReceiverStatus();
    }
  }
  
  // Button press shows detailed stats
  if (button.isSingleClick()) {
    showDetailedStats();
  }
  
  // Process Telegram commands
  processTelegramCommands();
}

