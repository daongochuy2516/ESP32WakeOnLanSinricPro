#include "prefs.h"

Preferences preferences;

String readStringFromPrefs(const char* key, const String& defaultValue) {
  preferences.begin("config", true); // readonly
  String value = preferences.getString(key, defaultValue);
  preferences.end();
  return value;
}

void saveStringToPrefs(const char* key, const String& str) {
  preferences.begin("config", false);
  preferences.putString(key, str);
  preferences.end();
}

void setAPMode(bool mode) {
  preferences.begin("config", false);
  preferences.putBool("apmode", mode);
  preferences.end();
}

bool getAPMode() {
  preferences.begin("config", true);
  bool mode = preferences.getBool("apmode", false);
  preferences.end();
  return mode;
}

String checkWOLMode() {
  String wolMode = readStringFromPrefs("wolMode", "both");
  return wolMode;
}

bool checkLEDRainbow() {
  String enableLed = readStringFromPrefs("enableLed", "on");
  return enableLed == "on";
}

bool checkBuzzer() {
  String enableBuzzer = readStringFromPrefs("enableBuzzer", "on");
  return enableBuzzer == "on"; 
}