#define HELTEC_POWER_BUTTON
#include <Arduino.h>
#include <heltec_unofficial.h>

void setup() {
  // Manual display initialization for clone board
  pinMode(21, OUTPUT);           // Display power
  digitalWrite(21, HIGH);        // Enable display power
  delay(100);
  
  pinMode(18, OUTPUT);           // Reset pin (also SCL)
  digitalWrite(18, LOW);         // Reset display
  delay(10);
  digitalWrite(18, HIGH);        // Release reset
  delay(50);
  
  heltec_setup();
  Serial.println("Serial works");
  
  // Re-initialize display after heltec_setup
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  // Display test
  display.drawString(0, 0, "Display works!");
  
  // Radio test
  display.drawString(0, 10, "Radio ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    display.drawString(40, 10, "works");
  } else {
    display.drawString(40, 10, "fail: " + String(state));
  }
  
  // Battery test
  float vbat = heltec_vbat();
  display.drawString(0, 20, "Vbat: " + String(vbat, 2) + "V");
  display.drawString(0, 30, "(" + String(heltec_battery_percent(vbat)) + "%)");
  
  display.display();  // Show everything
}

void loop() {
  heltec_loop();
  
  // Button test
  if (button.isSingleClick()) {
    display.clear();
    display.drawString(0, 0, "Button works!");
    display.display();
    
    // LED test
    for (int n = 0; n <= 100; n++) { heltec_led(n); delay(5); }
    for (int n = 100; n >= 0; n--) { heltec_led(n); delay(5); }
    
    display.drawString(0, 10, "LED works!");
    display.display();
    
    delay(2000);  // Show message for 2 seconds
  }
}