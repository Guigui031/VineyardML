//TEST CODE

#include "RTC.h"
#include <WiFiS3.h>
#include <WiFi.h>

#include <SoftwareSerial.h>
#include <ESPAsyncWebServer.h>

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;


int testHour = 12, timeUpdateHour = 9;
bool testSent = false, timeUpdated = false;
bool sendMessage = true; // send a notification on powerup. 
unsigned long unixTime;
unsigned long unixTimeLastSend = 0;
unsigned long minutesSinceLastSend;
unsigned long minMinutesBetweenSends = 15;

WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

#define WIFI_SSID "genois"
#define WIFI_PASS "master13" // not needed with MAC address filtering

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP DS18B20 Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATUREC"){
    return temperatureC;
  }
  else if(var == "TEMPERATUREF"){
    return temperatureF;
  }
  return String();
}

void setup(){

  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  //  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  connectwifi();
  WiFi.disconnect();
  delay(5000);
  Serial.println("Patrol Mode Initiated...");
  
  // Start up the DS18B20 library
  // sensors.begin();

  temperatureC = 1234;
  temperatureF = 5678;

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureC.c_str());
  });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureF.c_str());
  });
  // Start server
  server.begin();
}
 
void loop(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(1);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  inches = (duration / 2) / 74;
  Serial.println(inches);
  /*
  if (inches < 20 || inches > 500) {
    Serial.println("Intruder Detected!");
    Serial.println("Sending text Notification...");
    delay(5000);
    Serial.println("Patrol Mode Initiated...");
  }
  */
  if ((millis() - lastTime) > timerDelay) {
    temperatureC = 1234;
    temperatureF = 5678;
    lastTime = millis();
  }  
}

void connectwifi(){
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // WiFi.begin(WIFI_SSID) or WiFi.begin(WIFI_SSID, WIFI_PASS); depending on if password is necessary
  Serial.print("Connecting to Wi-Fi.");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.print(" Connected with IP: ");
  Serial.println(WiFi.localIP());
}
