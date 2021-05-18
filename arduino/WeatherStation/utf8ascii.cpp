#include <Arduino.h>

// Convert UTF8-string to extended ASCII using

static uint8_t c1;  // Last character buffer

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
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

  return  (0);                                     // otherwise: return zero, if character has to be ignored
}

const String sTranslitFrom = "ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿřťčůňúďěŘŤČŮŇÚĎĚżąłśćęźńığışŻĄŁŚĆĘŹŃĞŞбвгдежзийклмнпрстуфхцчшщъьюяыэёоаБВГДЕЖЗИЙКЛМНПРСТУФХЦЧШЩЪЬЮЯЫЭЁОАΑαΒβΓγΔδΕεΖζΗηΘθΙιΚκΜμΝνΞξΟοΠπΡρΣσςΤτΥυΦφΧχΨψΩωόλάήΌΛΆΉ";
const String sTranslitTo   = "SOZsozYYuAAAAAAACEEEEIIIIDNOOOOOOUUUUYsaaaaaaaceeeeiiiionoooooouuuuyyrtcunudeRTCUNUDEzalsceznigisZALSCEZNGSbvgdejziyklmnprstufhccsswxyyweyoaBVGDEJZIYKLMNPRSTUFHCCSSWXYYWEYOAAaVvGgDdEeZzIiTtIiKkMmNnXxOoPpRrSssTtIiPpKkPpOoolaiOLAI";

char replaceChar( char c1, char c2) {
  //Replace 2-bytes UTF8 characters
  for (int i=0; i < sTranslitFrom.length() - 1; i+=2) { //try to find character in the translation table
    if ((c1 == sTranslitFrom.charAt(i)) && (c2 == sTranslitFrom.charAt(i+1)))
      return sTranslitTo.charAt(i/2);
  }
  Serial.println( String(c1) + String(c2) + " -> ?");
  return '?';
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(const String s) {
  String r = "";
  char c;
  char c1 = 0;
  for (int i=0; i < s.length(); i++) {
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


void testutf8() {
  //Serial.println( sTranslitFrom);
  if (sTranslitFrom.length() != (sTranslitTo.length()*2))
    Serial.println("ERROR - utf8 translit table");

/*  Serial.println( sTranslitFrom.length());
  for (int i=0; i < sTranslitFrom.length() - 1; i+=2) {
    if (( sTranslitFrom.charAt(i) & 0b11100000) == 0b11000000) {
      Serial.print( String(sTranslitFrom.charAt(i)) + String(sTranslitFrom.charAt(i+1)) + " ");
      Serial.print( sTranslitFrom.charAt(i), BIN);
      Serial.print( " ");
      Serial.println( sTranslitFrom.charAt(i+1), BIN);
    }
  }
  Serial.println( sTranslitTo);
  Serial.println( sTranslitTo.length()*2);*/

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
}
