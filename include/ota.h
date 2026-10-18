#pragma once
#include <Arduino.h>

// Download and flash firmware from url. Reboots on success,
// returns an error message on failure.
String startOtaUpdate(const String &url);
