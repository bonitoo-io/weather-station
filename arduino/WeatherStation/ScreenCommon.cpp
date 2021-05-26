#include <OLEDDisplayUi.h>
#include <ESPWiFi.h>
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "tools.h"

extern tConfig conf;
extern String deviceID;
float getDHTTemp(bool metric);
float getCurrentWeatherTemperature();
bool errorInfluxDB();
String errorInfluxDBMsg();

void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawWindForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

// This array keeps function pointers to all frames, frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTimeAnalog, drawDateTime, drawDHT, drawCurrentWeather, drawForecast, drawWindForecast, drawAstronomy};
//FrameCallback frames[] = { drawDateTimeAnalog};
OverlayCallback overlays[] = { drawHeaderOverlay };

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
  ESP.wdtFeed();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
  display->drawString(88, 5, String("Weather Station\nby InfluxData\nV") + version);
  display->display();
}

void drawWifiProgress(OLEDDisplay *display, const char* version) {
  int counter = 0;
  Serial.print("Wifi " + conf.wifi_ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    display->clear();
    display->drawXbm( 0, 0, Logo_width, Logo_height, Logo_bits);
    display->drawString(88, 0, "Connecting WiFi");
    display->drawString(88, 15, conf.wifi_ssid);
    display->drawXbm(71, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display->drawXbm(85, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display->drawXbm(99, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display->drawString(88, 40, String("V") + version);
    display->display();
    counter++;
    delay(500);
  }
  Serial.println();
}

void drawUpdateProgress(OLEDDisplay *display, int percentage, const char* label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
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
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[8];
  sprintf_P(buff, PSTR("%2d:%02d"), conf.use24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));

  if (!conf.use24hour)
    display->drawString(display->getStringWidth(String(buff)), 52, timeInfo->tm_hour>=12?"pm":"am");

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth(), 54, "In:" + String(getDHTTemp( conf.useMetric), 0) + (conf.useMetric ? "°C" : "°F") + " Out:" + String(getCurrentWeatherTemperature(), 0) + (conf.useMetric ? "°C" : "°F"));

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

void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawRect(0, 0, display->getWidth(), display->getHeight());
  if ( secToReset > 5) {
    display->drawString(1,  0, "Wifi " + WiFi.SSID() + " " + String((WiFi.status() == WL_CONNECTED) ? String(getWifiSignal()) + "%" : wifiStatusStr(WiFi.status())));
    display->drawString(1, 10, "Up: " + String(millis()/1000/3600) + "h " + String((millis()/1000)%3600) + "s RAM: " + String( ESP.getFreeHeap()));
    display->drawString(1, 20, "Update in " + String((conf.update_data_min*60*1000 - (millis() - lastUpdate))/1000) + " s");
    display->drawString(1, 30, "InfluxDB " + (!errorInfluxDB() ? deviceID : errorInfluxDBMsg()));
    display->drawString(1, 40, String("V") + version + "; tz: " + String(conf.utcOffset) + " " + conf.language);
    display->drawString(1, 50, "http://" + WiFi.localIP().toString());
  } else
    display->drawString(0, 30, "RESETING IN " + String(secToReset) + "s !");

  display->display();
}
