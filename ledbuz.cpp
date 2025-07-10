#include "ledbuz.h"
#include "var.h"
#include "pins.h"

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