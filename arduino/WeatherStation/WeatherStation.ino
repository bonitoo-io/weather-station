#define VERSION "0.21"

// Include libraries
#include <Arduino.h>
#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonListener.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

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

//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY "XXX"

// Go to https://openweathermap.org/find?q= and search for a location
String OPEN_WEATHER_MAP_LOCATION = "Prague,CZ";
int utc_offset = 0;
String OPEN_WEATHER_MAP_LANGUAGE = "en";
const uint8_t MAX_FORECASTS = 3;

bool bMetric = true;
bool b24hour = true;

// Weather update
const int UPDATE_INTERVAL_SECS = 3600; // Update weather every hour

// Display settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D4;
const int SDC_PIN = D5;

// Internal sensor settings
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN D1       // Digital pin connected to the DHT sensor

#define BUTTONHPIN D3   //Boot button pin

// Adjust according to your language
const char* const WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "server-url"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "server token"
// InfluxDB v2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "org id"
// InfluxDB v2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "bucket name"
// Refresh rate - write temperature and humidity
#define INFLUXDB_REFRESH_SECS 60  //Once per minute

/***************************
 * End Settings
 **************************/
#include "mirek.h" //Custom development configuration - remove or comment it out

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

time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";
String deviceID;

long timeSinceLastWUpdate = 0;
long timeLastInfluxDBUpdate = 0;

// Data point
Point sensor("environment");

InfluxDBClient influxDBClient;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display, bool firstStart);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void detectLocationFromIP( String& location, int& utc_offset, String& lang, bool& b24h, bool& metric);
String utf8ascii(const String s);
void testutf8();

// This array keeps function pointers to all frames, frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTimeAnalog, drawDateTime, drawDHT, drawCurrentWeather, drawForecast};
//FrameCallback frames[] = { drawDateTimeAnalog};
OverlayCallback overlays[] = { drawHeaderOverlay };

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  ESP.wdtEnable(10000); //10 seconds watchdog timeout
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

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  display.drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
  display.drawString(88, 5, "Weather Station\nby InfluxData\nV" VERSION);
  display.display();
  delay(500);
  readDHT();

  int counter = 0;
  Serial.print("Wifi " WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    display.clear();
    display.drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
    display.drawString(88, 0, "Connecting WiFi");
    display.drawString(88, 15, WIFI_SSID);
    display.drawXbm(71, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(85, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(99, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.drawString(88, 40, "V" VERSION);
    display.display();
    counter++;
    delay(500);
  }
  Serial.println();

  ui.setTargetFPS(30);
  ui.setTimePerFrame(10000);

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

  // Inital UI takes care of initalising the display too
  ui.init();

  //Generate Device ID
  deviceID = "WS-" + WiFi.macAddress() + "-" + WiFi.SSID();
  deviceID.remove(17, 1);
  deviceID.remove(14, 1);
  deviceID.remove(11, 1);
  deviceID.remove(8, 1);
  deviceID.remove(5, 1);

  //Configure InfluxDB
  influxDBClient.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
  sensor.addTag("clientId", deviceID);
  sensor.addTag("Device", "WS-ESP8266");
  sensor.addTag("TemperatureSensor", "DHT11");
  sensor.addTag("HumiditySensor", "DHT11");

  updateData(&display, true);
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display, bool firstStart) {
  ESP.wdtFeed();
  drawProgress(display, 10, "Detecting location");

  detectLocationFromIP( OPEN_WEATHER_MAP_LOCATION, utc_offset, OPEN_WEATHER_MAP_LANGUAGE, b24hour, bMetric); //Load location data from IP

  ESP.wdtFeed();
  drawProgress(display, 20, "Updating time");
  if (firstStart) {
    configTime(utc_offset, 0 , "pool.ntp.org", "time.nis.gov");

    // Wait till time is synced
    Serial.print("Syncing time");
    int i = 0;
    while (time(nullptr) < 1000000000ul && i < 40) {
      Serial.print(".");
      delay(500);
      i++;
    }
    Serial.println();
  
    // Show time
    time_t tnow = time(nullptr);
    Serial.print("Synchronized time: ");
    Serial.println(ctime(&tnow));
  }
  ESP.wdtFeed();
  drawProgress(display, 40, "Updating weather");
  currentWeatherClient.setMetric(bMetric);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrent(&currentWeather, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION);
  ESP.wdtFeed();
  drawProgress(display, 60, "Updating forecasts");
  forecastClient.setMetric(bMetric);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecasts(forecasts, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION, MAX_FORECASTS);
  ESP.wdtFeed();
  drawProgress(display, 80, "Connecting InfluxDB");
  // Check server connection
  if (firstStart)
    if (influxDBClient.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(influxDBClient.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(influxDBClient.getLastErrorMessage());
    }
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done");
  ESP.wdtFeed();
  delay(500);
}

void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int clockCenterX=30+x;
  int clockCenterY=30+y;
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
  char buff[19];
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%s, %s\n%2d/%2d/%04d"), WDAY_NAMES[t->tm_wday], MONTH_NAMES[t->tm_mon], t->tm_mday, t->tm_mon+1, t->tm_year + 1900);
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
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), b24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x - (b24hour ? 0 : 4), 20 + y, String(buff));

  if (!b24hour) {
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->setFont(ArialMT_Plain_10);
    display->drawString(display->getWidth() + x, 18 + y, timeInfo->tm_hour>=12?"pm":"am");
  }
}

