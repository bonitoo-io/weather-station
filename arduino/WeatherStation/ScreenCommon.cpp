#include <OLEDDisplayUi.h>
#include <ESPWiFi.h>
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "Tools.h"
#include "WiFi.h"
#include "InfluxDBHelper.h"

float getDHTTemp(bool metric);
float getCurrentWeatherTemperature();

void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawWindForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
  

// This array keeps function pointers to all frames, frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTimeAnalog, drawDateTime, drawDHT, drawTemperatureChart, drawCurrentWeather, drawForecast, drawWindForecast, drawAstronomy};
//FrameCallback frames[] = { drawTemperatureChart};
OverlayCallback overlays[] = { drawHeaderOverlay};

void setupOLEDUI(OLEDDisplayUi *ui) {
  ui->setTargetFPS(30);
  ui->setTimePerFrame(10000);
  ui->setActiveSymbol(activeSymbole);
  ui->setInactiveSymbol(inactiveSymbole);
  ui->setIndicatorPosition(TOP);
  ui->setIndicatorDirection(LEFT_RIGHT);   // Defines where the first frame is located in the bar.
  ui->setFrameAnimation(SLIDE_LEFT);

  ui->setFrames(frames, sizeof(frames) / sizeof(FrameCallback));
  ui->setOverlays(overlays, sizeof(overlays) / sizeof(OverlayCallback));
  ui->init(); // Inital UI takes care of initalising the display too
}


void drawSplashScreen(OLEDDisplay *display, const char* version) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
  display->drawString(88, 5, String(F("Weather Station\nby InfluxData\nV")) + version);
  display->display();
}

int wifiProgressCounter;
uint32_t wifiProgressLastDrown;

void drawWifiProgress(OLEDDisplay *display, const char* version, const char *ssid) {
  if(!wifiProgressLastDrown || (millis()-wifiProgressLastDrown >= 500)) {
    Serial.print(F("."));
    display->clear();
    display->drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
    display->drawString(88, 0, getStr(s_Connecting_WiFi));
    display->drawString(88, 15, ssid);
    display->drawXbm(71, 30, 8, 8, wifiProgressCounter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display->drawXbm(85, 30, 8, 8, wifiProgressCounter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display->drawXbm(99, 30, 8, 8, wifiProgressCounter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display->drawString(88, 40, String(F("V")) + version);
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
  display->drawString(0, 0, getStr(s_Wifi_AP_connect));
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 12, info->ssid);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 24, getStr(s_Wifi_web_point));
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 36, info->ipAddress.toString());
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 48, getStr(s_Wifi_configure));
  display->display();
}

void drawUpdateProgress(OLEDDisplay *display, int percentage, const String& label) {
  ESP.wdtFeed();  
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
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
  display->drawString(64, 10, getStr(s_Updating_to) + version);
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
  time_t now = time(nullptr);

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, strTime(now,true));
  display->drawString(display->getStringWidth(F("00:00")), 52, strTimeSuffix(now));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth(), 54, getStr(s_In) + strTemp(getDHTTemp( conf.useMetric)) + getStr(s_Out) + strTemp(getCurrentWeatherTemperature()));

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

void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate, const String deviceID) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawRect(0, 0, display->getWidth(), display->getHeight());
  if ( secToReset > 5) {
    display->drawString(1,  0, String(F("Wifi ")) + WiFi.SSID() + String(F(" ")) + String((WiFi.status() == WL_CONNECTED) ? String(getWifiSignal()) + String(F("%")) : String(wifiStatusStr(WiFi.status()))));
    display->drawString(1, 10, String(F("Up: ")) + String(millis()/1000/3600) + String(F("h ")) + String((millis()/1000)%3600) + String(F("s RAM: ")) + String( ESP.getFreeHeap()));
    display->drawString(1, 20, String(F("Update in ")) + String((conf.updateDataMin*60*1000 - (millis() - lastUpdate))/1000) + String(F(" s")));
    display->drawString(1, 30, String(F("InfluxDB ")) + (!errorInfluxDB() ? deviceID : errorInfluxDBMsg()));
    display->drawString(1, 40, String("V") + version + String(F("; tz: ")) + String(conf.utcOffset) + String(F(" ")) + conf.language);
    display->drawString(1, 50, String(F("http://")) + WiFi.localIP().toString());
  } else
    display->drawString(0, 30, String(F("FACTORY RESET IN ")) + String(secToReset) + String(F("s !")));

  display->display();
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
