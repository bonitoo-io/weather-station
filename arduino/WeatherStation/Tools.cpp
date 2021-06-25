#include "Tools.h"

// English strings
static const char DAYS_ENG[] PROGMEM = "Sun\0" "Mon\0" "Tue\0" "Wed\0" "Thu\0" "Fri\0" "Sat\0";
static const char MONTHS_ENG[] PROGMEM = "Jan\0" "Feb\0" "Mar\0" "Apr\0" "May\0" "Jun\0" "Jul\0" "Aug\0" "Sep\0" "Oct\0" "Nov\0" "Dec\0";
static const char MOON_ENG[] PROGMEM = "new moon\0" "waxing crescent\0" "first quarter\0" "waxing gibbous\0" "full moon\0" "waning gibbous\0" "third quarter\0" "waning crescent\0";
static const char STR_ENG[] PROGMEM = 
"Connecting WiFi\0" "Connecting IoT Center\0" "Detecting location\0" "Checking update\0" "Updating time\0" "Updating weather\0" "Calculate moon phase\0" "Updating forecasts\0" "Connecting InfluxDB\0" "Done\0"
"Found update \0" "Starting update\0" "Update successful\0" "Restarting...\0" "Update failed \0" "Updating \0" "Restarting\0" "1. Connect to WiFi:\0" "2. Open web browser with:\0" "3. Configure WiFi network\0"
"InfluxData Weather Station\0" "Configure via:\0" "for InfluxData by bonitoo.io\0"
"In:\0" " Out:\0" "now\0"
"INDOOR\0" "feel: \0" "hum\0" "wind\0"
"Moon\0" "Sun\0" ;

// Czech strings
static const char DAYS_CZ[] PROGMEM = "Ne\0" "Po\0" "Út\0" "St\0" "Čt\0" "Pá\0" "So\0";
static const char MONTHS_CZ[] PROGMEM = "Leden\0" "Únor\0" "Březen\0" "Duben\0" "Květen\0" "Červen\0" "Červenec\0" "Srpen\0" "Září\0" "Říjen\0" "Listopad\0" "Prosinec\0";
static const char MOON_CZ[] PROGMEM = "nov\0" "dorůstající srpek\0" "první čtvrť\0" "dorůstající měsíc\0" "úplněk\0" "couvající měsíc\0" "poslední čtvrť\0" "ubývající srpek\0";
static const char STR_CZ[] PROGMEM =
"Připojuji WiFi\0" "Připojuji IoT Center\0" "Zjišťuji polohu\0" "Kontrola aktualizace\0" "Aktualizuji čas\0" "Aktualizuji počasí\0" "Vypočítávám fázi měsíce\0" "Aktualizuji předpověď\0" "Připojuji InfluxDB\0" "Hotovo\0"
"Dostupná aktualizace \0" "Připravuji aktualizaci\0" "Aktualizace byla úpěšná\0" "Restartuji...\0" "Aktualizace selhala \0" "Aktualizuji \0" "Restartuji...\0" "1. Připojte WiFi síť\0" "2. V prohlížeči otevřete:\0" "3. Nastavte WiFi síť\0"
"Meteorologická stanice\0" "Nastavte přes:\0" "pro InfluxData od bonitoo.io\0"
"Zde:\0" " Vně:\0" "nyní\0"
"DOMA\0" "pocitově: \0" "vlhkost\0" "vítr\0"
"Měsíc\0" "Slunce\0";

// Add other languages
// ...

struct tLanguage {
  const char* days;
  const char* months;
  const char* moon;
  const char* str;
};

const tLanguage languages[] PROGMEM = {
  {DAYS_ENG, MONTHS_ENG, MOON_ENG, STR_ENG}, //0 - en (default
  {DAYS_CZ, MONTHS_CZ, MOON_CZ, STR_CZ}     //1 - cs
};

const tLanguage* pLang = languages;

String getPgmStr( const char* s, uint8_t index) {
  const char * ps = s;
  while ( index > 0) {
    if ( pgm_read_byte( ps) == 0)
      index--;
    ps++;  
  }
  return utf8ascii( String( FPSTR(ps)));
}

void setLanguage( const char* lang) {
  //Default language is en
  if ( strcmp( lang, "cz") == 0)
    pLang = &languages[1];
  
  //Add other languages
  //....
}

String getDayName( uint8_t index) {
  return getPgmStr(pLang->days, index);
}

String getMonthName( uint8_t index) {
  return getPgmStr(pLang->months, index);
}

String getMoonPhaseName( uint8_t index) {
  return getPgmStr(pLang->moon, index);
}

String getStr( uint8_t index) {
  return getPgmStr(pLang->str, index);
}

String strTime(time_t timestamp, bool shortTime) {
  struct tm *timeInfo = localtime(&timestamp);
  char buf[9];
  if (shortTime)
    sprintf_P(buf, PSTR("%2d:%02d"), conf.use24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min);
  else  
    sprintf_P(buf, PSTR("%02d:%02d:%02d"), conf.use24hour ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min, timeInfo->tm_sec);
  return String(buf);
}

String strTimeSuffix(time_t timestamp) {
  struct tm *timeInfo = localtime(&timestamp);  
  return conf.use24hour ? "" : String(timeInfo->tm_hour>=12?F("pm"):F("am"));
}

