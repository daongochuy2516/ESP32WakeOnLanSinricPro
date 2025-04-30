#define FIRMWARE_VERSION "3.6.2"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "HealthDiagnostics.h"
#include "ESP32OTAHelper.h"
#include "SemVer.h"
#include <Preferences.h>
#include <WebServer.h>

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


#define LED_R  27  // Red channel of RGB LED
#define LED_G  26  // Green channel of RGB LED
#define LED_B  25  // Blue channel of RGB LED
#define BUZZER 33  // Buzzer output
#define PHY_BTN 19 // Physical Wake Button input



HealthDiagnostics healthDiagnostics;
Preferences preferences;
// Uncomment the following line to enable serial debug output
// #define SINRICPRO_NOSSL // Uncomment if you have memory limitation issues.
//#define ENABLE_DEBUG

// No need to edit this section — values will be automatically loaded from EEPROM (Preferences)
#define WIFI_SSID         "Wi-Fi SSID"
#define WIFI_PASS         "Wi-Fi Password"
#define APP_KEY           "App Key from SinricPro"
#define APP_SECRET        "App Secret from SinricPro"
#define SWITCH_ID_1       "Device ID from SinricPro"
#define PCMAC             "pcmac to wake up"
#define RELAYPIN_1        2

#define BEEP_TIME 500 // buzzer time


// biến cho AP Mode và web server
bool apModeFlag = false;
WebServer server(80);

// led, buzzer
String wolmode;
bool enablergb;
bool enablebuzzer;

int rainbowSpeed = 20;
unsigned long beepDuration = BEEP_TIME;
WiFiUDP UDP;
WakeOnLan WOL(UDP);
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
#define LONG_PRESS_TIME 5000 // 5000 ms (5 giây)


const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device config</title>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 20px;
        }

        h2 {
            color: #333;
            text-align: center;
            margin-bottom: 20px;
        }

        form {
            width: 100%;
            max-width: 400px;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
        }

        input[type="text"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 20px;
            border: 1px solid #ccc;
            border-radius: 4px;
            font-size: 16px;
            box-sizing: border-box;
        }

        input[type="submit"] {
            width: 100%;
            padding: 12px;
            border: none;
            background-color: #4CAF50;
            color: white;
            font-size: 16px;
            border-radius: 4px;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: #45a049;
        }

        footer {
            margin-top: 20px;
            text-align: center;
            font-size: 0.8rem;
            color: #777;
        }

        footer p {
            margin: 5px 0;
        }
        @media (max-width: 600px) {
            body {
                padding: 15px;
            }
            h2 {
                font-size: 20px;
            }
            form {
                padding: 15px;
            }
            input[type="text"], input[type="submit"] {
                font-size: 14px;
            }
        }
            .toggle-group {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 16px;
        font-family: sans-serif;
    }

    .toggle-group label {
        font-weight: 600;
        font-size: 16px;
    }

    .switch {
        position: relative;
        display: inline-block;
        width: 46px;
        height: 24px;
    }

    .switch input {
        opacity: 0;
        width: 0;
        height: 0;
    }

    .slider {
        position: absolute;
        cursor: pointer;
        background-color: #ccc;
        border-radius: 34px;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        transition: 0.3s;
    }

    .slider:before {
        position: absolute;
        content: "";
        height: 18px;
        width: 18px;
        left: 3px;
        bottom: 3px;
        background-color: white;
        border-radius: 50%;
        transition: 0.3s;
    }

    .switch input:checked + .slider {
        background-color: #4CAF50;
    }

    .switch input:checked + .slider:before {
        transform: translateX(22px);
    }
        /* Style for form and inputs */
    .form-group {
    margin-bottom: 20px;
    display: flex;
    flex-direction: column;
    }

    label {
    font-weight: bold;
    margin-bottom: 8px;
    }

    .custom-select {
    padding: 10px;
    font-size: 14px;
    border-radius: 5px;
    border: 1px solid #ccc;
    outline: none;
    transition: border-color 0.3s ease;
    }

    .custom-select:focus {
    border-color: #4CAF50;
    }

    small.note {
    font-size: 12px;
    color: #888;
    margin-top: 5px;
    }

    .custom-select:hover {
    background-color: #f0f0f0;
    }

    </style>
