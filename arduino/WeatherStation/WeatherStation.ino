/*
Board: Lolin D1 mini (clone) (ESP-12E Module)
Executable segment sizes:
ICACHE : 32768           - flash instruction cache
IROM   : 939468          - code in flash         (default or ICACHE_FLASH_ATTR)
IRAM   : 29521   / 32768 - code in IRAM          (IRAM_ATTR, ISRs...)
DATA   : 1712  )         - initialized variables (global, static) in RAM/HEAP
RODATA : 7560  ) / 81920 - constants             (global, static) in RAM/HEAP
BSS    : 27936 )         - zeroed variables      (global, static) in RAM/HEAP
Sketch uses 978261 bytes (93%) of program storage space. Maximum is 1044464 bytes.
Global variables use 37208 bytes (45%) of dynamic memory, leaving 44712 bytes for local variables. Maximum is 81920 bytes.
*/

#include <Arduino.h>
#include <ESPWiFi.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include "WeatherStation.h"
#include "InfluxDBHelper.h"
#include "Tools.h"
#include "Updater.h"
#include "Sensor.h"
#include "Debug.h"
#include "Version.h"
#include "ServiceState.h"
#include "ScreenCommon.h"

//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define OPEN_WEATHER_MAP_API_KEY ""
// IoT Center url, e.g. http://192.168.1.20:5000  (can be empty, if not configured)
#define IOT_CENTER_URL ""

// Button and LED
#define PIN_LED    D2   //LED pin  GPIO4
#define PIN_BUTTON D3   //Boot button pin  GPIO0
// I2C settings
#define PIN_SDA    D4   //GPIO2
#define PIN_SCL    D5   //GPIO14
#define I2C_OLED_ADDRESS 0x3c

#include "custom_dev.h" //Custom development configuration - remove or comment it out 

tConfig conf = {
//TODO: move iot center config to web config
  IOT_CENTER_URL, //iotCenterUrl 
  60             //iotRefreshMin
};

// Initialize the oled display
SSD1306Wire display(I2C_OLED_ADDRESS, PIN_SDA, PIN_SCL);
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
APInfo *pAPInfo = nullptr;
bool initialized = false;
String resetReason;
bool bForceUpdate = false;
uint16_t nextUIUpdate = 0;

void updateData(OLEDDisplay *display, bool firstStart);
bool loadIoTCenter( bool firstStart, const String& iot_url, const char *deviceID, InfluxDBSettings *influxdbSettings, unsigned int& iotRefreshMin, float& latitude, float& longitude);
int detectLocationFromIP(RegionalSettings *pRegionalSettings);
bool updateClock( bool firstStart, int utc_offset, const String &ntp);
bool updateAstronomy(bool firstStart, const float lat, const float lon);
bool updateCurrentWeather(RegionalSettings *pRegionalSettings, const String& APIKey);
bool updateForecast( RegionalSettings *pRegionalSettings, const String& APIKey);

