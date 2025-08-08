#include "handleUsbWeb.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "prefs.h"
#include "setupwifi.h"

constexpr size_t USB_BUFFER_LIMIT = 512;
static String serialBuffer = "";
bool wifiPrevConnected = false;

void handleSaveUsb(const String &json) {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);

  if (err) {
    Serial.println("ERROR: Invalid JSON");
    return;
  }

  if (doc.containsKey("ssid")) saveStringToPrefs("ssid", doc["ssid"].as<String>());
  if (doc.containsKey("password")) saveStringToPrefs("pass", doc["password"].as<String>());
  if (doc.containsKey("appKey")) saveStringToPrefs("appKey", doc["appKey"].as<String>());
  if (doc.containsKey("appSecret")) saveStringToPrefs("appSecret", doc["appSecret"].as<String>());
  if (doc.containsKey("deviceId")) saveStringToPrefs("deviceId", doc["deviceId"].as<String>());
  if (doc.containsKey("pcMac")) saveStringToPrefs("pcMac", doc["pcMac"].as<String>());
  if (doc.containsKey("wolMode")) saveStringToPrefs("wolMode", doc["wolMode"].as<String>());

  if (doc["enableLed"]) {
    saveStringToPrefs("enableLed", "on");
  } else {
    saveStringToPrefs("enableLed", "off");
  }

  if (doc["enableBuzzer"]) {
    saveStringToPrefs("enableBuzzer", "on");
  } else {
    saveStringToPrefs("enableBuzzer", "off");
  }

  Serial.println("OK");
}

void handleUsbWeb() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      serialBuffer.trim();

      if (serialBuffer == "GET_CONFIG") {
        StaticJsonDocument<512> doc;

        doc["ssid"] = readStringFromPrefs("ssid");
        doc["password"] = readStringFromPrefs("pass");
        doc["appKey"] = readStringFromPrefs("appKey");
        doc["appSecret"] = readStringFromPrefs("appSecret");
        doc["deviceId"] = readStringFromPrefs("deviceId");
        doc["pcMac"] = readStringFromPrefs("pcMac", "");
        doc["wolMode"] = readStringFromPrefs("wolMode", "both");
        doc["enableLed"] = (readStringFromPrefs("enableLed", "off") == "on");
        doc["enableBuzzer"] = (readStringFromPrefs("enableBuzzer", "off") == "on");
        doc["firmwareVersion"] = firmwareVersion;

        serializeJson(doc, Serial);
        Serial.println();
      } else if (serialBuffer == "PING") {
        Serial.println("PONG");
      } else if (serialBuffer.startsWith("{") && serialBuffer.endsWith("}")) {
        handleSaveUsb(serialBuffer);
      } else if (serialBuffer == "RESTART") {
        ESP.restart();
      } else if (serialBuffer == "SCAN_WIFI") {
        wifiPrevConnected = (WiFi.status() == WL_CONNECTED);
        Serial.println("SCANNING");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(200);

        int n = WiFi.scanNetworks();
        if (n <= 0) {
          Serial.println("[]");
          if (wifiPrevConnected) setupWiFi();
          return;
        }

        String json = "[";
        for (int i = 0; i < n; i++) {
          json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
          json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
          json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
          if (i < n - 1) json += ",";
        }
        json += "]";
        Serial.println(json);
        delay(500);
        if (wifiPrevConnected) setupWiFi();
        return;
      }

      serialBuffer = "";
    } else {
      serialBuffer += c;
      if (serialBuffer.length() > USB_BUFFER_LIMIT) {
        serialBuffer = "";
        Serial.println("ERROR: Input too long");
      }
    }
  }
}
