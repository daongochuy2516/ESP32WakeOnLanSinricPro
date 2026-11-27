// SinricPro.h defines the global SinricPro object, so it must only be
// included from this translation unit. Other modules go through sinric.h.
#include <Arduino.h>
#include "sinric.h"

// The SDK reports "firmwareVersion" to the server only if FIRMWARE_VERSION is
// defined before SinricPro.h. The real version lives at the top of main.cpp,
// so alias it to the exported firmwareVersion string (the SDK wraps it in String()).
#define FIRMWARE_VERSION firmwareVersion
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include "HealthDiagnostics.h"
#include "SemVer.h"
#include "ledbuz.h"
#include "prefs.h"
#include "pins.h"
#include "wol.h"
#include "var.h"
#include "ota.h"

static HealthDiagnostics healthDiagnostics;

static bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  Version currentVersion  = Version(firmwareVersion);
  Version newVersion      = Version(String(major) + "." + String(minor) + "." + String(patch));
  bool updateAvailable    = newVersion > currentVersion;

  Serial.print("URL: ");
  Serial.println(url.c_str());
  Serial.print("Current version: ");
  Serial.println(currentVersion.toString());
  Serial.print("New version: ");
  Serial.println(newVersion.toString());
  if (forceUpdate) Serial.println("Enforcing OTA update!");

  if (forceUpdate || updateAvailable) {
    if (updateAvailable) {
      Serial.println("Update available!");
    }
    setRGB(0, 0, 255);
    String result = startOtaUpdate(url);

    if (!result.isEmpty()) {
      SinricPro.setResponseMessage(std::move(result));
      setRGB(255, 0, 0);
      return false;
    }
    return true;
  } else {
    String result = "Current version is up to date.";
    Serial.println(result);
    SinricPro.setResponseMessage(std::move(result));
    return false;
  }
}

// Callback từ SinricPro (Switch)
static bool onPowerState1(const String &deviceId, bool &state) {
    Serial.printf("Device 1 turned %s\n", state ? "on" : "off");
    if (state){
        if (wolmode == "physical") {
          state = false;
          digitalWrite(RELAYPIN_1, LOW);
          SinricPro.setResponseMessage("Control access from SinricPro is blocked. Please switch to WebAP mode to change this setting.");
          return false;
        }
        Serial.println("Sending WOL Packet...");
        startWakeSequence(false);
    }
    return true;
}

// push state lên sinric
void setSinricState(bool state){
    String switchIdStored = readStringFromPrefs("deviceId");
    SinricProSwitch& mySwitch1 = SinricPro[switchIdStored];
    mySwitch1.sendPowerStateEvent(state);
}

// khởi tạo sinric
void setupSinricPro() {
    String appKeyStored = readStringFromPrefs("appKey");
    String appSecretStored = readStringFromPrefs("appSecret");
    String switchIdStored = readStringFromPrefs("deviceId");
    SinricProSwitch& mySwitch1 = SinricPro[switchIdStored];
    mySwitch1.onPowerState(onPowerState1);

    SinricPro.onConnected([]() {
        Serial.println("[SinricPro] Connected");
    });
    SinricPro.onDisconnected([]() {
        Serial.println("[SinricPro] Disconnected!");
    });
    SinricPro.onOTAUpdate(handleOTAUpdate);
    SinricPro.onReportHealth([](String &healthReport) {
        return healthDiagnostics.reportHealth(healthReport);
    });

    if (appKeyStored.length() > 0 && appSecretStored.length() > 0) {
        Serial.println("Using Sinric cert from eeprom");
    } else {
        Serial.println("eeprom sinric error");
    }
    SinricPro.begin(
        appKeyStored.c_str(),
        appSecretStored.c_str()
    );
}

void handleSinricPro() {
    SinricPro.handle();
}
