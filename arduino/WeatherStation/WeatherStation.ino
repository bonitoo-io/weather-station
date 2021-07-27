// Include libraries
#include <Arduino.h>
#include <ESPWiFi.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include "WeatherStation.h"
#include "InfluxDBHelper.h"
#include "Tools.h"
#include "Updater.h"
#include "DHTSensor.h"
#include "Debug.h"
#include "Version.h"

/***************************
 * Begin Settings
 **************************/
//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY "XXX"

// IoT Center url, e.g. http://192.168.1.20:5000  (can be empty, if not configured)
#define IOT_CENTER_URL ""

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

tConfig conf = {
  true, //detectLocationIP
  60,   //updateDataMin
  OPEN_WEATHER_MAP_API_KEY, // openweatherApiKey;  
  "San Francisco,US", //location
  "en", //language
  -25200, //utcOffset in seconds
  37.7749,   //latitude
  -122.4194,    //longitude
  false, //useMetric
  false, //use24hour
  true, //useYMDdate
  "pool.ntp.org,time.nis.gov,time.google.com", //ntp
  0,  //tempOffset
  0,  //humOffset
  
  IOT_CENTER_URL, //iotCenterUrl;
  60             //iotRefreshMin
};

// Initialize the oled display
SSD1306Wire display(I2C_OLED_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui( &display);

InfluxDBHelper influxdbHelper;
// Weather station backend
WeatherStation station(&conf, &influxdbHelper);
Updater updater;

unsigned long timeSinceLastUpdate = 0;
unsigned int lastUpdateMins = 0;
String wifiSSID;
bool shouldSetupInfluxDb = false;
bool shouldDrawWifiProgress = false;
bool isAPRunning = false;
bool initialized = false;
String resetReason;

//declaring prototypes
void setupOLEDUI(OLEDDisplayUi *ui);

void drawUpdateProgress(OLEDDisplay *display, int percentage, const String& label);
void startWifiProgress(OLEDDisplay *display, const char* version, const char *ssid);
void drawWifiProgress(OLEDDisplay *display, const char* version, const char *ssid);
void drawAPInfo(OLEDDisplay *display, APInfo *info);
void drawFWUpdateInfo(OLEDDisplay *display, const String &fistLine, const String &secondLine);
void drawFWUpdateProgress(OLEDDisplay *display, const char* version, int percent);

void updateData(OLEDDisplay *display, bool firstStart);
bool loadIoTCenter( bool firstStart, const String& iot_url, const char *deviceID, InfluxDBSettings *influxdbSettings, unsigned int& iotRefreshMin, float& latitude, float& longitude);
void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, char* lang, bool& b24h, bool& bYMD, bool& metric, float& latitude, float& longitude);
void updateClock( bool firstStart, int utc_offset, const String ntp);
void updateAstronomy(bool firstStart, const float lat, const float lon);
void updateCurrentWeather( const bool metric, const String& lang, const String& location, const String& APIKey);
void updateForecast( const bool metric, const String& lang, const String& location, const String& APIKey);

void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate, const char *deviceID, InfluxDBHelper *influxDBHelper);

void initData() {
  if(!initialized && WiFi.isConnected() && !isAPRunning) {
    WS_DEBUG_RAM("InitData");
    updater.init(station.getUpdaterSettings(), VERSION);
    
    //Initialize OLED UI
    setupOLEDUI(&ui);
    //Load all data
    updateData(&display, true);
   
    initialized = true;
  }
}

void setup() {
  // Prepare serial port
  Serial.begin(74880);
  Serial.println();
  Serial.println();
  ESP.wdtEnable(WDTO_8S); //8 seconds watchdog timeout (still ignored) 
  Serial.println(F("Starting Weather station v" VERSION));
  WS_DEBUG_RAM("Setup 1");
  if(ESP.getResetInfoPtr()->reason != REASON_DEEP_SLEEP_AWAKE) {
    resetReason = ESP.getResetReason();
  }

  station.getWifiManager()->setWiFiConnectionEventHandler(wifiConnectionEventHandler);
  station.getWifiManager()->setAPEventHandler(wifiAPEventHandler);
  
  station.getInfluxDBSettings()->setHandler([](){
    shouldSetupInfluxDb =  true;
  });
  
  station.getUpdaterSettings()->setHandler([](){
    updater.init(station.getUpdaterSettings(), VERSION);
  });

  updater.setUpdateCallbacks(updateStartHandler,updateProgressHandler,updateFinishedHandler);
  
  station.begin();

  WS_DEBUG_RAM("Setup 2");

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
  refreshDHTCachedValues(conf.useMetric);

  initData();

  WS_DEBUG_RAM("Setup 3");
}

