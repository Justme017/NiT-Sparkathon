/* Focus Anchor - XIAO ESP32S3 TinyML Project
 * Integrates MPU6050 HAR with OLED and Haptic Feedback
 */

#include <Arduino.h>
#include <U8x8lib.h>
#include <PCF8563.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- EDGE IMPULSE INCLUDES ---
#include <Parvesh-project-1_inferencing.h> // <-- YOUR GENERATED EI HEADER

// --- HARDWARE CONFIGURATION ---
Adafruit_MPU6050 mpu;
PCF8563 pcf;
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

#define VIB_PIN A0 // Assuming D0 (GPIO0) or another available pin is used for the vibration motor
#define BUTTON_PIN D1

// --- EDGE IMPULSE INFERENCE VARIABLES ---
// EI_CLASSIFIER_FREQUENCY is the sampling rate (e.g., 100.0f)
// EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE is the total number of features (e.g., 720)

// Calculate interval based on the model's expected frequency
const int interval_ms = (int)(1000.0f / EI_CLASSIFIER_FREQUENCY); 
unsigned long last_read_time = 0;

// Buffer to store one full window of IMU data (e.g., 6 axes * 120 timesteps)
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE]; 
int feature_ix = 0; // Index to track current position in the buffer

// --- STATE TRACKING VARIABLES ---
// Counter for how many *consecutive* prediction windows have been classified as 'fidgeting'
int fidget_window_counter = 0;
unsigned long last_warning_reset_time = 0; 

// Thresholds for escalating the warning
const int MILD_THRESHOLD = 5;  // Go to 'fidgeting' for 5 consecutive windows
const int MEDIUM_THRESHOLD = 20; // Go to 'moving_a_lot' for 20 consecutive windows
const int STRONG_THRESHOLD = 40; // Go to 'toomuch' for 40 consecutive windows
const unsigned long HOUR_MS = 3600000UL; 
// --- FUNCTION PROTOTYPES ---
// These are needed because some functions are defined after they are called.
void print_inference_result(ei_impulse_result_t result);
void handle_classification_result(ei_impulse_result_t result);
void vibrate_mild();
void vibrate_medium();
void vibrate_strong();
void welcome_msg();
void fidget_msg();
void relax_msg();
void moving_msg();
void toomuch_msg();
void staystill_msg();
void showdateandtime(Time nowTime);


// --- EI CALLBACK FUNCTION ---
// Function called by the inference library to get data from our buffer.
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// --------------------------------------------------------------------------------

// --- VIBRATION FUNCTIONS ---
void vibrate_mild()
{
    // Note: analogWrite works for PWM output
    analogWrite(VIB_PIN, 100);    // value from 0–255
    delay(5000); // Shorter delay for haptic feedback
    analogWrite(VIB_PIN, 0); 
}

void vibrate_medium()
{
    analogWrite(VIB_PIN, 128);    
    delay(1000);
    analogWrite(VIB_PIN, 0);
}

void vibrate_strong()
{
    analogWrite(VIB_PIN, 255);    
    delay(1000);
    analogWrite(VIB_PIN, 0);
}

// --------------------------------------------------------------------------------

// --- OLED MESSAGE FUNCTIONS ---
void welcome_msg()
{
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Focus Anchor...");
    delay(2000);
    u8x8.clearDisplay();
}

void fidget_msg()
{
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("You are");
    u8x8.setCursor(0, 2);
    u8x8.print("fidgeting");
    delay(2000);
    u8x8.clearDisplay();
}

void relax_msg()
{
    //mild warning
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Relax your leg");
    delay(2000);
    u8x8.clearDisplay();
}

void moving_msg()
{
    //medium warning
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("you are");
    u8x8.setCursor(0, 2);
    u8x8.print("moving a lot");
    delay(2000);
    u8x8.clearDisplay();
}

void toomuch_msg()
{
    //strong warning
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Strong movement!");
    delay(2000);
    u8x8.clearDisplay();
}

void staystill_msg()
{
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Staying Still");
    delay(2000);
    u8x8.clearDisplay();
}

void showdateandtime(Time nowTime)
{
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print(nowTime.day);
    u8x8.print("/");
    u8x8.print(nowTime.month);
    u8x8.print("/");
    u8x8.print("20");
    u8x8.print(nowTime.year);
    u8x8.setCursor(0, 1);
    u8x8.print(nowTime.hour);
    u8x8.print(":");
    u8x8.print(nowTime.minute);
    u8x8.print(":");
    u8x8.println(nowTime.second);
    delay(2000);
    u8x8.clearDisplay();
}

