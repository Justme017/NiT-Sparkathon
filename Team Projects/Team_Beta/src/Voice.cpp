#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <ArduinoJson.h>
#include <base64.h>

// ===================== USER SETTINGS =====================

const char* ROLE = "A";  // change to "B" on the second board

const char* WIFI_SSID = "{PLACE YOUR WIFI SSID HERE}";
const char* WIFI_PASS = "{PLACE YOUR WIFI PASSWORD HERE}";

const char* MQTT_HOST = "{PLACE YOUR MQTT HOST HERE}";  // e.g., a2xxxxxx-ats.iot.us-east-1.amazonaws.com
const uint16_t MQTT_PORT = 8883;

// Google API Key
const char* GOOGLE_API_KEY = "{PLACE YOUR GOOGLE API KEY HERE}";

// Custom names (family/pets) - customize these!
const char* CUSTOM_NAMES[] = {"mom", "dad", "sarah", "max", "luna", "tommy", "emma", "grandma", "grandpa"};
const int CUSTOM_NAMES_COUNT = 9;

// GPIOs
static const int BUTTON_PIN = 4;   // Tap button (active LOW)
static const int DND_PIN    = 3;   // DND toggle button (active LOW) / 4 clicks for recording
static const int HAPTIC_PIN = 2;   // Motor/LED driver

// I2S pins
#define I2S_WS 42
#define I2S_SD 41
#define I2S_SCK -1

#define SAMPLE_RATE 16000
#define RECORD_TIME 3  // seconds
#define BUFFER_SIZE (SAMPLE_RATE * RECORD_TIME * 2)

// ================= END USER SETTINGS =====================

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

// ---- Globals ----
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

uint8_t* audioBuffer = nullptr;

uint32_t lastSendMs   = 0;
uint32_t lastHapticMs = 0;
uint32_t dndPressStart = 0;

const uint32_t SEND_GAP_MS     = 1000;
const uint32_t HAPTIC_MS       = 150;
const uint32_t HAPTIC_LOCK_MS  = 1000;
const uint32_t DND_HOLD_MS     = 2000;
const uint32_t CLICK_TIMEOUT   = 500;  // Max time between clicks

bool dndState = false;

// Click detection variables
uint8_t clickCount = 0;
uint32_t lastClickTime = 0;
bool lastDndState = HIGH;

void updateDndLed() {
  digitalWrite(LED_BUILTIN, dndState ? HIGH : LOW);
}

void vibrateOnce() {
  if (millis() - lastHapticMs < HAPTIC_LOCK_MS) return;
  digitalWrite(HAPTIC_PIN, HIGH);
  delay(HAPTIC_MS);
  digitalWrite(HAPTIC_PIN, LOW);
  lastHapticMs = millis();
}

void onMqtt(char* topic, byte* payload, unsigned int len) {
  Serial.print("RX "); Serial.print(topic); Serial.print(" : ");
  Serial.write(payload, len); Serial.println();

  if (!dndState) {
    vibrateOnce();
  } else {
    Serial.println("DND active -> ignoring pulse");
  }
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi: ");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (millis() - t0 > 20000) {
      Serial.println(" timeout, retrying");
      t0 = millis();
    }
  }
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());
}

void ensureMqtt() {
  if (mqtt.connected()) return;

  tlsClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);

  String clientId = String("bondbit-") + ROLE + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.print("MQTT: ");
  while (!mqtt.connected()) {
    Serial.print(".");
    if (mqtt.connect(clientId.c_str(), creds.user, creds.pass)) break;
    delay(1000);
  }
  Serial.println(" connected");

  mqtt.subscribe(creds.topic_sub);
  Serial.print("SUB "); Serial.println(creds.topic_sub);
  Serial.print("PUB "); Serial.println(creds.topic_pub);
}

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void recordAudio() {
  Serial.println("Recording...");
  digitalWrite(LED_BUILTIN, HIGH);  // LED on during recording
  
  size_t bytesRead = 0;
  size_t totalBytesRead = 0;
  
  while (totalBytesRead < BUFFER_SIZE) {
    i2s_read(I2S_NUM_0, audioBuffer + totalBytesRead, 
             BUFFER_SIZE - totalBytesRead, &bytesRead, portMAX_DELAY);
    totalBytesRead += bytesRead;
  }
  
  digitalWrite(LED_BUILTIN, dndState ? HIGH : LOW);  // Restore LED state
  Serial.println("Recording complete!");
}

