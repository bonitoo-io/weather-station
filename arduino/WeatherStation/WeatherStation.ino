#define VERSION "0.44"

// Include libraries
#include <Arduino.h>
#include <ESPWiFi.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#include "Tools.h"

/***************************
 * Begin Settings
 **************************/
// WIFI
#define WIFI_SSID "yourssid"
#define WIFI_PWD "yourpassword"

//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY "XXX"

// IoT Center url, e.g. http://192.168.1.20:5000  (can be empty, if not configured)
#define IOT_CENTER_URL ""
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "server-url"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "server token"
// InfluxDB v2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "org id"
/***************************
 * End Settings
 **************************/

// Display settings
#define I2C_OLED_ADDRESS 0x3c
#define SDA_PIN D4
#define SDC_PIN D5
// Button and LED
#define BUTTONHPIN D3   //Boot button pin
#define LED        D2   //LED pin

#include "custom_dev.h" //Custom development configuration - remove or comment it out 

tConfig conf = {  //default values
  WIFI_SSID,  //wifi_ssid
  WIFI_PWD, //wifi_pwd

  true, //detectLocationIP
  60,   //updateDataMin
  OPEN_WEATHER_MAP_API_KEY, // openweatherApiKey;  
  "San Francisco, US", //location
  "en", //language
  -25200, //utcOffset
  37.7749,   //latitude
  122.4194,    //longitude
  false, //useMetric
  false, //use24hour
  true, //useYMDdate
  "pool.ntp.org,time.nis.gov,time.google.com", //ntp
  0,  //tempOffset
  0,  //humOffset
  
  IOT_CENTER_URL, //iotCenterUrl;
  60,             //iotRefreshMin
  
  INFLUXDB_URL,   //influxdbUrl;
  INFLUXDB_TOKEN, //influxdbToken;
  INFLUXDB_ORG,   //influxdbOrg;
  "iot_center",   //influxdbBucket;
  1 //influxdbRefreshMin;
};

// Initialize the oled display
SSD1306Wire display(I2C_OLED_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui( &display);

String deviceID;

unsigned long timeSinceLastUpdate = 0;
unsigned int lastUpdateMins = 0;

//declaring prototypes
void setupOLEDUI(OLEDDisplayUi *ui);
void setupInfluxDB( const String &serverUrl, const String &org, const String &bucket, const String &authToken, int refresh_sec);
void setupDHT();
float getDHTTemp(bool metric);
float getDHTHum();
void saveDHTTemp(bool metric);
void drawSplashScreen(OLEDDisplay *display, const char* version);
void drawWifiProgress(OLEDDisplay *display, const char* version);
void drawUpdateProgress(OLEDDisplay *display, int percentage, const String& label);

void updateData(OLEDDisplay *display, bool firstStart);
void loadIoTCenter( bool firstStart, const String& iot_url, const String &deviceID, String& influxdbUrl, String& influxdbOrg, String& influxdbToken, String& influxdbBucket, unsigned int& influxdbRefreshMin, unsigned int& iotRefreshMin, float& latitude, float& longitude);
void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, char* lang, bool& b24h, bool& bYMD, bool& metric, float& latitude, float& longitude);
void updateClock( bool firstStart, int utc_offset, const String ntp);
void updateAstronomy(bool firstStart, const float lat, const float lon);
void updateCurrentWeather( const bool metric, const String& lang, const String& location, const String& APIKey);
void updateForecast( const bool metric, const String& lang, const String& location, const String& APIKey);

void updateInfluxDB( bool firstStart, const String &deviceID, const String &wifi, const String &version, const String &location);
void writeInfluxDB( float temp, float hum, const float lat, const float lon);
void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate, const String deviceID);

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
  setLanguage( conf.language);
  display.init();
  display.clear();
  display.display();
  //display.flipScreenVertically();
  //display.setContrast(100);

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
  WiFi.begin(conf.wifi_ssid, conf.wifi_pwd);
  drawSplashScreen(&display, VERSION);
  delay(500);
  drawWifiProgress(&display, VERSION);

  //Initialize OLED UI
  setupOLEDUI(&ui);

  //Configure InfluxDB
  setupInfluxDB( conf.influxdbUrl, conf.influxdbOrg, conf.influxdbBucket, conf.influxdbToken, conf.influxdbRefreshMin * 60);

  //Load all data
  updateData(&display, true);

  //Save temperature for the chart
  saveDHTTemp( conf.useMetric);
}

