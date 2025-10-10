#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <U8x8lib.h>
#include <Wire.h>

// ===================== SETTINGS =====================
const char* ROLE = "B";  // "A" on sender, "B" on this display device

const char* WIFI_SSID = "{PLACE YOUR WIFI SSID HERE}";
const char* WIFI_PASS = "{PLACE YOUR WIFI PASSWORD HERE}";

const char* MQTT_HOST = "{PLACE YOUR MQTT HOST HERE}";  // e.g., a2xxxxxx-ats.iot.us-east-1.amazonaws.com
const uint16_t MQTT_PORT = 8883;

static const int BUTTON_PIN = D1;   // safe GPIO
static const int LED_PIN    = LED_BUILTIN;

// ===================== TEXT DISPLAY =====================
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(SCL, SDA, -1);

// ===================== MQTT CREDENTIALS =====================
struct MqttCredentials {
  const char* user;
  const char* pass;
  const char* topic_pub;
  const char* topic_sub;
} creds;

void setupCredentials() {
  if (strcmp(ROLE, "A") == 0) {
    creds.user      = "deviceA";
    creds.pass      = "deviceA@1234";
    creds.topic_pub = "deviceA";
    creds.topic_sub = "deviceB";
  } else {
    creds.user      = "deviceB";
    creds.pass      = "deviceB@1234";
    creds.topic_pub = "deviceB";
    creds.topic_sub = "deviceA";
  }
}

// ===================== MQTT =====================
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

// ===================== DISPLAY HELPER =====================
void displayTextMessage(const String &type, const String &text) {
  u8x8.clearDisplay();
  
  if (type == "EMERGENCY") {
    u8x8.setFont(u8x8_font_chroma48medium8_r); // bold font
  } else {
    u8x8.setFont(u8x8_font_chroma48medium8_r); // normal font
  }

  // Print text in multiple lines if needed
  int maxLen = 16; // 16 chars per line on 128px display
  int start = 0;
  int row = 1; // starting row
  while (start < text.length() && row < 8) {
    String line = text.substring(start, start + maxLen);
    u8x8.setCursor(0, row++);
    u8x8.print(line);
    start += maxLen;
  }
}

// ===================== MQTT CALLBACK =====================
void onMqtt(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.printf("RX [%s] : %s\n", topic, msg.c_str());

  // Parse JSON manually (simpler than including ArduinoJson)
  int typeIdx = msg.indexOf("\"type\":\"");
  int textIdx = msg.indexOf("\"text\":\"");

  if (typeIdx >= 0 && textIdx >= 0) {
    int typeStart = typeIdx + 8;
    int typeEnd = msg.indexOf("\"", typeStart);
    String type = msg.substring(typeStart, typeEnd);

    int textStart = textIdx + 8;
    int textEnd = msg.indexOf("\"", textStart);
    String text = msg.substring(textStart, textEnd);

    displayTextMessage(type, text);
  }
}

// ===================== WIFI & MQTT HELPERS =====================
void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to Wi-Fi ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (millis()-t0 > 20000) { WiFi.disconnect(); WiFi.reconnect(); t0 = millis(); }
  }
  Serial.print("\nWi-Fi connected, IP: "); Serial.println(WiFi.localIP());
}

void ensureMqtt() {
  if (mqtt.connected()) return;

  tlsClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);

  String clientId = String("display-") + ROLE + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.print("Connecting to MQTT...");
  while (!mqtt.connect(clientId.c_str(), creds.user, creds.pass)) { Serial.print("."); delay(1000); }
  Serial.println(" connected");

  mqtt.subscribe(creds.topic_sub);
  Serial.printf("Subscribed to topic: %s\n", creds.topic_sub);
}

// ===================== BUTTON HANDLING =====================
bool lastButton = false;
uint32_t lastSendMs = 0;
const uint32_t SEND_GAP_MS = 1000;

void handleButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  uint32_t now = millis();
  if (pressed && !lastButton) {
    if (mqtt.connected() && now - lastSendMs > SEND_GAP_MS) {
      const char* payload = "{\"type\":\"pulse\"}";
      bool ok = mqtt.publish(creds.topic_pub, payload);
      Serial.printf("TX %s : %s -> %s\n", creds.topic_pub, payload, ok ? "OK" : "FAILED");

      // Optional LED blink
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);

      displayTextMessage("MESSAGE", "PULSE SENT!");
      lastSendMs = now;
    }
  }
  lastButton = pressed;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== DEVICE BOOT ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  u8x8.begin();
  u8x8.setFlipMode(1);
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  setupCredentials();
  ensureWifi();
  ensureMqtt();

  displayTextMessage("MESSAGE", "IDLE...");
}

// ===================== LOOP =====================
void loop() {
  ensureWifi();
  ensureMqtt();
  mqtt.loop();
  handleButton();
  delay(10);
}
