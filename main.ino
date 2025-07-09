#define FIRMWARE_VERSION "3.7.5"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <Preferences.h>
#include <WebServer.h>

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "HealthDiagnostics.h"
#include "ESP32OTAHelper.h"
#include "htmlhelper.h"
#include "SemVer.h"
#include "prefs.h"
#include "pins.h"
#include "web.h"

/*
 * Project: ESP32 Wake-on-LAN Controller (SinricPro + Google Home / Alexa Integration)
 * Author : daongochuy2516
 * License: MIT License
 * 
 * Description:
 *  - Remotely power on your PC using ESP32 with Wake-on-LAN technology.
 *  - Seamlessly integrated with Google Home and Amazon Alexa via SinricPro.
 *  - No code modification needed for the default circuit; simply upload and enjoy!
 *  - Supports customizable hardware connections if needed.
 *  - Even if you are far away from your PC and on a different network, you can still turn it on as long as the ESP32 is on the same network as the PC,
 *    and the PC is connected via LAN with Wake-on-LAN configured properly.
 *  - The ESP32 runs completely wirelessly, except for the power supply.
 *
 * Hardware connections (default):
 *   - LED_RGB -> GPIO 27 (Red), GPIO 26 (Green), GPIO 25 (Blue) — common anode/cathode RGB LED (PWM control)
 *   - BUZZER  -> GPIO 33 (Buzzer output)
 *   - PHY_BTN -> GPIO 19 (Physical Wake Button input, INPUT_PULLUP)
 *
 * Notes:
 *  - Long-press the Physical Wake Button for more than 5 seconds to enter WebAP mode.
 *  - This will create an access point (AP) named "ESP32-SinricProWol" with the password "12345678".
 *  - After connecting to that Wi-Fi network, open a browser and go to http://192.168.4.1 to change settings.
 */



HealthDiagnostics healthDiagnostics;

// Uncomment the following line to enable serial debug output
//#define SINRICPRO_NOSSL // Uncomment if you have memory limitation issues.
//#define ENABLE_DEBUG

WiFiUDP UDP;
WakeOnLan WOL(UDP);
WebServer server(80);

// biến cho AP Mode và web server
bool apModeFlag = false;

// led, buzzer
String wolmode;
bool enablergb;
bool enablebuzzer;

int rainbowSpeed = 20;
unsigned long beepDuration = BEEP_TIME;

unsigned long rainbowTimer = 0;
unsigned long buzzerTimer = 0;
bool buzzerActive = false;
bool isWOLActive = false;

// cho xử lý tự động set status sinric về false
unsigned long powerOnDuration = 2000;
unsigned long powerOffTimer = 0;
bool pendingOff=false;

// cho xử lý nút 
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const int LONG_PRESS_TIME = 5000; // 5000 ms (5 giây)

//------------------------- Các hàm hỗ trợ ----------------------------


void controlBuzzer(bool state) {
    if (enablebuzzer) {
        digitalWrite(BUZZER, state ? HIGH : LOW);
    }
}

void setRGB(int r, int g, int b) {
    analogWrite(LED_R, r);
    analogWrite(LED_G, g);
    analogWrite(LED_B, b);
}

void blinkRed() {
    static unsigned long lastBlink = 0;
    static bool state = false;
    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        state = !state;
        setRGB(state ? 255 : 0, 0, 0);
    }
}

void rainbowEffect() {
    if (isWOLActive) return;
    if (!enablergb) {
      setRGB(0,0,0);
    };
    if (millis() - rainbowTimer > rainbowSpeed) {
        rainbowTimer = millis();
        float phase = (millis() % 3000) / 3000.0;
        int r = sin(phase * 2 * PI) * 127 + 128;
        int g = sin((phase + 0.33) * 2 * PI) * 127 + 128;
        int b = sin((phase + 0.66) * 2 * PI) * 127 + 128;
        setRGB(r, g, b);
    }
}

void wolExec() {
    String pcMacStored = readStringFromPrefs("pcMac", "");
    const char* mac = pcMacStored.c_str();
    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    WOL.sendMagicPacket(mac);
}

bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  Version currentVersion  = Version(FIRMWARE_VERSION);
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
    SinricPro.setResponseMessage(std::move(result));
    Serial.println(result);
    return false;
  }
}

