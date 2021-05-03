/**
 * Copyright 2021 Bonitoo.io
 * 
 * TZT ESP8266 Weather Station example code.
 * 
 * Reads temperature and humidity in a loop and displays values on I2C OLED display.
 * 
 * Requires:
 *  - Adafruit DHT library https://github.com/adafruit/DHT-sensor-library 
 *  - U8G2 graphics library for monochrome OLED displays: https://github.com/olikraus/u8g2 
 * 
 */

#include <U8x8lib.h>
#include <Wire.h>
#include <DHT.h>

#define PIN_DHT 5
#define PIN_LED 4

DHT dht(PIN_DHT, DHT11);


U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

void setup() {
  Wire.begin(2, 14);
  Serial.begin(74880);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 1);
  u8x8.begin();
  dht.begin();
  delay(1000);
  Serial.println("Weather station starting");
  u8x8.setFont(u8x8_font_8x13_1x2_f);
  u8x8.drawString(0, 0, "Weather station");
  delay(1000);
}

void loop() {
  digitalWrite(PIN_LED, 0);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  u8x8.clear();
  if (isnan(temp) || isnan(hum)) {
    u8x8.drawString(0, 0, "DHT read error");
  } else {
    u8x8.drawString(0, 1, "Temp:");
    u8x8.drawString(6, 1, String(temp, 1).c_str());
    u8x8.drawString(0, 3, "Hum:");
    u8x8.drawString(6, 3, String(hum, 0).c_str());
  }
  digitalWrite(PIN_LED, 1);
  delay(10000);
}