// --------------------------------------------------------------------------------

// --- CLASSIFICATION HANDLER (Triggers Feedback) ---
void handle_classification_result(ei_impulse_result_t result) {
    // Find the class with the highest confidence
    float max_score = 0;
    int max_idx = -1;

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_score) {
            max_score = result.classification[i].value;
            max_idx = i;
        }
    }
    
    // Safety check: require a minimum confidence (e.g., 60%)
    if (max_score < 0.60) {
        staystill_msg(); // Default message if confidence is low
        return;
    }

    const char* predicted_label = ei_classifier_inferencing_categories[max_idx];
    
    // --- Application Logic: Map labels to actions ---
    if (strcmp(predicted_label, "fidgeting") == 0) {
        fidget_msg();
        vibrate_mild();
    } 
    else { // Assume "stillness" or other non-fidgeting classes
        staystill_msg();
    }
 
    
}






// --- CLASSIFICATION HANDLER (Triggers Feedback) ---



// --------------------------------------------------------------------------------

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    Wire.begin(); 
    Serial.println("\nFocus Anchor TinyML System Starting...");

    // --- MPU6050 Setup ---
    u8x8.begin();
    u8x8.setFlipMode(1);
    if (!mpu.begin()) {
        Serial.println("MPU6050 not found. Check wiring!");
        while (1);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
    // --- EI Buffer Setup ---
    memset(features, 0, sizeof(features));

    // --- RTC Setup (Your existing code) ---
    pcf.init();
    pcf.stopClock();
    pcf.setYear(25); pcf.setMonth(10); pcf.setDay(9);
    pcf.setHour(14); pcf.setMinut(10); pcf.setSecond(0);
    pcf.startClock();

    // --- Pin Setup ---
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(VIB_PIN, OUTPUT); 
    pinMode(0,OUTPUT); // Keeping this line, though D0 is GPIO0

    welcome_msg();
    Serial.println("Edge Impulse Inference Ready!");
    delay(100);
}

// --------------------------------------------------------------------------------

// --- MAIN LOOP (Inference Engine) ---
void loop() {
    // Handle the button press to show date/time
    if (digitalRead(BUTTON_PIN) == LOW) {
        showdateandtime(pcf.getTime());
        delay(500); 
    }

    // --- STEP 1: TIME CHECK (Ensure correct sampling rate) ---
    if (millis() - last_read_time >= interval_ms) {
        last_read_time = millis();
        
        // --- STEP 2: READ LIVE IMU DATA ---
        sensors_event_t accel, gyro, temp;
        mpu.getEvent(&accel, &gyro, &temp);

        // --- STEP 3: FILL THE FEATURES BUFFER ---
        features[feature_ix++] = accel.acceleration.x; 
        features[feature_ix++] = accel.acceleration.y;
        features[feature_ix++] = accel.acceleration.z;
        features[feature_ix++] = gyro.gyro.x;
        features[feature_ix++] = gyro.gyro.y;
        features[feature_ix++] = gyro.gyro.z;
        
        // Output for debugging
        Serial.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", 
            accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
            gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);

        // --- STEP 4: CHECK IF WINDOW IS FULL ---
        if (feature_ix == EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
            
            // --- STEP 5: RUN INFERENCE ---
            ei_impulse_result_t result = { 0 };
            signal_t features_signal;
            features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
            features_signal.get_data = &get_signal_data;

            EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

            if (res == EI_IMPULSE_OK) {
                // --- STEP 6: ACT ON RESULT ---
                print_inference_result(result); // Print raw scores to serial
                handle_classification_result(result); // Trigger display and vibration
            }

            // Reset the index for the next non-overlapping window
            feature_ix = 0; 
        }
    }
}

// --------------------------------------------------------------------------------

// --- LINKER ERROR FIX: DEFINITION OF print_inference_result ---
// This function was the cause of the "undefined reference" error.
void print_inference_result(ei_impulse_result_t result) {
    // Print how long it took to perform inference
    ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
              result.timing.dsp,
              result.timing.classification,
              result.timing.anomaly);

    // Print the prediction results (classification)
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    // Your model is HAR, so this branch is likely unused, but kept for completeness
    ei_printf("Object detection results omitted...\r\n"); 
#else
    ei_printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }
#endif

    // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY
    ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif
}
