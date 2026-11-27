#pragma once

extern const char* const firmwareVersion;

void startAPMode();
// Webui main loop: serve HTTP clients and watch the exit button.
void handleAPMode();
