#include <ESPWiFi.h>
#include "WeatherStation.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "Tools.h"
#include "Sensor.h"
#include "ScreenCommon.h"

const char ScreenConstants::About = 'B';
const char ScreenConstants::DateTimeAnalog = 'A';
const char ScreenConstants::DateTimeDigital = 'D';
const char ScreenConstants::SensorValues = 'I';
const char ScreenConstants::Covid19 = 'S';
const char ScreenConstants::CurrentWeather = 'O';
const char ScreenConstants::WeatherForecast = 'F';
const char ScreenConstants::WindForecast = 'W';
const char ScreenConstants::Astronomy = 'M';
const char ScreenConstants::TemperatureChart = 'T';
const char ScreenConstants::Config = 'C';

const char ScreenConstants::defaultList[] = {DateTimeAnalog, DateTimeDigital, SensorValues, TemperatureChart, Covid19, CurrentWeather, WeatherForecast, WindForecast, Astronomy, Config, '\0'};

// This array keeps function pointers to all frames, frames are the single views that slide from right to left
OverlayCallback overlays[] = { drawHeaderOverlay};

void initOLEDUI(OLEDDisplayUi *ui, DisplaySettings *pDisplaySettings) {
  configureUI(ui, pDisplaySettings);
  ui->init(); // Inital UI takes care of initalising the display too
}

FrameCallback getFrameCallback(char c) {
  switch(c) {
    case ScreenConstants::DateTimeAnalog:
      return drawDateTimeAnalog;
    case ScreenConstants::DateTimeDigital:
      return drawDateTime;
    case ScreenConstants::SensorValues:
      return drawSensor;
    case ScreenConstants::TemperatureChart:
      return drawTemperatureChart;
    case ScreenConstants::Covid19:
      return drawCovid19;
    case ScreenConstants::CurrentWeather:
      return drawCurrentWeather;
    case ScreenConstants::WeatherForecast:
      return drawForecast;
    case ScreenConstants::WindForecast:
      return drawWindForecast;
    case ScreenConstants::Astronomy:
      return drawAstronomy;
    case ScreenConstants::Config:
      return drawAbout;
    default:
      return nullptr;
  }
}

static FrameCallback *pFrames = nullptr;

void configureUI(OLEDDisplayUi *ui, DisplaySettings *pDisplaySettings) {
  ui->setTargetFPS(30);
  ui->setTimePerFrame(pDisplaySettings->screenRotateInterval*1000);
  ui->setActiveSymbol(activeSymbole);
  ui->setInactiveSymbol(inactiveSymbole);
  ui->setIndicatorPosition(TOP);
  ui->setIndicatorDirection(LEFT_RIGHT);   // Defines where the first frame is located in the bar.
  ui->setFrameAnimation(SLIDE_LEFT);
  ui->setOverlays(overlays, sizeof(overlays) / sizeof(OverlayCallback));
  if(pFrames) {
    delete [] pFrames;
  }
  pFrames = new FrameCallback[pDisplaySettings->screens.length()];
  int8_t i=0;
  for(char c: pDisplaySettings->screens) {
    FrameCallback f = getFrameCallback(c);
    if(f) {
      pFrames[i++] = f;
    }
  }
  ui->setFrames(pFrames, i);
}

uint32_t wifiProgressCounter;
uint32_t wifiProgressLastDrown;

void drawWifiProgress(OLEDDisplay *display, const char* version, const char *ssid) {
  if(!wifiProgressLastDrown || (millis()-wifiProgressLastDrown >= 500)) {
    Serial.print(F("."));

    display->clear();
    display->drawXbm( 0, 0, Logo_width, Logo_height, Logo_XBM);
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);

    if (wifiProgressCounter < 3) {  //show splash screen
      display->drawString(88, 5, String(F("Weather Station\nby bonitoo.io\nv")) + version);
    } else {  //show Wifi connecting screen
      display->drawString(88, 0, getStr(s_Connecting_WiFi));
      if (strlen(ssid) > 0)
        display->drawString(88, 15, ssid);
      else
        display->drawString(88, 15, getStr(s_Searching));
      display->drawXbm(71, 30, 8, 8, wifiProgressCounter % 3 == 0 ? activeSymbole : inactiveSymbole);
      display->drawXbm(85, 30, 8, 8, wifiProgressCounter % 3 == 1 ? activeSymbole : inactiveSymbole);
      display->drawXbm(99, 30, 8, 8, wifiProgressCounter % 3 == 2 ? activeSymbole : inactiveSymbole);
      display->drawString(88, 36, getStr(s_or_wait_for_setup));
      display->drawString(88, 47, String(F("v")) + version);
    }

    if ( isnan( pSensor->getTempF()) || isnan( pSensor->getHum()) || (pSensor->getTempF() == 0) || (pSensor->getHum() == 0))
      display->drawXbm( 0, 0, 8, 8, warning_8x8);

    display->display();
    wifiProgressCounter++;
    wifiProgressLastDrown = millis();
  }
}

void startWifiProgress(OLEDDisplay *display, const char* version, const char *ssid) {
  wifiProgressCounter = 0;
  wifiProgressLastDrown = 0;
  drawWifiProgress(display, version, ssid);
}

void drawAPInfo(OLEDDisplay *display, APInfo *info) {
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, getStr(s_Wifi_configure));

  if (!info->clientsCount) {    //Any connected client to Wifi?
    display->drawString(0, 13, getStr(s_Wifi_AP_connect));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0, 45, info->ssid);
  } else {
    display->drawString(0, 13, getStr(s_Wifi_web_point));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0, 45, String(F("http://")) + info->ipAddress.toString());
  }
  if ( isnan( pSensor->getTempF()) || isnan( pSensor->getHum()) || (pSensor->getTempF() == 0) || (pSensor->getHum() == 0))
    display->drawXbm( 0, 0, 8, 8, warning_8x8);
  display->display();
}

