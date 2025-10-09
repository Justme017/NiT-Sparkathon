#include <I2S.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi
const char* ssid     = "THINK_NET";
const char* password = "TVWn1TEurgH5J";

const char* serverName = "http://172.16.16.81:8000/api/send_status"; //TODO: change

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void setup() {
  // Open serial communications and wait for port to open:
  // A baud rate of 115200 is used instead of 9600 for a faster data rate
  // on non-native USB ports
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected! ESP32 IP: ");
  Serial.println(WiFi.localIP());  // Shows IP in Serial Monitor

  // start I2S at 16 kHz with 16-bits per sample
  I2S.setAllPins(-1, 42, 41, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1); // do nothing
  }
}

void loop() {
  // read a sample
  int avg = 0;

  for (int i = 0; i < 1000; i++) {
    int sample = I2S.read();

    if (sample && sample != -1 && sample != 1) {
      avg += sample;
      //Serial.println(sample);
    }
  }
  avg /= 1000;
  
  if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");

      String httpRequestData;
      // Data to send with HTTP POST
      if (avg < 1200 || avg > 1400) {
        Serial.print("STRESSFUL\n");
        httpRequestData = "{\"status\":1}"; 
      } else {
        Serial.print("CALM     \n");
        httpRequestData = "{\"status\":2}";  
      }
             
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();

  
  
  // Serial.println(avg);
}