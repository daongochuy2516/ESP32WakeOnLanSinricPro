#include "configinit.h"
#include "prefs.h"
#include "var.h"
//#include <Arduino.h>

void initStartupConfig() {
    apModeFlag   = getAPMode();
    wolmode      = checkWOLMode();
    enablergb    = checkLEDRainbow();
    enablebuzzer = checkBuzzer();
}
