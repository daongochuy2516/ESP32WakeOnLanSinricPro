#pragma once
#include <Arduino.h>
extern bool apModeFlag;

extern String wolmode;
extern bool enablergb;
extern bool enablebuzzer;
extern int rainbowSpeed;
extern unsigned long beepDuration;
extern unsigned long rainbowTimer;
extern unsigned long buzzerTimer;
extern bool buzzerActive;
extern bool isWOLActive;

extern unsigned long powerOnDuration;
extern unsigned long powerOffTimer;
extern bool pendingOff;

extern unsigned long buttonPressStartTime;
extern bool buttonPressed;
extern const int LONG_PRESS_TIME;
