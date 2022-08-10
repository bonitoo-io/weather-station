#include <ESPWiFi.h>
#include "Tools.h"
#include "WeatherStation.h"

// English strings
static const char DAYS_ENG[] PROGMEM = "Sun\0" "Mon\0" "Tue\0" "Wed\0" "Thu\0" "Fri\0" "Sat\0";
static const char MONTHS_ENG[] PROGMEM = "Jan\0" "Feb\0" "Mar\0" "Apr\0" "May\0" "Jun\0" "Jul\0" "Aug\0" "Sep\0" "Oct\0" "Nov\0" "Dec\0";
static const char MOON_ENG[] PROGMEM = "new moon\0" "waxing crescent\0" "first quarter\0" "waxing gibbous\0" "full moon\0" "waning gibbous\0" "third quarter\0" "waning crescent\0";
static const char STR_ENG[] PROGMEM =
"Connecting WiFi\0" "Searching...\0" "or wait for setup\0" "Connecting IoT Center\0" "Waiting for restart\0" "Detecting location\0" "Checking update\0" "Updating time\0" "Updating weather\0" "Calculate moon phase\0" "Updating forecasts\0" "Connecting InfluxDB\0" "Sending status\0" "Done\0"
"Found update \0" "Starting update\0" "Update successful\0" "Restarting...\0" "Update failed \0" "Updating \0" "Restarting\0" "connect the following WiFi\nvia your phone or laptop\0" "follow the WiFi config steps\nor open browser with\0" "DEVICE CONFIGURATION:\0"
"Weather Station\0" "Configure via:\0" "Forecast error\0"
"In:\0" " Out:\0" "now\0" "Temperature sensor error!\0"
"INDOOR\0" "feel: \0" "hum\0" "wind\0"
"Moon\0" "Sun\0"
"Indoor spread risk\0" "Low\0" "Medium\0" "High!\0";

// Czech strings
static const char DAYS_CZ[] PROGMEM = "Ne\0" "Po\0" "Út\0" "St\0" "Čt\0" "Pá\0" "So\0";
static const char MONTHS_CZ[] PROGMEM = "Leden\0" "Únor\0" "Březen\0" "Duben\0" "Květen\0" "Červen\0" "Červenec\0" "Srpen\0" "Září\0" "Říjen\0" "Listopad\0" "Prosinec\0";
static const char MOON_CZ[] PROGMEM = "nov\0" "dorůstající srpek\0" "první čtvrť\0" "dorůstající měsíc\0" "úplněk\0" "couvající měsíc\0" "poslední čtvrť\0" "ubývající srpek\0";
static const char STR_CZ[] PROGMEM =
"Připojuji WiFi\0" "Hledám...\0" "počkejte prosím\0" "Připojuji IoT Center\0" "Čekám na restart\0" "Zjišťuji polohu\0" "Kontrola aktualizace\0" "Aktualizuji čas\0" "Aktualizuji počasí\0" "Vypočítávám fázi měsíce\0" "Aktualizuji předpověď\0" "Připojuji InfluxDB\0" "Zapisuji stav\0" "Hotovo\0"
"Nová aktualizace \0" "Připravuji aktualizaci\0" "Aktualizace byla úpěšná\0" "Restartuji...\0" "Aktualizace selhala \0" "Aktualizuji \0" "Restartuji...\0" "přes telefon, nebo počítač\npřipojte následující WiFi\0" "dokončete WiFi připojení\nnebo otevřete prohlížeč\0" "NASTAVENÍ:\0"
"Meteorologická stanice\0" "Nastavte přes:\0" "Chyba předpovědi\0"
"Zde:\0" " Vně:\0" "teď\0" "Chyba senzoru teploty!\0"
"DOMA\0" "pocitově: \0" "vlhkost\0" "vítr\0"
"Měsíc\0" "Slunce\0"
"Riziko šíření uvnitř\0" "Nízká\0" "Střední\0" "Vysoká!\0";

// Add other languages
// ...

struct tLanguage {
  const char* days;
  const char* months;
  const char* moon;
  const char* str;
};

const tLanguage languages[] PROGMEM = {
  {DAYS_ENG, MONTHS_ENG, MOON_ENG, STR_ENG}, //0 - en (default)
  {DAYS_CZ, MONTHS_CZ, MOON_CZ, STR_CZ}     //1 - cs
// Add other languages
};

