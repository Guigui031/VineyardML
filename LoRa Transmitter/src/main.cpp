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
}
