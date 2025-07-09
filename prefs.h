#pragma once
#include <Arduino.h>
#include <Preferences.h>

extern Preferences preferences;

String readStringFromPrefs(const char* key, const String& defaultValue = "");
void saveStringToPrefs(const char* key, const String& str);
void setAPMode(bool mode);
bool getAPMode();
String checkWOLMode();
bool checkLEDRainbow();
bool checkBuzzer();