void drawUpdateProgress(OLEDDisplay *display, int percentage, const String& label) {
  Serial.println( label);
  ESP.wdtFeed();
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 53, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void drawFWUpdateInfo(OLEDDisplay *display, const String &firstLine, const String &secondLine) {
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 10, firstLine);
  display->drawString(64, 25, secondLine);
  display->display();
}

void drawFWUpdateProgress(OLEDDisplay *display, const char* version, int percent) {
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 53, getStr(s_Updating_to) + version);
  display->drawProgressBar(2, 28, 124, 10, percent);
  display->display();
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

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  //Draw Wifi signal level
  int8_t quality = getWifiSignal();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0)
        display->setPixel(120 + 2 * i, 6 - j);
    }
  }

  //Draw InfluxDB write mark
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  if ( isInfluxDBError())
    display->drawXbm( 0, 0, 8, 8, warning_8x8);

  display->drawHorizontalLine(0, 51, display->getWidth());
  time_t now = time(nullptr);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 53, strTime(now, true));
  display->drawString(display->getStringWidth(F("00:00")), 51, strTimeSuffix(now));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth(), 53, getStr(s_In) + Sensor::strTemp(pSensor->getTempF()) + getStr(s_Out) + Sensor::strTemp( Sensor::int2Float( getCurrentWeatherTemperature())));
}

const __FlashStringHelper * wifiStatusStr(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return F("No shield");
    case WL_IDLE_STATUS: return F("Idle");
    case WL_NO_SSID_AVAIL: return F("No SSID available");
    case WL_SCAN_COMPLETED: return F("Scan completed");
    case WL_CONNECTED: return F("Connected");
    case WL_CONNECT_FAILED: return F("Connect failed");
    case WL_CONNECTION_LOST: return F("Connection lost");
    case WL_DISCONNECTED: return F("Disconnected");
    default: return F("Unknown");
  }
}

void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate, const char *deviceID, InfluxDBHelper *influxDBHelper) {
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawHorizontalLine(0, 0, display->getWidth());
  display->drawHorizontalLine(0, display->getHeight()-1, display->getWidth());
  if ( secToReset > 5) {
    if ( WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
      display->drawString(0,  0, String(F("AP ")) + WiFi.softAPIP().toString() + String(F(" Clients: ")) + String(WiFi.softAPgetStationNum()));
    else
      display->drawString(0,  0, String(F("Wifi ")) + WiFi.SSID() + String(F(" ")) + String((WiFi.status() == WL_CONNECTED) ? String(getWifiSignal()) + String(F("%")) : String(wifiStatusStr(WiFi.status()))));
    display->drawString(0,  9, String(F("Up: ")) + String(millis()/1000/3600) + String(F("h ")) + String((millis()/1000)%3600) + String(F("s RAM: ")) + String( ESP.getFreeHeap()));
    display->drawString(0, 18, String(F("Update in ")) + String((station.getAdvancedSettings()->updateDataInterval*60*1000 - (millis() - lastUpdate))/1000) + String(F("s ")) + (isnan( pSensor->getTempF()) ? String( F("None")) : String(pSensor->getSensorName())));
    display->drawString(0, 29, String(F("DB ")) + (!influxDBHelper->isError() ? deviceID : influxDBHelper->errorMsg()));
    display->drawString(0, 38, String("v") + version + " " + station.getRegionalSettings()->language + " " + String(station.getRegionalSettings()->utcOffset) + String(F(" ")) + String(station.getAdvancedSettings()->getTempOffset(),2) + "/" + String(station.getAdvancedSettings()->getHumOffset(),2));
    if ( WiFi.localIP().isSet())
      display->drawString(0, 50, String(F("http://")) + WiFi.localIP().toString());
    else
      display->drawString(0, 50, String( Sensor::tempF2C(pSensor->getTempF()), 2) + String(F("°C ") + String(pSensor->getHum(), 2)) + "%");
  } else
    display->drawString(0, 30, String(F("FACTORY RESET IN ")) + String(secToReset) + String(F("s !")));

  display->display();
}

void drawAbout(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString((display->getWidth() / 2) + x, 5 + y, getStr(s_InfluxData_Weather_Station));
  display->drawRect(8 + x, 17 + y, display->getWidth() - 16, 23);
  display->drawString((display->getWidth() / 2) + x, 17 + y, getStr(s_Configure_via));
  display->drawString((display->getWidth() / 2) + x, 27 + y, String(F("http://")) + WiFi.localIP().toString());
  display->drawString((display->getWidth() / 2) + x, 38 + y, String(F("Id: ")) + getDeviceID());
}

/*void showFont(OLEDDisplay *display, const uint8_t *fontData) {
  Serial.println( "showFont");
  int from = pgm_read_byte_near(fontData+2);
  int to = pgm_read_byte_near(fontData+2)+pgm_read_byte_near(fontData+3)-1;
  Serial.println( "from " + String(from) + " to " +  String(to));
  bool but;
  while (true)
  for (char i=from; i<=to; i++) {
    but = false;
    display->clear();
    display->setFont(fontData);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 0, String((char)(i)));
    int len = display->getStringWidth(String((char)(i)));
    display->setFont(ArialMT_Plain_10);
    if ( len == 0) {
      display->drawString(0, 0, F("<empty>"));
      continue;
    }
    display->drawString(0, 40, String((int)(i)) + " " + String((char)(i)) + "\n " + String(from) + "-" + String(to));
    display->display();
    while (digitalRead(D3) == LOW) { //wait if the button is pressed
      delay( 200);
      but = true;
    }
    if (!but)
      delay( 1000);
  }
}*/
