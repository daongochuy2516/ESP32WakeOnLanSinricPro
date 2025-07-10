#include "var.h"
#include <Arduino.h>
// apmode and web
bool apModeFlag = false;

// led and buzzer
String wolmode;
bool enablergb;
bool enablebuzzer;
int rainbowSpeed = 20;
unsigned long beepDuration = 200;
unsigned long rainbowTimer = 0;
unsigned long buzzerTimer = 0;
bool buzzerActive = false;
bool isWOLActive = false;

// set sinric state
unsigned long powerOnDuration = 2000;
unsigned long powerOffTimer = 0;
bool pendingOff = false;

// button process
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const int LONG_PRESS_TIME = 5000;