String sendToGoogle() {
  Serial.println("Processing...");
  
  String audioBase64 = base64::encode(audioBuffer, BUFFER_SIZE);
  
  String jsonRequest = "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":" + 
                       String(SAMPLE_RATE) + 
                       ",\"languageCode\":\"en-US\"},\"audio\":{\"content\":\"" + 
                       audioBase64 + 
                       "\"}}";
  
  HTTPClient http;
  http.setTimeout(15000);
  
  String url = "https://speech.googleapis.com/v1/speech:recognize?key=" + String(GOOGLE_API_KEY);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonRequest);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    
    DynamicJsonDocument responseDoc(4096);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error && responseDoc.containsKey("results")) {
      const char* transcript = responseDoc["results"][0]["alternatives"][0]["transcript"];
      Serial.println("\nYou said: " + String(transcript) + "\n");
      http.end();
      return String(transcript);
    } else if (responseDoc.containsKey("error")) {
      Serial.println("Error: " + String(responseDoc["error"]["message"].as<const char*>()));
    }
  } else {
    Serial.println("HTTP Error: " + String(httpResponseCode));
  }
  
  http.end();
  return "";
}

String classifyMessage(String text) {
  text.toLowerCase();
  
  Serial.println("\n=== MESSAGE CLASSIFICATION ===");
  Serial.println("Text: " + text);
  
  // Priority 1: ALERT (highest priority)
  if (text.indexOf("fall") >= 0 || text.indexOf("fallen") >= 0 || text.indexOf("hurt") >= 0 || 
      text.indexOf("pain") >= 0 || text.indexOf("help") >= 0 || text.indexOf("emergency") >= 0 ||
      text.indexOf("accident") >= 0 || text.indexOf("injured") >= 0 || text.indexOf("bleeding") >= 0 ||
      text.indexOf("dizzy") >= 0 || text.indexOf("chest pain") >= 0 || text.indexOf("can't breathe") >= 0 ||
      text.indexOf("faint") >= 0 || text.indexOf("ambulance") >= 0 || text.indexOf("hospital") >= 0 ||
      text.indexOf("danger") >= 0 || text.indexOf("scared") >= 0 || text.indexOf("unsafe") >= 0 ||
      text.indexOf("threatened") >= 0 || text.indexOf("lost") >= 0 || text.indexOf("stuck") >= 0 ||
      text.indexOf("trapped") >= 0 || text.indexOf("attack") >= 0 || text.indexOf("fire") >= 0 ||
      text.indexOf("police") >= 0 || text.indexOf("sos") >= 0 || text.indexOf("urgent") >= 0) {
    Serial.println("Category: ALERT");
    return "EMERGENCY";
  }
  
  // Priority 2: CALL ME
  if (text.indexOf("call me") >= 0 || text.indexOf("phone me") >= 0 || text.indexOf("ring me") >= 0 ||
      text.indexOf("can you call") >= 0 || text.indexOf("need to talk") >= 0 || text.indexOf("talk to me") >= 0 ||
      text.indexOf("can we talk") >= 0 || text.indexOf("free to talk") >= 0) {
    Serial.println("Category: CALL REQUEST");
    return "CALL_REQUEST";
  }
  
  // Priority 3: MISS YOU
  if (text.indexOf("miss you") >= 0 || text.indexOf("missing you") >= 0 || text.indexOf("thinking of you") >= 0 ||
      text.indexOf("wish you were here") >= 0 || text.indexOf("can't wait to see you") >= 0 ||
      text.indexOf("want to see you") >= 0 || text.indexOf("come home") >= 0 || text.indexOf("lonely") >= 0) {
    Serial.println("Category: MISS YOU");
    return "MISS_YOU";
  }
  
  // Priority 4: LOVE
  if (text.indexOf("love you") >= 0 || text.indexOf("love ya") >= 0 || text.indexOf("i love") >= 0 ||
      text.indexOf("adore you") >= 0 || text.indexOf("cherish you") >= 0 || text.indexOf("my love") >= 0 ||
      text.indexOf("sweetheart") >= 0 || text.indexOf("honey") >= 0 || text.indexOf("darling") >= 0) {
    Serial.println("Category: LOVE");
    return "LOVE";
  }
  
  // Priority 5: CELEBRATION
  if (text.indexOf("congratulations") >= 0 || text.indexOf("congrats") >= 0 || text.indexOf("proud") >= 0 ||
      text.indexOf("achievement") >= 0 || text.indexOf("success") >= 0 || text.indexOf("great news") >= 0 ||
      text.indexOf("good news") >= 0 || text.indexOf("excited") >= 0 || text.indexOf("celebration") >= 0 ||
      text.indexOf("party") >= 0 || text.indexOf("birthday") >= 0 || text.indexOf("anniversary") >= 0 ||
      text.indexOf("amazing") >= 0 || text.indexOf("wonderful") >= 0) {
    Serial.println("Category: CELEBRATION");
    return "CELEBRATE";
  }
  
  // Priority 6: COMFORT
  if (text.indexOf("sad") >= 0 || text.indexOf("crying") >= 0 || text.indexOf("upset") >= 0 ||
      text.indexOf("depressed") >= 0 || text.indexOf("terrible day") >= 0 || text.indexOf("bad day") >= 0 ||
      text.indexOf("awful") >= 0 || text.indexOf("difficult") >= 0 || text.indexOf("hard time") >= 0 ||
      text.indexOf("struggle") >= 0 || text.indexOf("exhausted") >= 0 || text.indexOf("overwhelmed") >= 0 ||
      text.indexOf("stressed") >= 0 || text.indexOf("anxious") >= 0 || text.indexOf("worried") >= 0) {
    Serial.println("Category: COMFORT");
    return "COMFORT";
  }
  
  // Priority 7: GOODNIGHT
  if (text.indexOf("goodnight") >= 0 || text.indexOf("good night") >= 0 || text.indexOf("sleep well") >= 0 ||
      text.indexOf("sweet dreams") >= 0 || text.indexOf("bedtime") >= 0 || text.indexOf("going to sleep") >= 0 ||
      text.indexOf("nighty night") >= 0) {
    Serial.println("Category: GOODNIGHT");
    return "GOODNIGHT";
  }
  
  // Priority 8: GOODMORNING
  if (text.indexOf("good morning") >= 0 || text.indexOf("morning") >= 0 || text.indexOf("wake up") >= 0 ||
      text.indexOf("rise and shine") >= 0 || text.indexOf("have a great day") >= 0 || text.indexOf("beautiful day") >= 0) {
    Serial.println("Category: GOODMORNING");
    return "GOODMORNING";
  }
  
  // Priority 9: THINKING OF YOU
  if (text.indexOf("thinking of you") >= 0 || text.indexOf("on my mind") >= 0 || text.indexOf("remembered you") >= 0 ||
      text.indexOf("thought of you") >= 0 || text.indexOf("how are you") >= 0 || text.indexOf("checking in") >= 0 ||
      text.indexOf("hope you're okay") >= 0 || text.indexOf("sending love") >= 0 || text.indexOf("hug") >= 0) {
    Serial.println("Category: THINKING OF YOU");
    return "THINKING";
  }
  
  // Priority 10: COMING HOME
  if (text.indexOf("coming home") >= 0 || text.indexOf("on my way") >= 0 || text.indexOf("be there soon") >= 0 ||
      text.indexOf("almost there") >= 0 || text.indexOf("arriving") >= 0 || text.indexOf("heading home") >= 0 ||
      text.indexOf("see you soon") >= 0 || text.indexOf("leaving now") >= 0 || text.indexOf("just left") >= 0) {
    Serial.println("Category: COMING HOME");
    return "COMING_HOME";
  }
  
  // Priority 11: MEAL TIME
  if (text.indexOf("dinner ready") >= 0 || text.indexOf("lunch ready") >= 0 || text.indexOf("breakfast ready") >= 0 ||
      text.indexOf("food's ready") >= 0 || text.indexOf("let's eat") >= 0 || text.indexOf("meal time") >= 0) {
    Serial.println("Category: MEAL TIME");
    return "MEAL_TIME";
  }
  
  // Priority 12: AFFIRMATIVE
  if (text.indexOf("yes") >= 0 || text.indexOf("okay") >= 0 || text.indexOf(" ok ") >= 0 ||
      text.indexOf("sure") >= 0 || text.indexOf("alright") >= 0 || text.indexOf("sounds good") >= 0 ||
      text.indexOf("perfect") >= 0 || text.indexOf("agreed") >= 0 || text.indexOf("confirm") >= 0) {
    Serial.println("Category: AFFIRMATIVE");
    return "CONFIRMED";
  }
  
  // Priority 13: NEGATIVE
  if (text.indexOf("no") >= 0 || text.indexOf("can't") >= 0 || text.indexOf("cannot") >= 0 ||
      text.indexOf("busy") >= 0 || text.indexOf("not now") >= 0 || text.indexOf("later") >= 0 ||
      text.indexOf("unable") >= 0 || text.indexOf("running late") >= 0 || text.indexOf("delayed") >= 0) {
    Serial.println("Category: NEGATIVE");
    return "BUSY";
  }
  
  // Default: GENERAL MESSAGE
  Serial.println("Category: GENERAL MESSAGE");
  return "MESSAGE";
}

