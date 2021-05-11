#define VERSION "0.03"

#include <Arduino.h>
#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonListener.h>

// Influxdb
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "WeatherStationFonts.h"
#include "DSEG7Classic-BoldFont.h"
#include "WeatherStationImages.h"
#include "DHT.h"

/***************************
 * Begin Settings
 **************************/

// WIFI
#define WIFI_SSID "yourssid"
#define WIFI_PWD "yourpassword"

#define TZ              1       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries

// https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY "XXX"

#include "mirek.h" //Remove or comment it out

// Go to https://openweathermap.org/find?q= and search for a location
String OPEN_WEATHER_MAP_LOCATION = "Prague,CZ";

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "en";
const uint8_t MAX_FORECASTS = 3;

const boolean IS_METRIC = true;
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// Weather update
const int UPDATE_INTERVAL_SECS = 3600; // Update every hour

// Display settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D4;
const int SDC_PIN = D5;

// Internal sensor settings
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN D1     // Digital pin connected to the DHT sensor

#define BUTTONHPIN D3  //Boot button pin

// Adjust according to your language
const char* const WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
/***************************
 * End Settings
 **************************/

DHT dht(DHTPIN, DHTTYPE);
float tempDHT;
float humDHT;
unsigned long timeDHT = 0;

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTimeAnalog, drawDateTime, drawDHT, drawCurrentWeather, drawForecast};
//FrameCallback frames[] = { drawDateTimeAnalog};
OverlayCallback overlays[] = { drawHeaderOverlay };

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(BUTTONHPIN, INPUT);
  dht.begin();
  
  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);
  
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  display.drawXbm( 0, 0, InfluxData_Logo_width, InfluxData_Logo_height, InfluxData_Logo_bits);
  display.drawString(88, 5, "Weather Station\nby InfluxData\nV" VERSION);
  display.display();
  delay(500);
  readDHT();

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawXbm( 0, 0, InfluxData_Logo_width, InfluxData_Logo_height, InfluxData_Logo_bits);
    display.drawString(88, 5, "Connecting WiFi");
    display.drawString(88, 15, WIFI_SSID);
    display.drawXbm(71, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(85, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(99, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.drawString(88, 40, "V" VERSION);
    display.display();

    counter++;
  }
  
  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(TOP);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, sizeof(frames) / sizeof(FrameCallback));

  ui.setOverlays(overlays, sizeof(overlays) / sizeof(OverlayCallback));

  // Inital UI takes care of initalising the display too.
  ui.init();

  updateData(&display);
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 10, "Updating time...");
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  drawProgress(display, 30, "Updating weather...");
  currentWeatherClient.setMetric(IS_METRIC);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrent(&currentWeather, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION);
  drawProgress(display, 50, "Updating forecasts...");
  forecastClient.setMetric(IS_METRIC);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecasts(forecasts, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION, MAX_FORECASTS);

  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done");
  delay(1000);
}

void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  const int clockCenterX=30;
  const int clockCenterY=30;
  const int clockSize=20;
  
  int i;
  now = time(nullptr);
  struct tm* t;
  t = localtime(&now);
  
  // Draw marks for hours
  for (i=0; i<12; i++) {
    float f = ((i * 30) + 270) * 0.0175;
    display->drawLine(clockSize*cos(f)+clockCenterX, clockSize*sin(f)+clockCenterY, (clockSize-4+(i%3==0?0:3))*cos(f)+clockCenterX, (clockSize-4+(i%3==0?0:3))*sin(f)+clockCenterY);
  }

  //draw seconds
  float s = ((t->tm_sec*6)+270)*0.0175;
  display->drawLine(clockSize*cos(s)+clockCenterX, clockSize*sin(s)+clockCenterY, cos(s)+clockCenterX, sin(s)+clockCenterY);

  //Draw minutes
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int m=(t->tm_min*6)+270;
 
  x1=(clockSize-3)*cos(m*0.0175);
  y1=(clockSize-3)*sin(m*0.0175);
  x2=cos(m*0.0175);
  y2=sin(m*0.0175);
  x3=10*cos((m+8)*0.0175);
  y3=10*sin((m+8)*0.0175);
  x4=10*cos((m-8)*0.0175);
  y4=10*sin((m-8)*0.0175);
  
  display->drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  display->drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  display->drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  display->drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  //draw hour
  int h=(t->tm_hour*30)+(t->tm_min/2)+270;
  
  x1=(clockSize-7)*cos(h*0.0175);
  y1=(clockSize-7)*sin(h*0.0175);
  x2=cos(h*0.0175);
  y2=sin(h*0.0175);
  x3=8*cos((h+12)*0.0175);
  y3=8*sin((h+12)*0.0175);
  x4=8*cos((h-12)*0.0175);
  y4=8*sin((h-12)*0.0175);
  
  display->drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  display->drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  display->drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  display->drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);
  
  //draw date
  char buff[16];
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%s\n%2d/%2d/%04d"), WDAY_NAMES[t->tm_wday], t->tm_mday, t->tm_mon+1, t->tm_year + 1900);
  display->drawString(64 + x, 10 + y, String(buff));
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%s %2d/%2d/%04d"), WDAY_NAMES[timeInfo->tm_wday], timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(64 + x, 8 + y, String(buff));
  
  display->setFont(DSEG7_Classic_Bold_21);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 20 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void readDHT() {
  unsigned long actualTime = millis();
  if ( timeDHT > actualTime)
    timeDHT = actualTime; 
  if ( actualTime - timeDHT > 2000) { //read once 2 seconds, otherwise privide "cached" values
    tempDHT = dht.readTemperature(!IS_METRIC);
    humDHT = dht.readHumidity();
    timeDHT = millis();
  }
}

void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 5 + y, "INDOOR");
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 10 + y, "Temp");
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 10 + y, "Hum");
 
  readDHT();
  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String s = String(tempDHT, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(0 + x, 20 + y, s);

  s = String(humDHT, 0) + "%";
  display->drawString(80 + x, 20 + y, s);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, currentWeather.description);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(60 + x, 10 + y, temp);

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 5, WDAY_NAMES[timeInfo->tm_wday]);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 17, forecasts[dayIndex].iconMeteoCon);
  String temp = String(forecasts[dayIndex].temp, 0) + (IS_METRIC ? "°C" : "°F");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 39, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));

  readDHT();
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = String(tempDHT, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(118, 54, temp);

  int8_t quality = getWifiSignal();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 64 - j);
      }
    }
  }
  
  display->drawHorizontalLine(0, 52, 128);
}

// convert the wifi dbm to %
int8_t getWifiSignal() {
  int32_t q = 2 * (WiFi.RSSI() + 100);
  if (q < 0)
    return 0;
  if (q > 100)   
    return 100;
  return q;
}

const char* wifiStatusStr(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "No shield";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "No SSID available";
    case WL_SCAN_COMPLETED: return "Scan completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connect failed";
    case WL_CONNECTION_LOST: return "Connection lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown";    
  }
}

void showConfiguration(OLEDDisplay *display) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  display->drawString(0, 0, "WIFI: " + WiFi.SSID());
  display->drawString(0, 10, "Status: " + String(wifiStatusStr(WiFi.status())));
  display->drawString(0, 20, "Signal: " + String(getWifiSignal()) + "%");
  display->drawString(0, 40, "URL: http://" + WiFi.localIP().toString());
    
  display->display();
  delay(100);
}

void loop() {
  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    readyForWeatherUpdate = true;
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED)
    updateData(&display);

  while (digitalRead(BUTTONHPIN) == LOW)
    showConfiguration(&display);

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
}
