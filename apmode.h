#pragma once
#include <WebServer.h>
extern WebServer server;
extern const char* const firmwareVersion;
void handleSave();
void handleRoot();
void handleFavicon();
void startAPMode();
void handleAPModeButton();