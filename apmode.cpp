#include "apmode.h"
#include "prefs.h"
#include "htmlhelper.h"
#include "ledbuz.h"
#include "web.h"
#include "pins.h"
#include <WebServer.h>
#include <Arduino.h>
#include <WiFi.h>


// Hàm xử lý lưu cấu hình khi nhận POST từ web
void handleSave() {
    // Lấy các giá trị POST từ form
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String appKey = server.arg("appKey");
    String appSecret = server.arg("appSecret");
    String deviceId = server.arg("deviceId");
    String pcMac = server.arg("pcMac");
    String wolMode = server.arg("wolMode");

    String enableLed = server.arg("enableLed");
    String enableBuzzer = server.arg("enableBuzzer");

    if (wolMode.length() > 0) {
        saveStringToPrefs("wolMode", wolMode);
    }
    if (enableLed == "on") {
        saveStringToPrefs("enableLed", "on");
    } else {
        saveStringToPrefs("enableLed", "off");
    }
    if (enableBuzzer == "on") {
        saveStringToPrefs("enableBuzzer", "on");
    } else {
        saveStringToPrefs("enableBuzzer", "off");
    }

    if (ssid.length() > 0) {
        saveStringToPrefs("ssid", ssid);
    }
    if (password.length() > 0) {
        saveStringToPrefs("pass", password);
    }
    if (appKey.length() > 0) {
        saveStringToPrefs("appKey", appKey);
    }
    if (appSecret.length() > 0) {
        saveStringToPrefs("appSecret", appSecret);
    }
    if (deviceId.length() > 0) {
        saveStringToPrefs("deviceId", deviceId);
    }
    if (pcMac.length() > 0) {
        saveStringToPrefs("pcMac", pcMac);
    }
    String htmlSave = loadPage;
    htmlSave = minifyHTML(htmlSave);

    server.send(200, "text/html", htmlSave);

    setRGB(0, 255, 0);

    unsigned long startTime = millis();
    while (millis() - startTime < 500) {
      yield();
    }

    setAPMode(false);
    delay(2000);
    ESP.restart();
}

// Hàm xử lý trang chủ của web AP (SSR - Server Side Rendering)
void handleRoot() {
    String htmlContent = homePage;
    htmlContent.replace("%firmwareVersion%", firmwareVersion);
    String wolMode = readStringFromPrefs("wolMode", "both");
    String enableLed = readStringFromPrefs("enableLed", "off");
    String enableBuzzer = readStringFromPrefs("enableBuzzer", "off");
    String wifiSSID = readStringFromPrefs("ssid");
    String wifiPASS = readStringFromPrefs("pass");
    String appKeyStored = readStringFromPrefs("appKey");
    String appSecretStored = readStringFromPrefs("appSecret");
    String switchIdStored = readStringFromPrefs("deviceId");
    String pcMacStored = readStringFromPrefs("pcMac", "");

    htmlContent.replace("%WOL_MODE_BOTH%", (wolMode == "both" ? "selected" : ""));
    htmlContent.replace("%WOL_MODE_SINRIC%", (wolMode == "sinric" ? "selected" : ""));
    htmlContent.replace("%WOL_MODE_PHYSICAL%", (wolMode == "physical" ? "selected" : ""));
    htmlContent.replace("%ENABLE_LED%", (enableLed == "on" ? "checked" : ""));
    htmlContent.replace("%ENABLE_BUZZER%", (enableBuzzer == "on" ? "checked" : ""));
    htmlContent.replace("%WIFI%", (wifiSSID.length() > 0 ? wifiSSID : "WiFi Name"));
    htmlContent.replace("%WIFIPASS%", (wifiPASS.length() > 0 ? "●●●●●●●●" : "WiFi Password"));
    htmlContent.replace("%APPKEY%", (appKeyStored.length() > 0 ? "●●●●●●●●" : "App Key"));
    htmlContent.replace("%APPSECRET%", (appSecretStored.length() > 0 ? "●●●●●●●●" : "App Secret"));
    htmlContent.replace("%DEVICEID%", (switchIdStored.length() > 0 ? switchIdStored : "Device ID"));
    htmlContent.replace("%PCMAC%", (pcMacStored.length() > 0 ? pcMacStored : "00:1A:2B:3C:4D:5E"));

    htmlContent = minifyHTML(htmlContent);
    htmlContent = fun(htmlContent);
    server.send(200, "text/html", htmlContent);
}
void handleFavicon(){
    server.send(204);
}


void startAPMode() {
    Serial.println("Starting AP Mode...");
    setRGB(0, 0, 255);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-SinricProWol","12345678");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/favicon.ico", HTTP_GET, handleFavicon);
    server.begin();
    Serial.println("Web server started in AP Mode");
}

void handleAPModeButton() {
    if (digitalRead(PHY_BTN) == LOW) {
        Serial.println("AP Mode exit button pressed");
        setAPMode(false);
        ESP.restart();
    }
}