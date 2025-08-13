/**
 * LoRa Transmitter - Parking Notifier System
 * 
 * This transmitter monitors vehicle presence using ultrasonic sensor
 * and transmits detection events via LoRa to the receiver.
 */


#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"

#include <Arduino.h>
#include <heltec_unofficial.h>
#include "common.h"
#include "radio.h"
#include "display.h"
#include "sensors.h"
#include "sd_card.h"




void setup() {
  heltec_setup();
  
  // Initialize display
  // init_display();
  
  // Show radio initialization on display
  // display.clear();
  // display.drawString(0, 0, "Radio init...");
  // display.display();
  
  // // Initialize LoRa radio for transmission
  // setupTransmitter();
  
  // // Initialize sensors
  // initSensors();
  
  // // Initialize SD card
  // connectSDCardModule();
  
  // // Initialize time
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // // Show setup complete on display
  // displayTransmitterInfo();
  
  // // Initial sensor readings
  // lastLogTime = millis();
  // measureData(log_counter++, false);
  
  // Serial.println("LoRa Transmitter - Patrol Mode Initiated...");
}

void loop() {
  // heltec_loop();
  
  // unsigned long currentMillis = millis();
  
  // // Check for car detection
  // if (checkCarDetection()) {
  //   // Car detected - transmit immediately
  //   String data = createDataPacket();
  //   if (transmitData(data)) {
  //     show_transmission_success();
  //     Serial.println("Car detection transmitted successfully");
  //   } else {
  //     show_transmission_fail(-1);
  //     Serial.println("Car detection transmission failed");
  //   }
    
  //   // Log the event
  //   logEvent(event_counter++);
  //   counter++;
  // }
  
  // // Regular periodic transmission (every PAUSE seconds) or button press
  // bool tx_legal = millis() > last_tx + minimum_pause;
  // if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    
  //   String data = createDataPacket();
  //   show_transmission(counter, data);
    
  //   if (transmitData(data)) {
  //     show_transmission_success();
  //   } else {
  //     show_transmission_fail(-1);
  //   }
    
  //   counter++;
  // }
  
  // // Periodic light measurement logging
  // if (currentMillis - lastLogTime >= logInterval) {
  //   lastLogTime = currentMillis;
  //   measureData(log_counter++, false);
  // }
  
  // // Update display status periodically
  // static uint64_t last_display_update = 0;
  // if (millis() - last_display_update > 5000 && millis() - last_tx > 3000) {
  //   last_display_update = millis();
  //   displayTransmitterStatus();
  // }
}
