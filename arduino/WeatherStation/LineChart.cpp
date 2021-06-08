#include <OLEDDisplayUi.h>

const int Now = 1547;                   // To set the time; eg 15:47
unsigned long StartMins = (unsigned long)((Now/100)*60 + (Now%100));


void drawLineChart(OLEDDisplay *display, /*int samples[],*/ unsigned int samples, int16_t x, int16_t y) {
  unsigned int SampleNo = StartMins/15;
  // Plot temperature graph
  int x1 = 16, y1 = 11;
  int yscale = 2;                         // Points per degree
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(26 + x, 56 + y, F("Temperature °C"));

  // Horizontal axis
  display->drawHorizontalLine(x1 + x, y1 + y, 96);

  for (int i=0; i<=24; i=i+4) {
    int mark = x1+i*4;
    display->drawLine( mark + x, y1 + y, mark + x, y1-2 + y);
    int tens = i/10;
    if (tens != 0) {
      display->drawString( mark-6 + x, y1-12 + y, String((char)(tens+'0')));
      display->drawString( mark + x, y1-12 + y, String((char)(i%10+'0')));
    } else 
      display->drawString( mark-3 + x, y1-12 + y, String((char)(i%10+'0')));
  }
  
  // Vertical axis
  display->drawLine( x1 + x, y1 + y, x1 + x, y1+50+y);
  for (int i=5; i<=25; i=i+5) {
    int mark = y1+i*yscale-10;
    display->drawLine( x1 + x, mark + y, x1-2 + x, mark + y);
    int tens = i/10;
    if (tens != 0)
      display->drawString( x1-15 + x, mark-3 + y, String((char)(tens+'0')));
    display->drawString( x1-9 + x, mark-3 + y, String((char)(i%10+'0')));
  }
  if (samples > 96)
    samples = 96;
  for (unsigned int i = 0; i < samples; i++) {
    int Temperature = ((sin(float(i)/10) + 1) * 20) + 11;
    display->setPixel( i+x1 + x, Temperature-10+y1 + y);
  }
}

void drawLineChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawLineChart( display, 96, x, y);
}
