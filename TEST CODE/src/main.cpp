#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ----- Sound-Setup -----
#define SPEAKER_PIN A3 // G  (achte darauf, dass der gewählte Pin auf deinem Board für tone() geeignet ist)

// Alarm-Sound-Variablen
unsigned long lastAlarmToggle = 0;
bool alarmToneHigh = true;
unsigned long alarmStartTime = 0;
bool alarmActive = false;

// Alarm-Frequenzen für dramatischen Effekt
const int ALARM_FREQ_HIGH = 1200;  // Hohe Frequenz
const int ALARM_FREQ_LOW = 800;    // Tiefe Frequenz
const int ALARM_TOGGLE_INTERVAL = 150; // Schneller Wechsel für Dringlichkeit

void playAlarmSound() {
  if (!alarmActive) return;
  
  unsigned long currentTime = millis();
  
  // Schnell zwischen hohen und tiefen Tönen wechseln
  if (currentTime - lastAlarmToggle >= ALARM_TOGGLE_INTERVAL) {
    noTone(SPEAKER_PIN);
    
    if (alarmToneHigh) {
      tone(SPEAKER_PIN, ALARM_FREQ_HIGH, ALARM_TOGGLE_INTERVAL - 10);
    } else {
      tone(SPEAKER_PIN, ALARM_FREQ_LOW, ALARM_TOGGLE_INTERVAL - 10);
    }
    
    alarmToneHigh = !alarmToneHigh;
    lastAlarmToggle = currentTime;
  }
}

void startAlarm() {
  alarmActive = true;
  alarmStartTime = millis();
  lastAlarmToggle = millis();
  Serial.println("🚨 ALARM GESTARTET!");
}

void stopAlarm() {
  alarmActive = false;
  noTone(SPEAKER_PIN);
  Serial.println("🔇 Alarm gestoppt.");
}

// Wartet ms Millisekunden und spielt dabei Alarm-Sound (ohne blocking delays)
void waitWithAlarm(uint32_t ms) {
  unsigned long startTime = millis();
  startAlarm();
  
  Serial.println("🚨 Alarm läuft für 10 Sekunden...");
  
  while (millis() - startTime < ms) {
    playAlarmSound();
    // Minimale Pause für Watchdog, ohne den Sound zu unterbrechen
    yield(); 
  }
  
  stopAlarm();
}

// ----- Netzwerk-Setup -----
constexpr const char* WIFI_SSID     = "THINK_NET";
constexpr const char* WIFI_PASSWORD = "TVWn1TEurgH5J";

// Dein Webhook:
constexpr const char* WEBHOOK_URL = "https://naminatorasdf.app.n8n.cloud/webhook-test/9ff80410-7345-45c5-9231-e25e10c27e0d";

// Optional: Timeout-Parameter
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000; // 20s

bool connectWiFi() {
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  unsigned long lastDot = millis();
  
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    if (millis() - lastDot >= 250) {
      Serial.print(".");
      lastDot = millis();
    }
    yield(); // Nicht-blockierend, gibt CPU frei
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WLAN verbunden. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WLAN-Verbindung fehlgeschlagen.");
    return false;
  }
}

int sendGetRequest(const char* url) {
  // Für HTTPS:
  WiFiClientSecure secureClient;
  secureClient.setInsecure();  // akzeptiert jedes Zertifikat (für Produktion besser Fingerprint/Root-CA nutzen)

  HTTPClient http;
  Serial.print("Sende GET an: ");
  Serial.println(url);

  if (!http.begin(secureClient, url)) {
    Serial.println("HTTP begin() fehlgeschlagen.");
    return -1;
  }

  int httpCode = http.GET();  // Request senden
  if (httpCode > 0) {
    Serial.printf("HTTP Status: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("Antwort:");
    Serial.println(payload);
  } else {
    Serial.printf("HTTP Fehler: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return httpCode;
}

void setup() {
  Serial.begin(115200);
  // Kurze Pause für Serial-Initialisierung (nicht-blockierend)
  unsigned long initStart = millis();
  while (millis() - initStart < 20) yield();

  if (!connectWiFi()) {
    // Wenn kein WLAN, später nochmal versuchen (nicht-blockierend)
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("WLAN neu versuchen in 5s …");
      unsigned long retryStart = millis();
      while (millis() - retryStart < 5000) {
        yield(); // CPU freigeben während Wartezeit
      }
      connectWiFi();
    }
  }

  // Erst Webhook-Request senden
  Serial.println("Sende Webhook-Request...");
  int code = sendGetRequest(WEBHOOK_URL);
  Serial.printf("Webhook-Request abgeschlossen (Code: %d).\n", code);

  // Dann 5 Sekunden warten und Alarm abspielen
  Serial.println("Warte 5 Sekunden, dann Alarm für 10 Sekunden...");
  unsigned long waitStart = millis();
  while (millis() - waitStart < 5000) {
    yield(); // Nicht-blockierende Wartezeit
  }
  
  waitWithAlarm(10000);
}

void loop() {
  // Nichts weiter zu tun.
  // Optional: Deep Sleep oder periodisch erneut senden.
  
  // Nicht-blockierende Pause
  static unsigned long lastLoop = millis();
  if (millis() - lastLoop >= 1000) {
    lastLoop = millis();
    // Hier könnte periodisches Verhalten stehen
  }
  yield(); // CPU freigeben
}
