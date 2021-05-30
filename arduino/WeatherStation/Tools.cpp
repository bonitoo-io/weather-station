#include "Tools.h"

// Adjust according to your language
const char* const WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const MOON_PHASES[] = {"new moon", "waxing crescent", "first quarter", "waxing gibbous", "full moon", "waning gibbous", "third quarter", "waning crescent"};

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
  return conf.use24hour ? "" : (timeInfo->tm_hour>=12?"pm":"am");
}

String strDate(time_t timestamp, bool shortDate) {
  struct tm* timeInfo = localtime(&timestamp);
  char buff[20];
 
  if (conf.useYMDdate) {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %04d/%2d/%2d"), WDAY_NAMES[timeInfo->tm_wday], timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
    else  
      sprintf_P(buff, PSTR("%s, %s\n%04d/%02d/%02d"), WDAY_NAMES[timeInfo->tm_wday], MONTH_NAMES[timeInfo->tm_mon], timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday);
  } else {
    if (shortDate)
      sprintf_P(buff, PSTR("%s %2d.%2d.%04d"), WDAY_NAMES[timeInfo->tm_wday], timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
    else  
      sprintf_P(buff, PSTR("%s, %s\n%02d.%02d.%04d"), WDAY_NAMES[timeInfo->tm_wday], MONTH_NAMES[timeInfo->tm_mon], timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  }
  return String(buff);
}

String strTemp( float t) {
  return String(t, 0) + (conf.useMetric ? "°C" : "°F");
}

String strHum( float h) {
  return String(h, 0) + "%";
}

String strWind( float w) {
  return String(w, 0) + (conf.useMetric ? "m/s" : "mph");  
}

// Convert UTF8-string to extended ASCII

const String sTranslitFrom = "ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿřťčůňúďěŘŤČŮŇÚĎĚżąłśćęźńığışŻĄŁŚĆĘŹŃĞŞбвгдежзийклмнпрстуфхцчшщъьюяыэёоаБВГДЕЖЗИЙКЛМНПРСТУФХЦЧШЩЪЬЮЯЫЭЁОАΑαΒβΓγΔδΕεΖζΗηΘθΙιΚκΜμΝνΞξΟοΠπΡρΣσςΤτΥυΦφΧχΨψΩωόλάήΌΛΆΉ";
const String sTranslitTo   = "SOZsozYYuAAAAAAACEEEEIIIIDNOOOOOOUUUUYsaaaaaaaceeeeiiiionoooooouuuuyyrtcunudeRTCUNUDEzalsceznigisZALSCEZNGSbvgdejziyklmnprstufhccsswxyyweyoaBVGDEJZIYKLMNPRSTUFHCCSSWXYYWEYOAAaVvGgDdEeZzIiTtIiKkMmNnXxOoPpRrSssTtIiPpKkPpOoolaiOLAI";

char replaceChar( char c1, char c2) {
  //Replace 2-bytes UTF8 characters
  for (unsigned int i=0; i < sTranslitFrom.length() - 1; i+=2) { //try to find character in the translation table
    if ((c1 == sTranslitFrom.charAt(i)) && (c2 == sTranslitFrom.charAt(i+1)))
      return sTranslitTo.charAt(i/2);
  }
  Serial.println( String(c1) + String(c2) + " -> ?");
  return '?';
}

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
static uint8_t c1;  // Last character buffer
uint8_t utf8ascii(uint8_t ascii) {
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
  String r = "";
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
    c = utf8ascii( c);
    if (c != 0)
      r += c;
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
