#include <WiFi.h>
#include <WebServer.h>
// #include "esp_camera.h"
#include "esp_http_server.h"
#include "driver/i2s.h"
#include <math.h>

// ===== Wi-Fi Credentials =====
const char* ssid     = "THINK_NET";
const char* password = "TVWn1TEurgH5J";

// // ===== Camera Pins (XIAO ESP32-S3 Sense) =====
// void init_camera() {
//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer   = LEDC_TIMER_0;
//   config.pin_d0       = 15;
//   config.pin_d1       = 17;
//   config.pin_d2       = 18;
//   config.pin_d3       = 16;
//   config.pin_d4       = 14;
//   config.pin_d5       = 12;
//   config.pin_d6       = 11;
//   config.pin_d7       = 48;
//   config.pin_xclk     = 10;
//   config.pin_pclk     = 13;
//   config.pin_vsync    = 38;
//   config.pin_href     = 47;
//   config.pin_sccb_sda = 40;
//   config.pin_sccb_scl = 39;
//   config.pin_pwdn     = -1;
//   config.pin_reset    = -1;
//   config.xclk_freq_hz = 20000000;
//   config.pixel_format = PIXFORMAT_JPEG;
//   config.frame_size   = FRAMESIZE_QVGA;
//   config.jpeg_quality = 12;
//   config.fb_count     = 2;

//   if (esp_camera_init(&config) != ESP_OK) {
//     Serial.println("[CAM] Camera init failed!");
//     while (1);
//   }
//   Serial.println("[CAM] Camera ready.");
// }

// ===== Audio (I2S PDM Mic) =====
#define SAMPLE_RATE 16000
#define I2S_PORT    I2S_NUM_0
#define I2S_PIN_CLK 42  // BCLK
#define I2S_PIN_DATA 41 // DOUT

void init_audio() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_PIN_CLK,
      .ws_io_num = -1,
      .data_out_num = -1,
      .data_in_num = I2S_PIN_DATA,
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  Serial.println("[AUDIO] Mic ready.");
}

// // ===== MJPEG Stream Handler =====
// static esp_err_t stream_handler(httpd_req_t *req) {
//   camera_fb_t *fb = NULL;
//   esp_err_t res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
//   if (res != ESP_OK) return res;

//   while (true) {
//     fb = esp_camera_fb_get();
//     if (!fb) {
//       res = ESP_FAIL;
//     } else {
//       char part_buf[64];
//       snprintf(part_buf, 64,
//                "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
//                fb->len);
//       res = httpd_resp_send_chunk(req, part_buf, strlen(part_buf));
//       if (res == ESP_OK)
//         res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
//       if (res == ESP_OK)
//         res = httpd_resp_send_chunk(req, "\r\n", 2);
//       esp_camera_fb_return(fb);
//     }
//     if (res != ESP_OK) break;
//     vTaskDelay(30 / portTICK_PERIOD_MS);
//   }
//   return res;
// }

// ===== Audio JSON Handler =====
static esp_err_t audio_handler(httpd_req_t *req) {
  int16_t buffer[256];
  size_t bytes_read;
  i2s_read(I2S_PORT, (void *)buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

  long sum = 0;
  int samples = bytes_read / 2;
  for (int i = 0; i < samples; i++) {
    sum += abs(buffer[i]);
  }
  int avg = sum / samples;

  char response[64];
  snprintf(response, sizeof(response), "{\"volume\": %d}", avg);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, response, strlen(response));
}

// ===== Webpage for Oscillating Lines =====
static const char PROGMEM html_page[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Audio Visualizer</title></head>
<body>
<h2>Audio Oscilloscope</h2>
<canvas id="wave" width="400" height="100" style="border:1px solid black;"></canvas>
<script>
let ctx=document.getElementById('wave').getContext('2d');
setInterval(async()=>{
 let res=await fetch('/audio');
 let data=await res.json();
 let vol=data.volume/100;
 ctx.fillStyle="white"; ctx.fillRect(0,0,400,100);
 ctx.strokeStyle="green"; ctx.beginPath();
 for(let x=0;x<400;x++){
   let y=50+Math.sin(x/10+Date.now()/200)*vol;
   ctx.lineTo(x,y);
 }
 ctx.stroke();
},200);
</script>
</body>
</html>
)rawliteral";

static esp_err_t page_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html_page, strlen(html_page));
}

// ===== Start Server =====
void startServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK) {
    // Note: stream_handler (camera) is omitted here because camera init
    // was left commented in this sketch. We still expose the audio/json
    // endpoint and the visualiser page.
    // httpd_uri_t stream_uri = { .uri = "/", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
    httpd_uri_t audio_uri  = { .uri = "/audio", .method = HTTP_GET, .handler = audio_handler, .user_ctx = NULL };
    httpd_uri_t page_uri   = { .uri = "/wave", .method = HTTP_GET, .handler = page_handler, .user_ctx = NULL };
    // httpd_register_uri_handler(server, &stream_uri);
    httpd_register_uri_handler(server, &audio_uri);
    httpd_register_uri_handler(server, &page_uri);
  }
}

// ===== WiFi Connect with AP Fallback =====
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Connecting");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500); Serial.print(".");
    retries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("[WiFi] STA failed, starting AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Cam", "12345678");
    Serial.print("[WiFi] AP IP: "); Serial.println(WiFi.softAPIP());
    return false;
  }
}

// ===== MAIN =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[SETUP] Starting...");

  connectWiFi();
  // init_camera(); // camera init left commented out for now
  init_audio();
  startServer();
}

// Simple serial audio test: periodically read I2S buffer and print average
// absolute sample value. Open Serial Monitor at 115200 and tap the mic to
// see values change.
void loop() {
  int16_t buffer[256];
  size_t bytes_read = 0;
  esp_err_t res = i2s_read(I2S_PORT, (void *)buffer, sizeof(buffer), &bytes_read, 100 / portTICK_PERIOD_MS);
  if (res == ESP_OK && bytes_read > 0) {
    int samples = bytes_read / sizeof(int16_t);
    long long sum = 0;
    unsigned long long sumSq = 0;
    int16_t minv = 32767;
    int16_t maxv = -32768;
    for (int i = 0; i < samples; i++) {
      int16_t v = buffer[i];
      sum += v;
      sumSq += (unsigned long long)((long long)v * (long long)v);
      if (v < minv) minv = v;
      if (v > maxv) maxv = v;
    }
    double mean = (double)sum / (double)samples;
    double e2 = (double)sumSq / (double)samples; // E[x^2]
    double rms = sqrt(e2);
    // RMS with DC removed using var = E[x^2] - (E[x])^2
    double rms_no_dc = sqrt(max(0.0, e2 - mean * mean));
    int peak_to_peak = (int)maxv - (int)minv;
    // Convert to dBFS relative to full-scale 16-bit signed
    double dbfs = -120.0; // default very small
    if (rms_no_dc > 0.0) {
      dbfs = 20.0 * log10(rms_no_dc / 32767.0);
    }
    Serial.printf("[AUDIO] mean=%.1f rms=%.1f rms_no_dc=%.1f p2p=%d dbfs=%.1f samples=%d\r\n",
                  mean, rms, rms_no_dc, peak_to_peak, dbfs, samples);
  } else {
    // No data read; print a short heartbeat so user knows sketch is running
    static unsigned long last = 0;
    if (millis() - last > 1000) {
      Serial.println("[AUDIO] waiting for data...");
      last = millis();
    }
  }
  delay(250);
}