</head>
<body>

    <h2>ESP32 - SinricPro WakeOnLan device config</h2>

    <form action="/save" method="POST">
        <label for="ssid">Wi-Fi name (ONLY 2.4GHz pls):</label>
        <input type="text" id="ssid" name="ssid" >

        <label for="password">Wi-Fi Password:</label>
        <input type="text" id="password" name="password" >

        <label for="appKey">APP KEY:</label>
        <input type="text" id="appKey" name="appKey" >

        <label for="appSecret">APP SECRET:</label>
        <input type="text" id="appSecret" name="appSecret" >

        <label for="deviceId">DEVICE ID:</label>
        <input type="text" id="deviceId" name="deviceId" >

        <label for="pcMac">PC MAC Address:</label>
        <input type="text" id="pcMac" name="pcMac" >

        <div class="form-group">
            <label for="wolMode">Wake-on-LAN Mode:</label>
            <select id="wolMode" name="wolMode" class="custom-select">
              <option value="both" %WOL_MODE_BOTH% >Allow both</option>
              <option value="sinric" %WOL_MODE_SINRIC% >SinricPro only</option>
              <option value="physical" %WOL_MODE_PHYSICAL% >Physical button only</option>
            </select>
            <small class="note">Choose how the PC can be turned on. <br>
                This option can only be changed on this page</small>
          </div>
           
          
        <div class="toggle-group">
            <label for="enableLed">Enable LED RGB Effect</label>
            <label class="switch">
              <input type="checkbox" id="enableLed" name="enableLed" %ENABLE_LED%>
              <span class="slider"></span>
            </label>
          </div>
          
          <div class="toggle-group">
            <label for="enableBuzzer">Enable Buzzer</label>
            <label class="switch">
              <input type="checkbox" id="enableBuzzer" name="enableBuzzer" %ENABLE_BUZZER% >
              <span class="slider"></span>
            </label>
          </div>
          
        <br>

        <input type="submit" value="Save">
    </form>

    <footer>
        <p>You must connect this device to your own SinricPro account for it to function properly.<br>
        Otherwise, the device will not work correctly!<br>
        Firmware Version: %FIRMWARE_VERSION%<br>
        Copyright © daongochuy2516
        </p>
    </footer>

</body>
</html>


)rawliteral"; 


const char* loadPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Restarting...</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }

    .container {
      background-color: #fff;
      padding: 30px 40px;
      border-radius: 12px;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
      text-align: center;
      max-width: 400px;
      width: 100%;
    }

    .success {
      font-size: 20px;
      font-weight: bold;
      color: #4CAF50;
      margin-bottom: 10px;
    }

    .message {
      font-size: 15px;
      color: #555;
      margin-bottom: 25px;
    }

    .progress-bar {
      position: relative;
      height: 12px;
      width: 100%;
      background-color: #e0e0e0;
      border-radius: 6px;
      overflow: hidden;
      box-shadow: inset 0 2px 5px rgba(0, 0, 0, 0.1);
    }

    .indeterminate {
      position: absolute;
      height: 100%;
      background-color: #4CAF50;
      animation: fillBar 3s forwards;
      border-radius: 6px;
    }

    @keyframes fillBar {
      0% {
        left: 0;
        width: 0;
      }
      100% {
        left: 0;
        width: 100%;
      }
    }

  </style>
</head>
<body>
  <div class="container">
    <div class="success">Success!</div>
    <div class="message">Settings have been saved on this device. <br>Please wait while the device restarts...</div>
    <div class="progress-bar">
      <div class="indeterminate"></div>
    </div>
  </div>
</body>
</html>


  )rawliteral"; 

String minifyHTML(String html) {
  html.replace("\n", "");
  html.replace("\r", "");
  html.replace("\t", "");
  while (html.indexOf("> <") != -1) {
    html.replace("> <", "><");
  }
  while (html.indexOf("  ") != -1) {
    html.replace("  ", " ");
  }
  int eqPos = html.indexOf(" =");
  while (eqPos != -1) {
    html.remove(eqPos, 1);
    eqPos = html.indexOf(" =", eqPos);
  }

  eqPos = html.indexOf("= ");
  while (eqPos != -1) {
    html.remove(eqPos + 1, 1);
    eqPos = html.indexOf("= ", eqPos);
  }
  html.trim();
  return html;
}


//------------------------- Các hàm hỗ trợ ----------------------------

// read
String readStringFromPrefs(const char* key, const String& defaultValue = "") {
  preferences.begin("config", true); // Chế độ chỉ đọc
  String value = preferences.getString(key, defaultValue);
  preferences.end();
  return value;
}

