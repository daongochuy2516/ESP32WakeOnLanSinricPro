#include <Arduino.h>
#include "button.h"
#include "prefs.h"
#include "pins.h"
#include "wol.h"
#include "var.h"

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
            startWakeSequence(true);
        }
    }
}
