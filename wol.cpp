#include <WiFi.h>
#include "wol.h"
#include "prefs.h"

WiFiUDP UDP;
WakeOnLan WOL(UDP);

void wolExec() {
    String pcMacStored = readStringFromPrefs("pcMac", "");
    const char* mac = pcMacStored.c_str();
    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    WOL.sendMagicPacket(mac);
}
