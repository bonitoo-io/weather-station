//Board: NodeMCU 1.0 (ESP-12E Module)

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
#include "ServiceState.h"
#include "ScreenCommon.h"

/***************************
 * Begin Settings
 **************************/
//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY ""

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
  IOT_CENTER_URL, //iotCenterUrl 
  60             //iotRefreshMin
};

// Initialize the oled display
SSD1306Wire display(I2C_OLED_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui( &display);

InfluxDBHelper influxdbHelper;
// Weather station backend
WeatherStation station(&influxdbHelper);
Updater updater;


unsigned long timeSinceLastUpdate = 0;
unsigned int lastUpdateMins = 0;
String wifiSSID;
bool shouldSetupInfluxDb = false;
bool shouldDrawWifiProgress = false;
bool isAPRunning = false;
bool initialized = false;
String resetReason;



void updateData(OLEDDisplay *display, bool firstStart);
bool loadIoTCenter( bool firstStart, const String& iot_url, const char *deviceID, InfluxDBSettings *influxdbSettings, unsigned int& iotRefreshMin, float& latitude, float& longitude);
int detectLocationFromIP(RegionalSettings *pRegionalSettings);
bool updateClock( bool firstStart, int utc_offset, const String &ntp);
bool updateAstronomy(bool firstStart, const float lat, const float lon);
bool updateCurrentWeather(RegionalSettings *pRegionalSettings, const String& APIKey);
bool updateForecast( RegionalSettings *pRegionalSettings, const String& APIKey);

void initData() {
  if(!initialized && WiFi.isConnected() && !isAPRunning) {
    WS_DEBUG_RAM("InitData");
    updater.init(station.getUpdaterSettings(), VERSION);
    
    //Initialize OLED UI
    initOLEDUI(&ui, station.getAdvancedSettings());
    if(resetReason.length()) {
      influxdbHelper.registerResetInfo(resetReason);
      resetReason = (char *)nullptr;
    }
    ServicesTracker.reset();

    //Load all data
    updateData(&display, true);
   
    initialized = true;
  }
}

bool bForceUpdate = false;

void setup() {
  // Prepare serial port
  Serial.begin(74880);
    //Initialize OLED
  display.init();
  display.clear();
  display.display();
  delay(1000);

  Serial.println();
  Serial.println();
  ESP.wdtEnable(WDTO_8S); //8 seconds watchdog timeout (still ignored) 
  Serial.println(F("Starting Weather station v" VERSION " built " __DATE__ " " __TIME__));
  WS_DEBUG_RAM("Setup 1");
  if(ESP.getResetInfoPtr()->reason != REASON_DEEP_SLEEP_AWAKE) {
    resetReason = ESP.getResetReason();
    Serial.print(F(" Reset reason: "));
    Serial.println(resetReason);
  }

  station.getWifiManager()->setWiFiConnectionEventHandler(wifiConnectionEventHandler);
  station.getWifiManager()->setAPEventHandler(wifiAPEventHandler);
  
  station.getInfluxDBSettings()->setHandler([](){
    shouldSetupInfluxDb =  true;
  });
  
  station.getUpdaterSettings()->setHandler([](){
    updater.init(station.getUpdaterSettings(), VERSION);
  });

  RegionalSettings *pRegionalSettings = station.getRegionalSettings();
  pRegionalSettings->setHandler([pRegionalSettings](){
    if(!pRegionalSettings->detectAutomatically) {
      setLanguage( pRegionalSettings->language.c_str());  
    }
    bForceUpdate = true; 
  });

  station.getAdvancedSettings()->setHandler([](){
      configureUI(&ui, station.getAdvancedSettings());
      bForceUpdate = true;
  });

  updater.setUpdateCallbacks(updateStartHandler,updateProgressHandler,updateFinishedHandler);
  station.setFWUploadFinishedCallback(fwUploadFinishedHandler);
  station.begin();

  WS_DEBUG_RAM("Setup 2");

  // Configure pins
  pinMode(BUTTONHPIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite( LED, HIGH);
  setupDHT();

  setLanguage( pRegionalSettings->language.c_str());  
  refreshDHTCachedValues(pRegionalSettings->useMetricUnits);
  WS_DEBUG_RAM("Setup 3");
  ServicesTracker.load();
}

void updateData(OLEDDisplay *display, bool firstStart) {
  WS_DEBUG_RAM("UpdateData");
  digitalWrite( LED, LOW);

  drawUpdateProgress(display, 0, getStr(s_Detecting_location));
  if (station.getRegionalSettings()->detectAutomatically) {
    WS_DEBUG_RAM("Before IPloc");
    station.stopServer();
    WS_DEBUG_RAM("After stop server");
    if(!firstStart) {
      influxdbHelper.release();
      WS_DEBUG_RAM("After cl release");
    }
    delay(1000);
    WS_DEBUG_RAM("After delay");
    //Load location data from IP
    ServicesTracker.updateServiceState(SyncServices::ServiceLocationDetection, ServiceState::SyncStarted);
    ServicesTracker.save();
    int res = detectLocationFromIP(station.getRegionalSettings());
    if(!res) {
      ServicesTracker.updateServiceState(SyncServices::ServiceLocationDetection, ServiceState::SyncFailed);
    } else {
      ServicesTracker.updateServiceState(SyncServices::ServiceLocationDetection, ServiceState::SyncOk);
      if(res==2) {
        station.saveRegionalSettings();
        setLanguage( station.getRegionalSettings()->language.c_str());
      }
    } 
    
    WS_DEBUG_RAM("After IPloc");
    station.startServer();
    if(!firstStart) {
      influxdbHelper.begin(station.getInfluxDBSettings());
    }
  }

  drawUpdateProgress(display, 10, getStr(s_Updating_time));
  ServicesTracker.updateServiceState(SyncServices::ServiceClock, ServiceState::SyncStarted);
  ServicesTracker.save();
  if(updateClock( firstStart, station.getRegionalSettings()->utcOffset, station.getAdvancedSettings()->ntpServers)) {
    ServicesTracker.updateServiceState(SyncServices::ServiceClock, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceClock, ServiceState::SyncFailed);
  }


  drawUpdateProgress(display, 20, getStr(s_Checking_update));
  if(firstStart && station.getUpdaterSettings()->updateTime < 2400) {
    ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncStarted);
    ServicesTracker.save();
    WS_DEBUG_RAM("Before GH update");
    if(updater.checkUpdate()) {
      ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncOk);
    } else {
      ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncFailed);
    }
    WS_DEBUG_RAM("After GH update");
  }

  drawUpdateProgress(display, 30, getStr(s_Connecting_IoT_Center));
  if (firstStart) {
    if(conf.iotCenterUrl.length()) {
      // TODO: better solution for updating Settings from IoT center
      auto influxDBSettings = station.getInfluxDBSettings();
      ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncStarted);
      ServicesTracker.save();
      if(loadIoTCenter(firstStart, conf.iotCenterUrl, getDeviceID(), influxDBSettings,  conf.iotRefreshMin, station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) {
        ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncOk);
        station.getPersistence()->writeToFS(influxDBSettings);
        influxDBSettings->notify();
      } else {
        ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncFailed);
      }
    } else {
      influxdbHelper.begin(station.getInfluxDBSettings());
    }
  }

  drawUpdateProgress(display, 50, getStr(s_Updating_weather));
  ServicesTracker.updateServiceState(SyncServices::ServiceCurrentWeather, ServiceState::SyncStarted);
  ServicesTracker.save();
  if(updateCurrentWeather( station.getRegionalSettings(), station.getAdvancedSettings()->openWeatherAPIKey)) {
    ServicesTracker.updateServiceState(SyncServices::ServiceCurrentWeather, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceCurrentWeather, ServiceState::SyncFailed);
  }
  
  ServicesTracker.updateServiceState(SyncServices::ServiceAstronomy, ServiceState::SyncStarted);
  ServicesTracker.save();
  drawUpdateProgress(display, 60, getStr(s_Calculate_moon_phase));
  if(updateAstronomy( firstStart, station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) {
    ServicesTracker.updateServiceState(SyncServices::ServiceAstronomy, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceAstronomy, ServiceState::SyncFailed);
  }
  
  drawUpdateProgress(display, 70, getStr(s_Updating_forecasts));
  ServicesTracker.updateServiceState(SyncServices::ServiceForecast, ServiceState::SyncStarted);
  ServicesTracker.save();
  if(updateForecast(station.getRegionalSettings(), station.getAdvancedSettings()->openWeatherAPIKey)) {
    ServicesTracker.updateServiceState(SyncServices::ServiceForecast, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceForecast, ServiceState::SyncFailed);
  }

  ServicesTracker.save(true);
  
  drawUpdateProgress(display, 80, getStr(s_Connecting_InfluxDB));

  influxdbHelper.update( firstStart, getDeviceID(),  WiFi.SSID(), VERSION, station.getRegionalSettings()->location, station.getRegionalSettings()->useMetricUnits);

  drawUpdateProgress(display, 100, getStr(s_Done));

  if (firstStart) {
    //Save temperature for the chart
    saveDHTTempHist( station.getRegionalSettings()->useMetricUnits);
  }

  WS_DEBUG_RAM("After updates");

  ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncStarted);
  ServicesTracker.save();
  if(influxdbHelper.writeStatus()) {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncFailed);
  }
  ServicesTracker.save();
  WS_DEBUG_RAM("After write status");
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
    influxdbHelper.update( true, getDeviceID(),  WiFi.SSID(), VERSION, station.getRegionalSettings()->location,station.getRegionalSettings()->useMetricUnits);
    shouldSetupInfluxDb = false;
  }
  if(!initialized) {
    initData();
  }
  // Ticker function - executed once per minute
  if (ui.getUiState()->frameState == FIXED && (millis() - timeSinceLastUpdate >= 1000*60) || bForceUpdate) {
    WS_DEBUG_RAM("Loop 1");
    timeSinceLastUpdate = millis();
    lastUpdateMins++;
    refreshDHTCachedValues(station.getRegionalSettings()->useMetricUnits);
    ServicesTracker.reset();

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
          station.stopServer();
          WS_DEBUG_RAM("After stop server");
          delay(1000);
          WS_DEBUG_RAM("After delay");
          ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncStarted);
          ServicesTracker.save();
          WS_DEBUG_RAM("Before GH update");
          if(updater.checkUpdate()) {
             ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncOk);
          } else {
            ServicesTracker.updateServiceState(SyncServices::ServiceFWUpdate, ServiceState::SyncFailed);
          }
          WS_DEBUG_RAM("After GH update");
          station.startServer();
          WS_DEBUG_RAM("After start server");
          influxdbHelper.begin(station.getInfluxDBSettings());
          influxdbHelper.update( false, getDeviceID(),  WiFi.SSID(), VERSION, station.getRegionalSettings()->location, station.getRegionalSettings()->useMetricUnits);
        }
      }

      //Sync IoT Center configuration
      if (lastUpdateMins % conf.iotRefreshMin == 0 && conf.iotCenterUrl.length()) {
        digitalWrite( LED, LOW);
        // TODO: better solution for updating Settings from IoT center
        auto influxDBSettings = station.getInfluxDBSettings();
        ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncStarted);
        ServicesTracker.save();
        if(loadIoTCenter(false, conf.iotCenterUrl, getDeviceID(), influxDBSettings,  conf.iotRefreshMin, station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) {
          ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncOk);
          station.getPersistence()->writeToFS(influxDBSettings);
          influxDBSettings->notify();
        } else {
          ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncFailed);
        }
        ServicesTracker.save();
        digitalWrite( LED, HIGH);
      }

      if(influxdbHelper.wasReleased()) {
        influxdbHelper.begin(station.getInfluxDBSettings());
      }

      //Update data?
      if (lastUpdateMins % station.getAdvancedSettings()->updateDataInterval == 0 || bForceUpdate) {
        digitalWrite( LED, LOW);
        updateData(&display,false);
        digitalWrite( LED, HIGH);
        bForceUpdate = false;
      }

      //Write into InfluxDB
      if (lastUpdateMins % station.getInfluxDBSettings()->writeInterval == 0) {
        digitalWrite( LED, LOW);
        saveDHTTempHist( station.getRegionalSettings()->useMetricUnits);  //Save temperature for the chart
        ESP.wdtFeed();
        ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncStarted);
        ServicesTracker.save();
        //always save in celsius
        unsigned long start = millis();
        if(influxdbHelper.write( getDHTTemp( true), getDHTHum(), station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) { 
          ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncOk);
        } else {
          ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncFailed);
        }
        ServicesTracker.save();
        Serial.print(F("InfluxDB write "));
        Serial.println(String(millis() - start) + String(F("ms")));
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
      showConfiguration(&display, (200 - loops) / 10, VERSION, timeSinceLastUpdate + (station.getAdvancedSettings()->updateDataInterval - ((lastUpdateMins % station.getAdvancedSettings()->updateDataInterval)) * 60 * 1000), getDeviceID(), &influxdbHelper);  //Show configuration after 0.5s

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
      if(!shouldDrawWifiProgress) {
        shouldDrawWifiProgress = true;
        startWifiProgress(&display, VERSION, ssid);
        //TODO: better solution for passing current wifi
        wifiSSID = ssid;
      }
      break;
    case WifiConnectionEvent::ConnectingUpdate: 
      wifiSSID = ssid;
      break;
    case WifiConnectionEvent::ConnectingSuccess:
      shouldDrawWifiProgress = false;
      station.getWifiManager()->setWiFiConnectionEventHandler(nullptr);
      station.startServer();
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
      station.startServer();
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

// Finished automatic download firmware
void updateFinishedHandler(bool success, const char *err) {
  if(success) {
    drawFWUpdateInfo(&display, getStr(s_Update_successful), getStr(s_Update_restart_in));
    delay(1000);
    fwUploadFinishedHandler();
  } else {
    drawFWUpdateInfo(&display, getStr(s_Update_failed),  err);
    delay(3000);
  }
}

// Finished upload firmware via web
void fwUploadFinishedHandler() {
  drawFWUpdateInfo(&display, "", getStr(s_Update_restarting));
  Serial.println(F("restarting"));
  station.end();
  ESP.restart();
}

// Implementation must be in ino to have this compiled and updated always
const char *getLongVersion() {
  static const char *version = VERSION " built " __DATE__ " " __TIME__;
  return version;
}
