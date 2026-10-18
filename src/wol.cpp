#include <WiFi.h>
#include <WakeOnLan.h>
#include "wol.h"
#include "prefs.h"
#include "ledbuz.h"
#include "sinric.h"
#include "pins.h"
#include "var.h"

WiFiUDP UDP;
WakeOnLan WOL(UDP);

void wolExec() {
    String pcMacStored = readStringFromPrefs("pcMac", "");
    const char* mac = pcMacStored.c_str();
    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    WOL.sendMagicPacket(mac);
}

void startWakeSequence(bool notifySinric) {
    digitalWrite(RELAYPIN_1, HIGH);
    isWOLActive = true;
    setRGB(255, 0, 0);
    controlBuzzer(true);
    if (notifySinric) setSinricState(true);
    wolExec();
    buzzerActive = true;
    buzzerTimer = millis();
    powerOffTimer = millis();
    pendingOff = true;
}

void handleWakeTimers() {
    if (buzzerActive && millis() - buzzerTimer >= beepDuration ) {
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