String detectCustomNames(String text) {
  text.toLowerCase();
  String detectedNames = "";
  
  for (int i = 0; i < CUSTOM_NAMES_COUNT; i++) {
    if (text.indexOf(CUSTOM_NAMES[i]) >= 0) {
      if (detectedNames.length() > 0) {
        detectedNames += ",";
      }
      detectedNames += CUSTOM_NAMES[i];
    }
  }
  
  if (detectedNames.length() > 0) {
    Serial.println("Detected Names: " + detectedNames);
  }
  
  return detectedNames;
}

void sendClassifiedMessage(String transcript) {
  String category = classifyMessage(transcript);
  String names = detectCustomNames(transcript);
  
  // Build MQTT payload
  String payload = "{\"type\":\"" + category + "\"";
  if (names.length() > 0) {
    payload += ",\"names\":\"" + names + "\"";
  }
  payload += ",\"text\":\"" + transcript + "\"}";
  
  Serial.println("\n=== MQTT PUBLISH ===");
  Serial.println("Topic: " + String(creds.topic_pub));
  Serial.println("Payload: " + payload);
  
  bool success = mqtt.publish(creds.topic_pub, payload.c_str());
  if (success) {
    Serial.println("Status: SUCCESS");
  } else {
    Serial.println("Status: FAILED");
  }
  Serial.println("====================\n");
}