// Callback từ SinricPro (Switch)
bool onPowerState1(const String &deviceId, bool &state) {
    Serial.printf("Device 1 turned %s\n", state ? "on" : "off");
    if (state){
        if (wolmode == "physical") {
          state = false;
          digitalWrite(RELAYPIN_1, LOW);
          SinricPro.setResponseMessage("Control access from SinricPro is blocked. Please switch to WebAP mode to change this setting.");
          return false;
        }
        if (wolmode != "physical") {
            digitalWrite(RELAYPIN_1, state ? HIGH : LOW);
            Serial.println("Sending WOL Packet...");
            isWOLActive = true;
            setRGB(255, 0, 0);
            controlBuzzer(true);
            wolExec();
            buzzerActive = true;
            buzzerTimer = millis();
            powerOffTimer = millis();
            pendingOff = true;
        }
    }
    return true;
}

//------------------------- Chế độ AP Mode ----------------------------

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
    htmlContent.replace("%FIRMWARE_VERSION%", FIRMWARE_VERSION);
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

//------------------------- Chế độ Non-AP (Normal) ----------------------------

// Hàm xử lý nút bấm khi ở chế độ bình thường
void processButtonPress() {
    bool btnState = digitalRead(PHY_BTN);
    
    if (btnState == LOW && !buttonPressed) {
        buttonPressStartTime = millis();
        buttonPressed = true;
    } 
    if (btnState == HIGH && buttonPressed) {
        unsigned long holdTime = millis() - buttonPressStartTime;
        Serial.println(holdTime);
        buttonPressed = false;

        if (holdTime >= LONG_PRESS_TIME) {
            Serial.println("Long press detected. Entering AP Mode...");
            setAPMode(true);
            ESP.restart();
        } else if (wolmode != "sinric"){
            Serial.println("Short press detected. Sending WOL...");
            digitalWrite(RELAYPIN_1, HIGH);
            isWOLActive = true;
            setRGB(255, 0, 0);
            controlBuzzer(true);
            setSinricState(true);
            wolExec();
            buzzerActive = true;
            buzzerTimer = millis();
            powerOffTimer = millis();
            pendingOff = true;
        }
    }
}
//------------------------- các hàm cài đặt ban đầu ----------------------------
// kết nối wifi
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
    SinricPro.onReportHealth([&](String &healthReport) {
        return healthDiagnostics.reportHealth(healthReport);
    });
    
    // Serial.print("APP_KEY (EEPROM): ");
    // Serial.println(appKeyStored);
    // Serial.print("APP_SECRET (EEPROM): ");
    // Serial.println(appSecretStored);
    // Serial.print("DEVICE ID (EEPRROM): ");
    // Serial.println(switchIdStored);

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

void setup() {
    Serial.begin(115200);
    pinMode(PHY_BTN, INPUT_PULLUP);
    pinMode(RELAYPIN_1, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    apModeFlag = getAPMode();
    if (apModeFlag) {
        startAPMode();
        return;
    }

    wolmode = checkWOLMode();
    enablergb = checkLEDRainbow();
    enablebuzzer = checkBuzzer();

    setupWiFi();
    setupSinricPro();
}

void loop() {
    if (apModeFlag) {
        server.handleClient();
        handleAPModeButton();
        return;
    }
    SinricPro.handle();
    processButtonPress();

    if (WiFi.status() == WL_CONNECTED) {
        static bool printed = false;
        if (!printed) {
            Serial.printf("\n[WiFi]: Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            setRGB(0, 255, 0);
            delay(500);
            setRGB(0, 0, 0);
            printed = true;
        }
        if (!enablergb)setRGB(0, 0, 0);
        if (!isWOLActive && enablergb) rainbowEffect();
        
    } else {
        blinkRed();
    }

    if (buzzerActive && millis() - buzzerTimer >= BEEP_TIME ) {
        digitalWrite(BUZZER, LOW);
        buzzerActive = false;
        isWOLActive = false;
        if(!enablergb) setRGB(0,0,0);
    }
    if (powerOffTimer != 0 && millis () - powerOffTimer >= powerOnDuration && pendingOff){
      setSinricState(false);
      digitalWrite(RELAYPIN_1, LOW);
      pendingOff = false;
    }

}
