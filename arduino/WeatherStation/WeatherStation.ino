#define VERSION "0.36"

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
  0, //utcOffset
  37.7749,   //latitude
  122.4194,    //longitude
  false, //useMetric
  false, //use24hour
  true, //useYMDdate
  "pool.ntp.org,time.nis.gov,time.google.com",
  0,  //tempOffset
  0,  //humOffset
  
  INFLUXDB_URL,   //influxdbUrl;
  INFLUXDB_TOKEN, //influxdbToken;
  INFLUXDB_ORG,   //influxdbOrg;
  "iot_center",   //influxdbBucket;
  1 //influxdbRefreshMin;
};

// Initialize the oled display
SSD1306Wire     display(I2C_OLED_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display);

bool readyForWeatherUpdate = false; // flag changed in the ticker function
String deviceID;

unsigned long timeSinceLastWUpdate = 0;
unsigned long timeLastInfluxDBUpdate = 0;

//declaring prototypes
void setupOLEDUI(OLEDDisplayUi *ui);
void setupInfluxDB( const String &serverUrl, const String &org, const String &bucket, const String &authToken, int refresh_sec);
void setupDHT();
float getDHTTemp(bool metric);
float getDHTHum();
void drawSplashScreen(OLEDDisplay *display, const char* version);
void drawWifiProgress(OLEDDisplay *display, const char* version);
void drawUpdateProgress(OLEDDisplay *display, int percentage, const char* label);

void updateData(OLEDDisplay *display, bool firstStart);
void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, String& lang, bool& b24h, bool& bYMD, bool& metric, float& latitude, float& longitude);
void updateClock( bool firstStart, int utc_offset, const String ntp);
void updateAstronomy(bool firstStart, const float lat, const float lon);
void updateCurrentWeather( const bool metric, const String lang, const String location, const String APIKey);
void updateForecast( const bool metric, const String lang, const String location, const String APIKey);

void updateInfluxDB( bool firstStart, const String &deviceID, const String &version, const String &location);
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
  deviceID += "-" + WiFi.SSID();  //Add connected Wifi network
  setupInfluxDB( conf.influxdbUrl, conf.influxdbOrg, conf.influxdbBucket, conf.influxdbToken, conf.influxdbRefreshMin * 60);

  //Load all data
  updateData(&display, true);
}

void updateData(OLEDDisplay *display, bool firstStart) {
  digitalWrite( LED, LOW);

  drawUpdateProgress(display, 0, "Detecting location");
  if (conf.detectLocationIP)
    detectLocationFromIP( firstStart, conf.location, conf.utcOffset, conf.language, conf.use24hour, conf.useYMDdate, conf.useMetric, conf.latitude, conf.longitude); //Load location data from IP
  
  drawUpdateProgress(display, 10, "Updating time");
  updateClock( firstStart, conf.utcOffset, conf.ntp);

  drawUpdateProgress(display, 30, "Updating weather");
  updateCurrentWeather( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 50, "Calculate moon phase");  
  updateAstronomy( firstStart, conf.latitude, conf.longitude);
  
  drawUpdateProgress(display, 60, "Updating forecasts");
  updateForecast( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 80, "Connecting InfluxDB");
  updateInfluxDB( firstStart, deviceID, VERSION, conf.location);

  drawUpdateProgress(display, 100, "Done");
  readyForWeatherUpdate = false;

  digitalWrite( LED, HIGH);
  delay(500);
}

void loop() {
  if (millis() - timeSinceLastWUpdate > (1000L*conf.updateDataMin*60)) {
    readyForWeatherUpdate = true;
    timeSinceLastWUpdate = millis();
  }

  // Update data?
  if (ui.getUiState()->frameState == FIXED) {
    if (readyForWeatherUpdate)
      updateData(&display,false);

    //Write into InfluxDB
    if ((timeLastInfluxDBUpdate == 0) || ( millis() - timeLastInfluxDBUpdate > (1000L*conf.influxdbRefreshMin * 60))) {
      digitalWrite( LED, LOW);
      timeLastInfluxDBUpdate = millis();
      ESP.wdtFeed();
      writeInfluxDB( getDHTTemp( conf.useMetric), getDHTHum(), conf.latitude, conf.longitude);
      Serial.println("InfluxDB write " + String(millis() - timeLastInfluxDBUpdate) + "ms");
      digitalWrite( LED, HIGH);
    }
  }
  
  //Handle BOOT button
  int loops = 0;
  while (digitalRead(BUTTONHPIN) == LOW) {  //Pushed boot button?
    if (loops == 0) {
      ui.nextFrame();   //jump to the next frame
    }
    if (loops > 4)
      showConfiguration(&display, (200 - loops) / 10, VERSION, timeSinceLastWUpdate, deviceID);  //Show configuration after 0.5s

    loops++;
    if (loops > 200)  //reboot after 20 seconds
      ESP.restart();

    ESP.wdtFeed();
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