// write 
void saveStringToPrefs(const char* key, const String& str) {
  preferences.begin("config", false);
  preferences.putString(key, str);
  preferences.end();
}

void setAPMode(bool mode) {
  preferences.begin("config", false);
  preferences.putBool("apmode", mode);
  preferences.end();
}

bool getAPMode() {
  preferences.begin("config", true);
  bool mode = preferences.getBool("apmode", false);
  preferences.end();
  return mode;
}

String checkWOLMode() {
    String wolMode = readStringFromPrefs("wolMode", "both");
    return wolMode;
}

bool checkLEDRainbow() {
    String enableLed = readStringFromPrefs("enableLed", "on");
    return enableLed == "on";
}

bool checkBuzzer() {
    String enableBuzzer = readStringFromPrefs("enableBuzzer", "on");
    return enableBuzzer == "on"; 
}

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

void wolexec() {
    String pcMacStored = readStringFromPrefs("pcMac", "");
    const char* mac = (pcMacStored.length() > 0) ? pcMacStored.c_str() : PCMAC;
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
            wolexec();
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

// Hàm xử lý trang chủ của web AP
void handleRoot() {
    String htmlContent = htmlPage;
    htmlContent.replace("%FIRMWARE_VERSION%", FIRMWARE_VERSION);
    String wolMode = readStringFromPrefs("wolMode", "both");
    String enableLed = readStringFromPrefs("enableLed", "off");
    String enableBuzzer = readStringFromPrefs("enableBuzzer", "off");

    htmlContent.replace("%WOL_MODE_BOTH%", (wolMode == "both" ? "selected" : ""));
    htmlContent.replace("%WOL_MODE_SINRIC%", (wolMode == "sinric" ? "selected" : ""));
    htmlContent.replace("%WOL_MODE_PHYSICAL%", (wolMode == "physical" ? "selected" : ""));
    htmlContent.replace("%ENABLE_LED%", (enableLed == "on" ? "checked" : ""));
    htmlContent.replace("%ENABLE_BUZZER%", (enableBuzzer == "on" ? "checked" : ""));
    htmlContent = minifyHTML(htmlContent);
    server.send(200, "text/html", htmlContent);
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
            setsinricstate(true);
            wolexec();
            buzzerActive = true;
            buzzerTimer = millis();
            powerOffTimer = millis();
            pendingOff = true;
        }
    }
}
//------------------------- Các hàm cài đặt ban đầu ----------------------------
// Kết nối WiFi
void setupWiFi() {
    Serial.println("[WiFi]: Connecting");
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setHostname("ESP32-SinricPro");
    String wifiSSID = readStringFromPrefs("ssid", WIFI_SSID);
    String wifiPASS = readStringFromPrefs("pass", WIFI_PASS);
    Serial.println("Wi-Fi SSID: " + wifiSSID);
    Serial.println("Wi-Fi PASS: " + wifiPASS);

    WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
}
void setsinricstate(bool state){
    String switchIdStored = readStringFromPrefs("deviceId", SWITCH_ID_1);
    SinricProSwitch& mySwitch1 = SinricPro[switchIdStored];
    mySwitch1.sendPowerStateEvent(state);
}
// Hàm khởi tạo SinricPro
void setupSinricPro() {
    String appKeyStored = readStringFromPrefs("appKey", APP_KEY);
    String appSecretStored = readStringFromPrefs("appSecret", APP_SECRET);
    String switchIdStored = readStringFromPrefs("deviceId", SWITCH_ID_1);
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
    
    Serial.print("APP_KEY (EEPROM): ");
    Serial.println(appKeyStored);
    Serial.print("APP_SECRET (EEPROM): ");
    Serial.println(appSecretStored);
    Serial.print("DEVICE ID (EEPRROM): ");
    Serial.println(switchIdStored);

    if (appKeyStored.length() > 0 && appSecretStored.length() > 0) {
        Serial.println("Using Sinric cert from eeprom");
    } else {
        Serial.println("eeprom sinric error");
    }
    SinricPro.begin(
        appKeyStored.length() > 0 ? appKeyStored.c_str() : APP_KEY,
        appSecretStored.length() > 0 ? appSecretStored.c_str() : APP_SECRET
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
      setsinricstate(false);
      digitalWrite(RELAYPIN_1, LOW);
      pendingOff = false;
    }

}