void readDHT() {
  unsigned long actualTime = millis();
  if ( timeDHT > actualTime)
    timeDHT = actualTime;
  if ( actualTime - timeDHT > 2000) { //read once 2 seconds, otherwise provide "cached" values
    tempDHT = dht.readTemperature(!bMetric);
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

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  readDHT();
  display->drawString(0 + x, 20 + y, String(tempDHT, 1) + (bMetric ? "°C" : "°F"));
  display->drawString(80 + x, 20 + y, String(humDHT, 0) + "%");
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, utf8ascii(currentWeather.description));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 10 + y, String(currentWeather.temp, 0) + (bMetric ? "°C" : "°F"));

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

  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 39, String(forecasts[dayIndex].temp, 0) + (bMetric ? "°C" : "°F"));
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[8];
  sprintf_P(buff, PSTR("%2d:%02d"), b24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));

  if (!b24hour)
    display->drawString(display->getStringWidth(String(buff)), 52, timeInfo->tm_hour>=12?"pm":"am");

  readDHT();
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth(), 54, "In:" + String(tempDHT, 0) + (bMetric ? "°C" : "°F") + " Out:" + String(currentWeather.temp, 0) + (bMetric ? "°C" : "°F"));

  int8_t quality = getWifiSignal();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 6 - j);
      }
    }
  }

  display->drawHorizontalLine(0, 52, display->getWidth());
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

void showConfiguration(OLEDDisplay *display, int secToReset) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawRect(0, 0, display->getWidth(), display->getHeight());
  if ( secToReset > 5) {
    display->drawString(1, 0, "WIFI " + WiFi.SSID());
    display->drawString(1, 10, "Status " + String(wifiStatusStr(WiFi.status())) + " - " + String(getWifiSignal()) + "%");
    display->drawString(1, 20, "Weather update in " + String((UPDATE_INTERVAL_SECS*1000 - (millis() - timeSinceLastWUpdate))/1000) + " s");
    display->drawString(1, 30, "InfluxDB " + (influxDBClient.getLastErrorMessage() == "" ? deviceID : influxDBClient.getLastErrorMessage()));
    display->drawString(1, 40, String("V" VERSION) + ", " + utf8ascii(OPEN_WEATHER_MAP_LOCATION) + ";" + String(utc_offset) + " " + OPEN_WEATHER_MAP_LANGUAGE);
    display->drawString(1, 50, "http://" + WiFi.localIP().toString());
  } else
    display->drawString(0, 30, "RESETING IN " + String(secToReset) + "s !");

  display->display();
}

void loop() {
  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    readyForWeatherUpdate = true;
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED)
    updateData(&display,false);

  int loops = 0;
  while (digitalRead(BUTTONHPIN) == LOW) {  //Pushed boot button?
    if (loops == 0) {
      ui.nextFrame();   //jump to the next frame
    }
    if (loops > 4)
      showConfiguration(&display, (200 - loops) / 10);  //Show configuration after 0.5s

    loops++;
    if (loops > 200)  //reboot after 20 seconds
      ESP.restart();

    ESP.wdtFeed();
    delay(100);
  }

  int remainingTimeBudget = ui.update();

  //Write into InfluxDB
  if ((timeLastInfluxDBUpdate == 0) || ( millis() - timeLastInfluxDBUpdate > (1000L*INFLUXDB_REFRESH_SECS))) {
    timeLastInfluxDBUpdate = millis();
    sensor.clearFields();
    readDHT();
    // Report temperature and humidity
    sensor.addField("Temperature", tempDHT);
    sensor.addField("Humidity", humDHT);
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(influxDBClient.pointToLineProtocol(sensor));

    // Write point
    if (!influxDBClient.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(influxDBClient.getLastErrorMessage());
    }
  }

  ESP.wdtFeed();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your time budget.
    delay(remainingTimeBudget);
  }
}
