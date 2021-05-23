#define VERSION "0.28"
#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-Wparentheses"

// Include libraries
#include <Arduino.h>
#include <ESPWiFi.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#include "tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

/***************************
 * Begin Settings
 **************************/

// WIFI
#define WIFI_SSID "yourssid"
#define WIFI_PWD "yourpassword"

//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY "XXX"

// Go to https://openweathermap.org/find?q= and search for a location
String g_location = "Prague,CZ";
int g_utc_offset = 0;
String g_language = "en";
float g_latitude;
float g_longitude;

bool g_bMetric = true;
bool g_b24hour = true;

// Weather update
const int UPDATE_INTERVAL_SECS = 3600; // Update weather every hour

// Display settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D4;
const int SDC_PIN = D5;


#define BUTTONHPIN D3   //Boot button pin
#define LED        D2   //LED pin

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

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";
String deviceID;

long timeSinceLastWUpdate = 0;
long timeLastInfluxDBUpdate = 0;

//declaring prototypes
void updateClock( int utc_offset, bool firstStart);
void updateAstronomy(bool firstStart, const float lat, const float lon);
void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void setupDHT();
float getDHTTemp(bool metric);
float getDHTHum();
void updateCurrentWeather( const bool metric, const String lang, const String location, const String APIKey);
float getCurrentWeatherTemperature();
void updateForecast( const bool metric, const String lang, const String location, const String APIKey);

void setupInfluxDB( const char *serverUrl, const char *org, const char *bucket, const char *authToken);
void updateInfluxDB( bool firstStart, const String deviceID, const String version, const String location, int refresh_sec);
bool errorInfluxDB();
String errorInfluxDBMsg();
void writeInfluxDB( float temp, float hum, const float lat, const float lon);

void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);

void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display, bool firstStart);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, String& lang, bool& b24h, bool& metric, float& latitude, float& longitude);

// This array keeps function pointers to all frames, frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTimeAnalog, drawDateTime, drawDHT, drawCurrentWeather, drawForecast, drawAstronomy};
//FrameCallback frames[] = { drawDateTimeAnalog};
OverlayCallback overlays[] = { drawHeaderOverlay };

void setup() {
  // Prepare serial port
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  ESP.wdtEnable(WDTO_8S); //8 seconds watchdog timeout (still ignored) 

  // Configure pins
  pinMode(BUTTONHPIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite( LED, HIGH);
  setupDHT();

  //Initialize OLED
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  //Generate Device ID
  deviceID = "WS-" + WiFi.macAddress();
  deviceID.remove(17, 1); //remove MAC separators
  deviceID.remove(14, 1);
  deviceID.remove(11, 1);
  deviceID.remove(8, 1);
  deviceID.remove(5, 1);

  //Initialize Wifi
  WiFi.mode(WIFI_STA);
  WiFi.hostname(deviceID);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  display.drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
  display.drawString(88, 5, "Weather Station\nby InfluxData\nV" VERSION);
  display.display();
  delay(500);

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

  //Initialize OLED UI
  ui.setTargetFPS(30);
  ui.setTimePerFrame(10000);
  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);
  ui.setIndicatorPosition(TOP);
  ui.setIndicatorDirection(LEFT_RIGHT);   // Defines where the first frame is located in the bar.
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, sizeof(frames) / sizeof(FrameCallback));
  ui.setOverlays(overlays, sizeof(overlays) / sizeof(OverlayCallback));
  ui.init(); // Inital UI takes care of initalising the display too

  //Configure InfluxDB
  deviceID += "-" + WiFi.SSID();  //Add connected Wifi network
  setupInfluxDB( INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

  //Load all data
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
  digitalWrite( LED, LOW);
  ESP.wdtFeed();
  drawProgress(display, 10, "Detecting location");
  detectLocationFromIP( firstStart, g_location, g_utc_offset, g_language, g_b24hour, g_bMetric, g_latitude, g_longitude); //Load location data from IP

  ESP.wdtFeed();
  drawProgress(display, 20, "Updating time");
  updateClock( g_utc_offset, firstStart);

  ESP.wdtFeed();
  drawProgress(display, 40, "Updating weather");
  updateCurrentWeather( g_bMetric, g_language, g_location, OPEN_WEATHER_MAP_API_KEY);
  updateAstronomy( firstStart, g_latitude, g_longitude);
  
  ESP.wdtFeed();
  drawProgress(display, 60, "Updating forecasts");
  updateForecast( g_bMetric, g_language, g_location, OPEN_WEATHER_MAP_API_KEY);
  
  ESP.wdtFeed();
  drawProgress(display, 80, "Connecting InfluxDB");
  updateInfluxDB( firstStart, deviceID, VERSION, g_location, INFLUXDB_REFRESH_SECS);
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done");
  ESP.wdtFeed();
  digitalWrite( LED, HIGH);
  delay(500);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  time_t now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[8];
  sprintf_P(buff, PSTR("%2d:%02d"), g_b24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));

  if (!g_b24hour)
    display->drawString(display->getStringWidth(String(buff)), 52, timeInfo->tm_hour>=12?"pm":"am");

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth(), 54, "In:" + String(getDHTTemp( g_bMetric), 0) + (g_bMetric ? "°C" : "°F") + " Out:" + String(getCurrentWeatherTemperature(), 0) + (g_bMetric ? "°C" : "°F"));

  int8_t quality = getWifiSignal();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0)
        display->setPixel(120 + 2 * i, 6 - j);
     }
  }

  //Draw InfluxDB write mark
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  if ( errorInfluxDB())
    display->drawXbm( 0, 0, 8, 8, warning_8x8);
  
  display->drawHorizontalLine(0, 52, display->getWidth());
}

// convert the wifi dbm to %
int8_t getWifiSignal() {
  if (WiFi.status() != WL_CONNECTED)
    return 0;
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
    display->drawString(1,  0, "Wifi " + WiFi.SSID() + " " + String((WiFi.status() == WL_CONNECTED) ? String(getWifiSignal()) + "%" : wifiStatusStr(WiFi.status())));
    display->drawString(1, 10, "Up: " + String(millis()/1000/3600) + "h " + String((millis()/1000)%3600) + "s RAM: " + String( ESP.getFreeHeap()));
    display->drawString(1, 20, "Update in " + String((UPDATE_INTERVAL_SECS*1000 - (millis() - timeSinceLastWUpdate))/1000) + " s");
    display->drawString(1, 30, "InfluxDB " + (!errorInfluxDB() ? deviceID : errorInfluxDBMsg()));
    display->drawString(1, 40, String("V" VERSION) + "; tz: " + String(g_utc_offset) + " " + g_language);
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

  // Update data?
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED)
    updateData(&display,false);

  //Write into InfluxDB
  if ((timeLastInfluxDBUpdate == 0) || ( millis() - timeLastInfluxDBUpdate > (1000L*INFLUXDB_REFRESH_SECS)) && (ui.getUiState()->frameState == FIXED)) {
    digitalWrite( LED, LOW);
    timeLastInfluxDBUpdate = millis();
    ESP.wdtFeed();
    writeInfluxDB( getDHTTemp( g_bMetric), getDHTHum(), g_latitude, g_longitude);
    ESP.wdtFeed();
    Serial.println("InfluxDB write " + String(millis() - timeLastInfluxDBUpdate) + "ms");
    digitalWrite( LED, HIGH);
  }

  //Handle button
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
  ESP.wdtFeed();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your time budget.
    delay(remainingTimeBudget);
  }
}
