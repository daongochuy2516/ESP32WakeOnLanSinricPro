#define FIRMWARE_VERSION "4.0.2"

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
 * Hardware connections (default, see include/pins.h):
 *   - LED_RGB -> GPIO 27 (Red), GPIO 26 (Green), GPIO 25 (Blue) — common anode/cathode RGB LED (PWM control)
 *   - BUZZER  -> GPIO 33 (Buzzer output)
 *   - PHY_BTN -> GPIO 19 (Physical Wake Button input, INPUT_PULLUP)
 *
 * Notes:
 *  - Long-press the Physical Wake Button for more than 5 seconds to enter WebAP mode.
 *  - This will create an access point (AP) named "ESP32-SinricProWol" with the password "12345678".
 *  - After connecting to that Wi-Fi network, open a browser and go to http://192.168.4.1 to change settings.
 *  - SinricPro debug / NOSSL options live in platformio.ini (build_flags).
 */

#include <Arduino.h>
#include <WiFi.h>

#include "handleUsbWeb.h"
#include "configinit.h"
#include "setupwifi.h"
#include "ledbuz.h"
#include "apmode.h"
#include "button.h"
#include "sinric.h"
#include "pins.h"
#include "wol.h"
#include "var.h"

const char* const firmwareVersion = FIRMWARE_VERSION; //make a copy so module can access it

static void handleWiFiStatus() {
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
}

void setup() {
    Serial.begin(115200);
    pinMode(PHY_BTN, INPUT_PULLUP);
    pinMode(RELAYPIN_1, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    initStartupConfig(); //this will load vars from prefs, include the apmode flag
    if (apModeFlag) { //if yes, start apmode and ignore below
        startAPMode();
        return;
    }
    setupWiFi();
    setupSinricPro();
}

void loop() {
    handleUsbWeb();
    if (apModeFlag) { //if yes, webui main loop
        handleAPMode();
        return;
    }
    handleSinricPro();
    processButtonPress();
    handleWiFiStatus();
    handleWakeTimers();
}
