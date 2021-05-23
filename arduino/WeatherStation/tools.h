#pragma once
#include <Arduino.h>

// Adjust according to your language
extern const char* const WDAY_NAMES[];
extern const char* const MONTH_NAMES[];
extern const char* const MOON_PHASES[];

String strTime(time_t timestamp, bool shortTime);
String strDate(time_t timestamp, bool shortDate);
String utf8ascii(const String s);
