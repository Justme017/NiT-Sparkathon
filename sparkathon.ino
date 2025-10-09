#include <Arduino.h>
#include <U8x8lib.h>
#include <PCF8563.h>
PCF8563 pcf;
#include <Wire.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
Adafruit_MPU6050 mpu;

#define VIB_PIN A0
#define BUTTON_PIN D1


U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  u8x8.begin();
  u8x8.setFlipMode(1);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found. Check wiring!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 initialized with:");
  Serial.println("  Accel ±8G | Gyro ±500°/s | Filter 21Hz");
  delay(100);

  Wire.begin();
  pcf.init();//initialize the clock
  pcf.stopClock();//stop the clock
  pcf.setYear(25);//set year
  pcf.setMonth(10);//set month
  pcf.setDay(9);//set dat
  pcf.setHour(14);//set hour
  pcf.setMinut(10);//set minut
  pcf.setSecond(0);//set second
  pcf.startClock();//start the clock

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(0,OUTPUT);

  welcome_msg();
  delay(1000);
}

void vibrate_mild()
{
  analogWrite(VIB_PIN, 64);   // value from 0–255
  delay(1000);
  analogWrite(VIB_PIN, 0); 
}

void vibrate_medium()
{
  analogWrite(VIB_PIN, 128);   // value from 0–255
  delay(1000);
  analogWrite(VIB_PIN, 0);
}

void vibrate_strong()
{
  analogWrite(VIB_PIN, 255);   // value from 0–255
  delay(1000);
  analogWrite(VIB_PIN, 0);
}

void welcome_msg()
{
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  //u8x8.setFont(u8x8_font_courB18_2x3_r);
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
  u8x8.print("Stay Still");
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
  delay(1000);
  u8x8.clearDisplay();
}




// the loop function runs over and over again forever
void loop() {

  Time nowTime = pcf.getTime();//get current time

  if (digitalRead(BUTTON_PIN) == LOW) {
    showdateandtime(nowTime);
    delay(500);
  }


  
//  vibrate_mild();
//  delay(5000);
//  vibrate_medium();
//  delay(5000);
//  vibrate_strong();

  

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Compact serial output
  Serial.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                accel.acceleration.x,
                accel.acceleration.y,
                accel.acceleration.z,
                gyro.gyro.x,
                gyro.gyro.y,
                gyro.gyro.z);

  delay(10);


  
//  fidget_msg();
//  delay(1000);
//  
//  relax_msg();
//  delay(1000);
//
//  moving_msg();
//  delay(1000);
//
//  toomuch_msg();
//  delay(1000);
//
//  staystill_msg();
//  delay(1000);
//
  
  
}
