#include "boot_button.h"

#include <Arduino.h>

#include "config.h"

namespace {
bool debouncedPressed = false;
bool lastReading = true;
uint32_t lastDebounceMs = 0;
uint32_t pressStartMs = 0;
bool resetTriggered = false;
}  // namespace

namespace BootButton {

void init() {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    lastReading = digitalRead(BOOT_BUTTON_PIN);
    debouncedPressed = (lastReading == LOW);
    resetTriggered = false;
}

bool update() {
    if (resetTriggered) {
        return false;
    }

    bool reading = digitalRead(BOOT_BUTTON_PIN);
    uint32_t now = millis();

    if (reading != lastReading) {
        lastDebounceMs = now;
    }
    lastReading = reading;

    if ((now - lastDebounceMs) > BUTTON_DEBOUNCE_MS) {
        bool pressed = (reading == LOW);
        if (pressed && !debouncedPressed) {
            pressStartMs = now;
        }
        debouncedPressed = pressed;
    }

    if (debouncedPressed && (now - pressStartMs) >= FACTORY_RESET_HOLD_MS) {
        resetTriggered = true;
        return true;
    }

    return false;
}

bool isHeld() { return debouncedPressed; }

}  // namespace BootButton
