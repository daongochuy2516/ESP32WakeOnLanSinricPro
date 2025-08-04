#include "handleUsbWeb.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "prefs.h"

constexpr size_t USB_BUFFER_LIMIT = 512;
static String serialBuffer = "";

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
