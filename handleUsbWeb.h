#pragma once
#include <Arduino.h>
extern const char* const firmwareVersion;
void handleUsbWeb();
void handleSaveUsb(const String &json);