String strDate(time_t timestamp, bool shortDate) {
  struct tm* timeInfo = localtime(&timestamp);
  char buff[20];
 
  if (conf.useYMDdate) {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %04d/%2d/%2d"), getDayName( timeInfo->tm_wday), timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
    else  
      sprintf_P(buff, PSTR("%s, %s\n%04d/%02d/%02d"), getDayName( timeInfo->tm_wday), getMonthName( timeInfo->tm_mon), timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
  } else {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %2d.%2d.%04d"), getDayName( timeInfo->tm_wday), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
    else  
      sprintf_P(buff, PSTR("%s, %s\n%02d.%02d.%04d"), getDayName( timeInfo->tm_wday), getMonthName( timeInfo->tm_mon), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  }
  return String(buff);
}

String strTempUnit() {
  return String(conf.useMetric ? F("°C") : F("°F"));
}

String strTemp( float t) {
  return String(t, 0) + strTempUnit();
}

String strHum( float h) {
  return String(h, 0) + String(F("%"));
}

String strWind( float w) {
  return String(w, 0) + String(conf.useMetric ? F("m/s") : F("mph"));  
}

// Convert UTF8-string to extended ASCII

const char sTranslitFrom[] PROGMEM = "ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿřťčůňúďěŘŤČŮŇÚĎĚżąłśćęźńığışŻĄŁŚĆĘŹŃĞŞбвгдежзийклмнпрстуфхцчшщъьюяыэёоаБВГДЕЖЗИЙКЛМНПРСТУФХЦЧШЩЪЬЮЯЫЭЁОАΑαΒβΓγΔδΕεΖζΗηΘθΙιΚκΜμΝνΞξΟοΠπΡρΣσςΤτΥυΦφΧχΨψΩωόλάήΌΛΆΉ";
const char sTranslitTo[] PROGMEM   = "SOZsozYYuAAAAAAACEEEEIIIIDNOOOOOOUUUUYsaaaaaaaceeeeiiiionoooooouuuuyyrtcunudeRTCUNUDEzalsceznigisZALSCEZNGSbvgdejziyklmnprstufhccsswxyyweyoaBVGDEJZIYKLMNPRSTUFHCCSSWXYYWEYOAAaVvGgDdEeZzIiTtIiKkMmNnXxOoPpRrSssTtIiPpKkPpOoolaiOLAI";

char replaceChar( char c1, char c2) {
  //Replace 2-bytes UTF8 characters
  for (unsigned int i=0; i < sizeof(sTranslitFrom) - 1; i+=2) { //try to find character in the translation table
    if ((c1 == pgm_read_byte( sTranslitFrom + i)) && (c2 == pgm_read_byte( sTranslitFrom + i + 1)))
      return pgm_read_byte( sTranslitTo+(i/2));
  }
  //Serial.println( String(c1) + String(c2) + " -> ?");
  return '?';
}

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
static uint8_t c1;  // Last character buffer
char utf8ascii(uint8_t ascii) {
  if (ascii < 128) {   // Standard ASCII-set 0..0x7F handling
    c1 = 0;
    return ascii;
  }

  // get previous input
  uint8_t last = c1;   // get last char
  c1 = ascii;         // remember actual character

  switch (last) {     // conversion depending on first UTF8-character
    case 0xC2: return ascii;  break;
    case 0xC3: return ascii | 0xC0;  break;
    case 0x82: if(ascii==0xAC) return(0x80);       // special case Euro-symbol
  }

  return '?';                                     // otherwise: return ?, if character is uknown
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(const String s) {
  String r(F(""));
  char c;
  char c1 = 0;
  for (unsigned int i=0; i < s.length(); i++) {
    c = s.charAt(i);
    if (c1 != 0) {  //already utf8 char?
      r += replaceChar( c1, c);
      c1 = 0;
      continue;
    }
    if (( c & 0b11100000) == 0b11000000) {  //2-byte utf8 char?
      c1 = c;
      continue;
    }
    r += utf8ascii( c);
  }
  return r;
}

/*void testutf8() {
  //Serial.println( sTranslitFrom);
  if (sTranslitFrom.length() != (sTranslitTo.length()*2))
    Serial.println("ERROR - utf8 translit table");

  Serial.println( sTranslitFrom.length());
  for (int i=0; i < sTranslitFrom.length() - 1; i+=2) {
    if (( sTranslitFrom.charAt(i) & 0b11100000) == 0b11000000) {
      Serial.print( String(sTranslitFrom.charAt(i)) + String(sTranslitFrom.charAt(i+1)) + " ");
      Serial.print( sTranslitFrom.charAt(i), BIN);
      Serial.print( " ");
      Serial.println( sTranslitFrom.charAt(i+1), BIN);
    }
  }
  Serial.println( sTranslitTo);
  Serial.println( sTranslitTo.length()*2);

  Serial.println( utf8ascii("Příliš žluťoučký kůň úpěl ďábelské ódy."));
  Serial.println( utf8ascii("PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY."));
  Serial.println( utf8ascii("Vogt Nyx: „Büß du ja zwölf Qirsch, Kämpe"));
  Serial.println( utf8ascii("Pranzo d'acqua fa volti sghembi"));
  Serial.println( utf8ascii("Stróż pchnął kość w quiz gędźb vel fax myjń"));
  Serial.println( utf8ascii("Benjamín pidió una bebida de kiwi y fresa. Noé, sin vergüenza, la más exquisita champaña del menú "));
  Serial.println( utf8ascii("Pijamalı hasta yağız şoföre çabucak güvendi"));
  Serial.println( utf8ascii("Høj bly gom vandt fræk sexquiz på wc"));
  Serial.println( utf8ascii("Любя, съешь щипцы, — вздохнёт мэр, — кайф жгуч."));
  Serial.println( utf8ascii("Γκόλφω, βάδιζε μπροστά ξανθή ψυχή!"));
}*/

int convertCtoF(int c) {
  return round(((float)(c/10) * 1.8 + 32) * 10);
}

int convertFtoC(int c) { 
  return round(((float)(c/10) - 32) * 5.55555);
}

float convertFtoC(float f) {
  return (f - 32) * 0.55555; 
}

float convertCtoF(float c) {
  return c * 1.8 + 32;
}