void handleDndButton() {
  bool currentState = digitalRead(DND_PIN);
  uint32_t now = millis();

  // Detect button press (HIGH to LOW transition)
  if (currentState == LOW && lastDndState == HIGH) {
    // Check if this is part of a click sequence
    if (now - lastClickTime < CLICK_TIMEOUT) {
      clickCount++;
    } else {
      clickCount = 1;  // Start new sequence
    }
    lastClickTime = now;
    dndPressStart = now;
    
    Serial.printf("Click %d\n", clickCount);
    
    // 4 clicks detected - start recording
    if (clickCount == 4) {
      clickCount = 0;  // Reset
      recordAudio();
      String text = sendToGoogle();
      if (text.length() > 0) {
        Serial.println("Transcription successful!");
        sendClassifiedMessage(text);
      }
    }
  }
  // Detect button release (LOW to HIGH transition) for long press
  else if (currentState == HIGH && lastDndState == LOW) {
    if (now - dndPressStart >= DND_HOLD_MS && clickCount < 4) {
      dndState = !dndState;
      Serial.print("DND toggled -> ");
      Serial.println(dndState ? "ENABLED" : "DISABLED");
      updateDndLed();
      clickCount = 0;  // Reset clicks
    }
  }

  // Reset click count if timeout exceeded
  if (clickCount > 0 && now - lastClickTime > CLICK_TIMEOUT) {
    clickCount = 0;
  }

  lastDndState = currentState;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.printf("BOOT (ROLE=%s)\n", ROLE);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DND_PIN,    INPUT_PULLUP);
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(HAPTIC_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  // Allocate audio buffer
  audioBuffer = (uint8_t*)ps_malloc(BUFFER_SIZE);
  if (!audioBuffer) {
    audioBuffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!audioBuffer) {
      Serial.println("Failed to allocate audio buffer!");
      while(1) delay(1000);
    }
  }

  setupCredentials();
  ensureWifi();
  ensureMqtt();
  setupI2S();
  
  Serial.println("Ready! 4 clicks on DND button to record audio.\n");
}

void loop() {
  ensureWifi();
  ensureMqtt();
  mqtt.loop();

  handleDndButton();

  // Button press to send pulse
  static bool lastPressed = false;
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if (pressed && !lastPressed) {
    if (millis() - lastSendMs > SEND_GAP_MS) {
      const char* payload = "{\"type\":\"pulse\"}";
      bool ok = mqtt.publish(creds.topic_pub, payload);
      Serial.print("TX "); Serial.print(creds.topic_pub);
      Serial.print(" : "); Serial.println(payload);
      if (!ok) Serial.println("Publish failed");
      lastSendMs = millis();
    }
  }
  lastPressed = pressed;

  delay(10);
}