void updateData(OLEDDisplay *display, bool firstStart) {
  digitalWrite( LED, LOW);

  drawUpdateProgress(display, 0, getStr(s_Connecting_IoT_Center));
  if (firstStart)
    loadIoTCenter(firstStart, conf.iotCenterUrl, deviceID, conf.influxdbUrl, conf.influxdbOrg, conf.influxdbToken, conf.influxdbBucket, conf.influxdbRefreshMin, conf.iotRefreshMin, conf.latitude, conf.longitude);
  
  drawUpdateProgress(display, 10, getStr(s_Detecting_location));
  if (conf.detectLocationIP) {
    detectLocationFromIP( firstStart, conf.location, conf.utcOffset, conf.language, conf.use24hour, conf.useYMDdate, conf.useMetric, conf.latitude, conf.longitude); //Load location data from IP
    setLanguage( conf.language);
  }
  
  drawUpdateProgress(display, 30, getStr(s_Updating_time));
  updateClock( firstStart, conf.utcOffset, conf.ntp);

  drawUpdateProgress(display, 50, getStr(s_Updating_weather));
  updateCurrentWeather( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 60, getStr(s_Calculate_moon_phase));
  updateAstronomy( firstStart, conf.latitude, conf.longitude);
  
  drawUpdateProgress(display, 70, getStr(s_Updating_forecasts));
  updateForecast( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 80, getStr(s_Connecting_InfluxDB));
  updateInfluxDB( firstStart, deviceID, WiFi.SSID(), VERSION, conf.location);

  drawUpdateProgress(display, 100, getStr(s_Done));

  digitalWrite( LED, HIGH);
  delay(500);
}

void loop() {
  // Ticker function - executed once per minute
  if ((ui.getUiState()->frameState == FIXED) && (millis() - timeSinceLastUpdate >= 1000*60)){
    timeSinceLastUpdate = millis();
    lastUpdateMins++;

    //Sync IoT Center configuration
    if (lastUpdateMins % conf.iotRefreshMin == 0) {
      digitalWrite( LED, LOW);
      loadIoTCenter( false, conf.iotCenterUrl, deviceID, conf.influxdbUrl, conf.influxdbOrg, conf.influxdbToken, conf.influxdbBucket, conf.influxdbRefreshMin, conf.iotRefreshMin, conf.latitude, conf.longitude);
      digitalWrite( LED, HIGH);
    }

    //Update data?
    if (lastUpdateMins % conf.updateDataMin == 0) {
      digitalWrite( LED, LOW);
      updateData(&display,false);
      digitalWrite( LED, HIGH);
    }

    //Write into InfluxDB
    if (lastUpdateMins % conf.influxdbRefreshMin == 0) {
      digitalWrite( LED, LOW);
      ESP.wdtFeed();
      writeInfluxDB( getDHTTemp( true), getDHTHum(), conf.latitude, conf.longitude);  //aways save in celsius
      Serial.print(F("InfluxDB write "));
      Serial.println(String(millis() - timeSinceLastUpdate) + String(F("ms")));
      digitalWrite( LED, HIGH);
      saveDHTTemp( conf.useMetric);  //Save temperature for the chart
    }
  }
  
  //Handle BOOT button
  unsigned int loops = 0;
  while (digitalRead(BUTTONHPIN) == LOW) {  //Pushed boot button?
    if (loops == 0) {
      ui.nextFrame();   //jump to the next frame
    }
    if (loops > 4)
      showConfiguration(&display, (200 - loops) / 10, VERSION, timeSinceLastUpdate + (conf.updateDataMin - ((lastUpdateMins % conf.updateDataMin)) * 60 * 1000), deviceID);  //Show configuration after 0.5s

    loops++;
    if (loops > 200)  //reboot after 20 seconds
      ESP.restart();

    delay(100);
  }

  //Handle OLED UI
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your time budget.
    delay(remainingTimeBudget);
  }
}