void updateData(OLEDDisplay *display, bool firstStart) {
  digitalWrite( LED, LOW);
  
  drawUpdateProgress(display, 0, getStr(s_Detecting_location));
  if (conf.detectLocationIP) {
    WS_DEBUG_RAM("Before IPloc");
    if(!firstStart) {
      influxdbHelper.release();
      WS_DEBUG_RAM("After cl release");
      delay(1000);
    }
    detectLocationFromIP( firstStart, conf.location, conf.utcOffset, conf.language, conf.use24hour, conf.useYMDdate, conf.useMetric, conf.latitude, conf.longitude); //Load location data from IP
    setLanguage( conf.language);
    WS_DEBUG_RAM("After IPloc");
    if(!firstStart) {
      influxdbHelper.begin(station.getInfluxDBSettings());
    }
  }

  drawUpdateProgress(display, 10, getStr(s_Updating_time));
  updateClock( firstStart, conf.utcOffset, conf.ntp);


  drawUpdateProgress(display, 20, getStr(s_Checking_update));
  if(firstStart && station.getUpdaterSettings()->updateTime < 2400) {
    updater.checkUpdate();
  }

  drawUpdateProgress(display, 30, getStr(s_Connecting_IoT_Center));
  if (firstStart) {
    // TODO: better solution for updating Settings from IoT center
    auto influxDBSettings = station.getInfluxDBSettings();
    if(loadIoTCenter(firstStart, conf.iotCenterUrl, getDeviceID(), influxDBSettings,  conf.iotRefreshMin, conf.latitude, conf.longitude)) {
      station.getPersistence()->writeToFS(influxDBSettings);
      influxDBSettings->notify();
    } else {
      influxdbHelper.begin(influxDBSettings);
    }
  }

  drawUpdateProgress(display, 50, getStr(s_Updating_weather));
  updateCurrentWeather( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 60, getStr(s_Calculate_moon_phase));
  updateAstronomy( firstStart, conf.latitude, conf.longitude);
  
  drawUpdateProgress(display, 70, getStr(s_Updating_forecasts));
  updateForecast( conf.useMetric, conf.language, conf.location, conf.openweatherApiKey);
  
  drawUpdateProgress(display, 80, getStr(s_Connecting_InfluxDB));
  influxdbHelper.update( firstStart, getDeviceID(),  WiFi.SSID(), VERSION, conf.location, conf.useMetric);

  drawUpdateProgress(display, 100, getStr(s_Done));

  if (firstStart) {
    //Save temperature for the chart
    saveDHTTempHist( conf.useMetric);
  }

  influxdbHelper.writeStatus(resetReason);
  resetReason = (char *)nullptr;


  digitalWrite( LED, HIGH);
  delay(500);
}

uint16_t nextUIUpdate = 0;

