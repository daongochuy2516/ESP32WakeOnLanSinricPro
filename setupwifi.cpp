#include "setupwifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include "prefs.h"
void setupWiFi() {
    Serial.println("[WiFi]: Connecting");
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setHostname("ESP32-SinricPro");
    String wifiSSID = readStringFromPrefs("ssid");
    String wifiPASS = readStringFromPrefs("pass");
    // Serial.println("Wi-Fi SSID: " + wifiSSID);
    // Serial.println("Wi-Fi PASS: " + wifiPASS);

    WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
}