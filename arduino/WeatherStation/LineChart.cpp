#include <OLEDDisplayUi.h>

const int Now = 1547;                   // To set the time; eg 15:47
unsigned long StartMins = (unsigned long)((Now/100)*60 + (Now%100));


void drawLineChart(OLEDDisplay *display, const char* name, /*int samples[],*/ unsigned int samples, int16_t x, int16_t y) {
  unsigned int SampleNo = StartMins/15;
  // Plot temperature graph
  int x1 = 16, y1 = 39;
  int yscale = 2;                         // Points per degree
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  //display->drawString(64 + x, 5 + y, name);

  // Horizontal axis
  display->drawHorizontalLine(x1 + x, y1 + y, 96);
  for (int i=0; i<=24; i=i+4) {
    int mark = x1+i*4;
    display->drawLine( mark + x, y1 + y, mark + x, y1+2 + y);
    display->drawString( mark + x, y1+1 + y, String(i));
  }
    
  // Vertical axis
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawLine( x1 + x, y1 + y, x1 + x, y1-30+y);
  for (int i=5; i<=20; i=i+5) {
    int mark = y1-(i*yscale-10);
    display->drawLine( x1 + x, mark + y, x1-2 + x, mark + y);
    display->drawString( x1-4 + x, mark - 6 + y, String(i));
  }

  //Data
  if (samples > 96)
    samples = 96;
  for (unsigned int i = 0; i < samples; i++) {
    int data = ((sin(float(i)/10) + 1) * 15) + 11;
    display->setPixel( i+x1 + x, y1 - (data-10 + y));
  }
}

void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawLineChart( display, String(F("Temperature °C")).c_str(), 96, x, y);
}