void loop() {
  station.loop();
  // Needs to be done asynchronously
  if(shouldDrawWifiProgress) {
    drawWifiProgress(&display, VERSION, wifiSSID.c_str());
  }
  if(shouldSetupInfluxDb) {
    influxdbHelper.begin(station.getInfluxDBSettings());
    influxdbHelper.update( true, getDeviceID(),  WiFi.SSID(), VERSION, conf.location, conf.useMetric);
    shouldSetupInfluxDb = false;
  }
  if(!initialized) {
    initData();
  }
  // Ticker function - executed once per minute
  if (ui.getUiState()->frameState == FIXED && (millis() - timeSinceLastUpdate >= 1000*60)) {
    WS_DEBUG_RAM("Loop 1");
    timeSinceLastUpdate = millis();
    lastUpdateMins++;

    refreshDHTCachedValues(conf.useMetric);

    if(WiFi.isConnected()) {
      if(station.getUpdaterSettings()->updateTime < 2400) {
        //TODO: change to an alarm like functionality
        time_t tnow = time(nullptr);
        struct tm timeinfo;
        localtime_r(&tnow, &timeinfo);
        uint16_t curtm = timeinfo.tm_hour*100+timeinfo.tm_min;
        if (curtm == station.getUpdaterSettings()->updateTime ) {
          influxdbHelper.release();
          WS_DEBUG_RAM("After release");
          delay(1000);
          WS_DEBUG_RAM("After delay");
          if(!updater.checkUpdate()) {
            influxdbHelper.begin(station.getInfluxDBSettings());
          }
        }
      }

      //Sync IoT Center configuration
      if (lastUpdateMins % conf.iotRefreshMin == 0) {
        digitalWrite( LED, LOW);
        // TODO: better solution for updating Settings from IoT center
        auto influxDBSettings = station.getInfluxDBSettings();
        if(loadIoTCenter(false, conf.iotCenterUrl, getDeviceID(), influxDBSettings,  conf.iotRefreshMin, conf.latitude, conf.longitude)) {
          station.getPersistence()->writeToFS(influxDBSettings);
          influxDBSettings->notify();
        }
        digitalWrite( LED, HIGH);
      }

      //Update data?
      if (lastUpdateMins % conf.updateDataMin == 0) {
        digitalWrite( LED, LOW);
        updateData(&display,false);
        digitalWrite( LED, HIGH);
      }

      //Write into InfluxDB
      if (lastUpdateMins % station.getInfluxDBSettings()->writeInterval == 0) {
        digitalWrite( LED, LOW);
        saveDHTTempHist( conf.useMetric);  //Save temperature for the chart
        ESP.wdtFeed();
        influxdbHelper.write( getDHTTemp( true), getDHTHum(), conf.latitude, conf.longitude);  //aways save in celsius
        Serial.print(F("InfluxDB write "));
        Serial.println(String(millis() - timeSinceLastUpdate) + String(F("ms")));
        digitalWrite( LED, HIGH);
        WS_DEBUG_RAM("After write");
      }
    }
  }
      
  //Handle BOOT button
  unsigned int loops = 0;
  while (digitalRead(BUTTONHPIN) == LOW) {  //Pushed boot button?
    if (loops == 0) {
      ui.nextFrame();   //jump to the next frame
    }
    if (loops > 4)
      showConfiguration(&display, (200 - loops) / 10, VERSION, timeSinceLastUpdate + (conf.updateDataMin - ((lastUpdateMins % conf.updateDataMin)) * 60 * 1000), getDeviceID(), &influxdbHelper);  //Show configuration after 0.5s

    loops++;
    if (loops > 200) {  //factory reset after 20 seconds
      station.getPersistence()->removeConfigs();
      ESP.restart();
    }

    delay(100);
  }

  if(initialized && !isAPRunning && (!nextUIUpdate || (int(nextUIUpdate - millis())<=0 ))) {
    ESP.wdtFeed();
    int remainingTimeBudget = ui.update();
    nextUIUpdate = millis()+remainingTimeBudget;
  }
}

bool isInfluxDBError() {
  return influxdbHelper.isError();
}

void wifiConnectionEventHandler(WifiConnectionEvent event, const char *ssid) {
  // WiFi interrupt events, don't do anything complex, just update state variables
 switch(event) {
    case WifiConnectionEvent::ConnectingStarted:
      shouldDrawWifiProgress = true;
      startWifiProgress(&display, VERSION, ssid);
      //TODO: better solution for passing current wifi
      wifiSSID = ssid;
      break;
    case WifiConnectionEvent::ConnectingSuccess:
      shouldDrawWifiProgress = false;
      station.getWifiManager()->setWiFiConnectionEventHandler(nullptr);
      break;        
  };
}

void wifiAPEventHandler(WifiAPEvent event, APInfo *info){
  switch(event) {
    case WifiAPEvent::APStarted:
      if(shouldDrawWifiProgress) {
        shouldDrawWifiProgress = false;
        station.getWifiManager()->setWiFiConnectionEventHandler(nullptr);
      }
      drawAPInfo(&display, info);
      isAPRunning = true;
      break;
    case WifiAPEvent::ClientConnected:
      drawAPInfo(&display, info);
      break;
    case WifiAPEvent::APStopped:
      isAPRunning = false;
      break;
  }
}

void updateStartHandler(const char *newVersion) {
  drawFWUpdateInfo(&display, getStr(s_Update_found) + newVersion, getStr(s_Update_start_in));
  delay(1000);
}

void updateProgressHandler(const char *newVersion, int progress) {
  drawFWUpdateProgress(&display, newVersion, progress);
}

void updateFinishedHandler(bool success, const char *err) {
  if(success) {
    drawFWUpdateInfo(&display, getStr(s_Update_successful), getStr(s_Update_restart_in));
    delay(1000);
    drawFWUpdateInfo(&display, "", getStr(s_Update_restarting));
    Serial.println(F("restarting"));
    station.end();
    ESP.restart();
  } else {
    drawFWUpdateInfo(&display, getStr(s_Update_failed),  err);
    delay(3000);
  }
}