const tLanguage* pLang = languages;
String deviceID;

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
  if ( strcmp( lang, "cz") == 0) {
    pLang = &languages[1];
  } else {
    pLang = &languages[0];
  }

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
    sprintf_P(buf, PSTR("%2d:%02d"), station.getRegionalSettings()->use24Hours ? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min);
  else
    sprintf_P(buf, PSTR("%02d:%02d:%02d"), station.getRegionalSettings()->use24Hours? timeInfo->tm_hour : (timeInfo->tm_hour+11)%12+1, timeInfo->tm_min, timeInfo->tm_sec);
  return String(buf);
}

String strTimeSuffix(time_t timestamp) {
  struct tm *timeInfo = localtime(&timestamp);
  return station.getRegionalSettings()->use24Hours ? "" : String(timeInfo->tm_hour>=12?F("pm"):F("am"));
}

String strDate(time_t timestamp, bool shortDate) {
  struct tm* timeInfo = localtime(&timestamp);
  char buff[25];

  if (station.getRegionalSettings()->useYMDFormat) {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %04d/%2d/%2d"), getDayName( timeInfo->tm_wday).c_str(), timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
    else
      sprintf_P(buff, PSTR("%s, %s\n%04d/%02d/%02d"), getDayName( timeInfo->tm_wday).c_str(), getMonthName( timeInfo->tm_mon).c_str(), timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
  } else {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %2d.%2d.%04d"), getDayName( timeInfo->tm_wday).c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
    else
      sprintf_P(buff, PSTR("%s, %s\n%02d.%02d.%04d"), getDayName( timeInfo->tm_wday).c_str(), getMonthName( timeInfo->tm_mon).c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  }
  return String(buff);
}

String strWind( unsigned int w) {
  return String(w) + String(station.getRegionalSettings()->useMetricUnits? F("m/s") : F("mph"));
}

const char *getDeviceID() {
  if(!deviceID.length()) {
    //Generate Device ID
    deviceID = "WS-" + WiFi.macAddress();
    for (uint8_t i = 17; i >= 5; i -= 3)
      deviceID.remove(i, 1); //remove MAC separators
  }
  return deviceID.c_str();
}

// Convert UTF8-string to extended ASCII
const char sTranslitFrom[] PROGMEM = "ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿřťčůňúďěŘŤČŮŇÚĎĚżąłśćęźńığışŻĄŁŚĆĘŹŃĞŞбвгдежзийклмнпрстуфхцчшщъьюяыэёоаБВГДЕЖЗИЙКЛМНПРСТУФХЦЧШЩЪЬЮЯЫЭЁОАΑαΒβΓγΔδΕεΖζΗηΘθΙιΚκΜμΝνΞξΟοΠπΡρΣσςΤτΥυΦφΧχΨψΩωόλάήΌΛΆΉ";
const char sTranslitTo[] PROGMEM   = "SOZsozYYuAAAAAAACEEEEIIIIDNOOOOOOUUUUYsaaaaaaaceeeeiiiionoooooouuuuyyrtcunudeRTCUNUDEzalsceznigisZALSCEZNGSbvgdejziyklmnprstufhccsswxyyweyoaBVGDEJZIYKLMNPRSTUFHCCSSWXYYWEYOAAaVvGgDdEeZzIiTtIiKkMmNnXxOoPpRrSssTtIiPpKkPpOoolaiOLAI";

char replaceUtf8Char( char c1, char c2) {
  //Replace 2-bytes UTF8 characters
  for (unsigned int i=0; i < sizeof(sTranslitFrom) - 1; i+=2) { //try to find character in the translation table
    if ((c1 == pgm_read_byte( sTranslitFrom + i)) && (c2 == pgm_read_byte( sTranslitFrom + i + 1)))
      return pgm_read_byte( sTranslitTo+(i/2));
  }
  //Serial.println( String(c1) + String(c2) + " -> ? " + String(c1,HEX) + " " + String(c2, HEX));
  return 0;
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(const String s) {
  String r;
  r.reserve(s.length());  //reduce reallocations
  char first = 0;
  for (uint8_t i=0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (first != 0) {  //already utf8 char?
      char c1 = replaceUtf8Char( first, c);
      if (c1 != 0) //found in the table?
        c = c1;
      else
        switch (first) {     // conversion depending on first UTF8-character
          case 0xC2:
            break; //keep char as is it
          case 0xC3:
            c = (char)c | 0xC0;
            break;
          case 0x82:
            if(c == 0xAC)
              c = (char)0x80; // special case Euro-symbol
            else
              c = '?';
            break;
          default:
            c = '?';
        }
      r += c;
      first = 0; //clear first symbol
      continue;
    }
    if (( c & 0b11100000) == 0b11000000) {  //2-byte utf8 char?
      first = c; //save first character
      continue;
    }
    if (c >= 128) //overwrite char above 128
      c = '?';
    r += c;
  }
  return r;
}

/*
/////////////////TEST////////////////////
void dump( const String& s) {
  Serial.print( "String: `" + s + "` -> ");
  for (uint8_t i=0; i < s.length(); i++)
    Serial.print( String((uint8_t)s.charAt(i), HEX) + " ");
  Serial.println();
}

void testutf8() {
  Serial.println( sTranslitFrom);
  Serial.println( sTranslitTo);
  Serial.println("utf8 translit table size " + String(sizeof(sTranslitFrom) - 1) + " vs " + String(sizeof(sTranslitTo) - 1)); //remove null character at the end of the string

  for (uint16_t i = 0; i < sizeof(sTranslitFrom) - 1; i += 2) {
    Serial.print( String( i) + " " + String(i/2) + " ");
    if (( String(sTranslitFrom).charAt(i) & 0b11100000) == 0b11000000) {
      Serial.print( String(String(sTranslitFrom).charAt(i)) + String(String(sTranslitFrom).charAt(i+1)) + " ");
      Serial.print( String(sTranslitFrom).charAt(i), HEX);
      Serial.print( " ");
      Serial.print( String(sTranslitFrom).charAt(i+1), HEX);
      Serial.print( " -> ");
      Serial.println( String(sTranslitTo).charAt(i/2));
    }
  }

  if ((sizeof(sTranslitFrom) - 1) != ((sizeof(sTranslitTo) - 1)*2))
    Serial.println("ERROR - utf8 translit table size " + String(sizeof(sTranslitFrom) - 1) + " vs " + String(sizeof(sTranslitTo) - 1)); //remove null character at the end of the string

  Serial.println( utf8ascii(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"));
  Serial.println( utf8ascii("Příliš žluťoučký kůň úpěl ďábelské ódy."));
  Serial.println( utf8ascii("PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY."));
  Serial.println( utf8ascii("Vogt Nyx: \"Büß du ja zwölf Qirsch, Kämpe"));
  Serial.println( utf8ascii("Pranzo d'acqua fa volti sghembi"));
  Serial.println( utf8ascii("Stróż pchnął kość w quiz gędźb vel fax myjń"));
  Serial.println( utf8ascii("Benjamín pidió una bebida de kiwi y fresa. Noé, sin vergüenza, la más exquisita champaña del menú "));

  Serial.println( utf8ascii("Pijamalı hasta yağız şoföre çabucak güvendi"));
  Serial.println( utf8ascii("Høj bly gom vandt fræk sexquiz på wc"));
  Serial.println( utf8ascii("Любя, съешь щипцы, - вздохнёт мэр, - кайф жгуч."));
  Serial.println( utf8ascii("Γκόλφω, βάδιζε μπροστά ξανθή ψυχή!"));
  dump("—");
  dump("„");
}

//  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
// Prilis zlutoucky kun upel dabelske ody.
// PRILIS ZLUTOUCKY KUN UPEL DABELSKE ODY.
// Vogt Nyx: ???Bus du ja zwolf Qirsch, Kampe
// Pranzo d'acqua fa volti sghembi
// Stroz pchnal kosc w quiz gedzb vel fax myjn
// Benjamin pidio una bebida de kiwi y fresa. Noe, sin verguenza, la mas exquisita champana del menu
// Pijamali hasta yagiz sofore cabucak guvendi
// Hoj bly gom vandt frak sexquiz pa wc
// Lyby, swesx sipcw, ??? vzdohnyt mer, ??? kayf jguc.
// Gkolpo, vadize mprosta xanti piki!
*/