void initData() {
  if(!initialized && WiFi.isConnected() && !pAPInfo) {
    WS_DEBUG_RAM("InitData");
    updater.init(station.getAdvancedSettings(), VERSION);
    
    //Initialize OLED UI
    initOLEDUI(&ui, station.getDisplaySettings());
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

void setup() {
  // Prepare serial port
  Serial.begin(115200);
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

  ServicesTracker.load();

  //Initialize OLED
  display.init();
  display.clear();
  display.display();

  Serial.print(F("WS crash reset count: "));
  Serial.println(ServicesTracker.getResetCount()-1);
  // repeated reset
  if(ServicesTracker.getResetCount() > 3) {
    for(int i=0; i<100;i++) {
      drawUpdateProgress(&display, i, getStr(s_Reset_wait));
      delay(1000);  
    }
  }

  // Configure pins
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite( PIN_LED, HIGH);
  setupSensor();

  station.getWifiManager()->setWiFiConnectionEventHandler(wifiConnectionEventHandler);
  station.getWifiManager()->setAPEventHandler(wifiAPEventHandler);
  
  station.getInfluxDBSettings()->setHandler([](){
    shouldSetupInfluxDb =  true;
  });
  
  station.getAdvancedSettings()->setHandler([](){
    updater.init(station.getAdvancedSettings(), VERSION);
    bForceUpdate = true;
  });

  RegionalSettings *pRegionalSettings = station.getRegionalSettings();
  pRegionalSettings->setHandler([pRegionalSettings](){
    if(!pRegionalSettings->detectAutomatically) {
      setLanguage( pRegionalSettings->language.c_str());  
    }
    bForceUpdate = true; 
  });

  station.getDisplaySettings()->setHandler([](){
      configureUI(&ui, station.getDisplaySettings());
  });
  
  updater.setUpdateCallbacks(updateStartHandler,updateProgressHandler,updateFinishedHandler);
  station.setFWUploadFinishedCallback(fwUploadFinishedHandler);
  station.begin();
  WS_DEBUG_RAM("Setup 2");

  setLanguage( pRegionalSettings->language.c_str());  
  WS_DEBUG_RAM("Setup 3");
  
}

void updateData(OLEDDisplay *display, bool firstStart) {
  WS_DEBUG_RAM("UpdateData");
  digitalWrite( PIN_LED, LOW);

  if (!firstStart) {
    if (influxdbHelper.getWriteSuccess() == 0) { //without any successfull write?
      Serial.println(F("Failed all writes to InfluxDB, restarting!"));
      //TODO: store status to identify reason of restart
      WiFi.disconnect(true);
      delay(500);
      ESP.restart();
    }
    Serial.print(F("InfluxDB successful writes: "));
    Serial.println( influxdbHelper.getWriteSuccess());
    influxdbHelper.clearWriteSuccess(); //reset OK counter
  }
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
  if(firstStart && station.getAdvancedSettings()->updateTime < 2400) {
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

  if (firstStart) {
    //Save temperature for the chart
    pSensor->saveTempHist();
  }
  drawUpdateProgress(display, 90, getStr(s_Saving_Status));
  
  WS_DEBUG_RAM("After updates");

  ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncStarted);
  ServicesTracker.save();
  if(influxdbHelper.writeStatus()) {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncOk);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteStatus, ServiceState::SyncFailed);
  }
  ServicesTracker.save();
  drawUpdateProgress(display, 100, getStr(s_Done));
  station.startServer();
  WS_DEBUG_RAM("After write status");
  digitalWrite( PIN_LED, HIGH);
}



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
  if (((ui.getUiState()->frameState == FIXED) && ((millis() - timeSinceLastUpdate) >= 1000*60)) || bForceUpdate) {
    WS_DEBUG_RAM("Loop 1");
    timeSinceLastUpdate = millis();
    lastUpdateMins++;
    ServicesTracker.reset();

    if(WiFi.isConnected()) {
      if(station.getAdvancedSettings()->updateTime < 2400) {
        //TODO: change to an alarm like functionality
        time_t tnow = time(nullptr);
        struct tm timeinfo;
        localtime_r(&tnow, &timeinfo);
        uint16_t curtm = timeinfo.tm_hour*100+timeinfo.tm_min;
        if (curtm == station.getAdvancedSettings()->updateTime ) {
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
        digitalWrite( PIN_LED, LOW);
        // TODO: better solution for updating Settings from IoT center
        auto influxDBSettings = station.getInfluxDBSettings();
        ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncStarted);
        ServicesTracker.save();
        if(loadIoTCenter(false, conf.iotCenterUrl, getDeviceID(), influxDBSettings, conf.iotRefreshMin, station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) {
          ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncOk);
          station.getPersistence()->writeToFS(influxDBSettings);
          influxDBSettings->notify();
        } else {
          ServicesTracker.updateServiceState(SyncServices::ServiceIoTCenter, ServiceState::SyncFailed);
        }
        ServicesTracker.save();
        digitalWrite( PIN_LED, HIGH);
      }

      if(influxdbHelper.wasReleased()) {
        influxdbHelper.begin(station.getInfluxDBSettings());
        influxdbHelper.update( false, getDeviceID(),  WiFi.SSID(), VERSION, station.getRegionalSettings()->location, station.getRegionalSettings()->useMetricUnits);
      }

      //Update data?
      if (lastUpdateMins % station.getAdvancedSettings()->updateDataInterval == 0 || bForceUpdate) {
        digitalWrite( PIN_LED, LOW);
        updateData(&display,false);
        digitalWrite( PIN_LED, HIGH);
        bForceUpdate = false;
      }

      //Write into InfluxDB
      if (lastUpdateMins % station.getInfluxDBSettings()->writeInterval == 0) {
        digitalWrite( PIN_LED, LOW);
        pSensor->saveTempHist();  //Save temperature for the chart
        ESP.wdtFeed();
        ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncStarted);
        ServicesTracker.save();
        //always save in celsius
        unsigned long start = millis();
        if(influxdbHelper.write( Sensor::tempF2C( pSensor->getTemp()), pSensor->getHum(), station.getRegionalSettings()->latitude, station.getRegionalSettings()->longitude)) { 
          ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncOk);
        } else {
          ServicesTracker.updateServiceState(SyncServices::ServiceDBWriteData, ServiceState::SyncFailed);
        }
        ServicesTracker.save();
        Serial.print(F("InfluxDB write "));
        Serial.println(String(millis() - start) + String(F("ms")));
        digitalWrite( PIN_LED, HIGH);
        WS_DEBUG_RAM("After write");
      }
    }
    if(!station.isServerStarted()) {
       station.startServer();
    }
  }
      
  //Handle BOOT button
  unsigned int loops = 0;
  while (digitalRead(PIN_BUTTON) == LOW) {  //Pushed boot button?
    if (loops == 0) {
      Serial.println( F("Button BOOT"));
      if(!pAPInfo) {
        ui.nextFrame();   //jump to the next frame
      } else {
        drawAPInfo(&display, pAPInfo);
      }
    }
    if ((loops > 4) || pAPInfo) {
      showConfiguration(&display, (200 - loops) / 10, VERSION, timeSinceLastUpdate + (station.getAdvancedSettings()->updateDataInterval - ((lastUpdateMins % station.getAdvancedSettings()->updateDataInterval)) * 60 * 1000), getDeviceID(), &influxdbHelper);  //Show configuration after 0.5s
    }

    loops++;
    if (loops > 200) {  //factory reset after 20 seconds
      station.getPersistence()->removeConfigs();
      WiFi.disconnect(true);
      delay(500);
      ESP.restart();
    }

    delay(100);
  }
  if (pAPInfo) {
    drawAPInfo(&display, pAPInfo);
  }

  if(initialized && !pAPInfo && (!nextUIUpdate || (int(nextUIUpdate - millis())<=0 ))) {
    ESP.wdtFeed();
    int remainingTimeBudget = ui.update();
    nextUIUpdate = millis()+remainingTimeBudget;
    // reached basic bussiness loop end
    ServicesTracker.clearResetCount();
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
    case WifiConnectionEvent::ConnectingFailed:
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
      pAPInfo = info;
      station.startServer();
      break;
    case WifiAPEvent::ClientConnected:
      drawAPInfo(&display, info);
      pAPInfo = info;
      break;
    case WifiAPEvent::APStopped:
      pAPInfo = nullptr;
      break;
    case WifiAPEvent::ClientDisconnected:
      if(info) {
        drawAPInfo(&display, info);
      }
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
  delay(500);
  ESP.restart();
}

// Implementation must be in ino to have this compiled and updated always
const char *getLongVersion() {
  static const char *version = VERSION " built " __DATE__ " " __TIME__;
  return version;
}
