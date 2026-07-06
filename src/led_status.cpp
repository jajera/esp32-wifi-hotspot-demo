#include "led_status.h"

#include <soc/soc_caps.h>

#include "config.h"
#include "provisioning_types.h"

#if SOC_RMT_SUPPORTED
#include <esp32-hal-rgb-led.h>
#else
#include "led_ws2812.h"
#endif

namespace {
bool initialized = false;
uint32_t lastToggleMs = 0;
bool blinkOn = false;

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized) {
        return;
    }
#if SOC_RMT_SUPPORTED
    neopixelWrite(LED_PIN, r, g, b);
#else
    ws2812SetPixel(LED_PIN, r, g, b);
#endif
}

void applyPattern(uint8_t r, uint8_t g, uint8_t b, bool shouldBlink) {
    if (!shouldBlink) {
        setColor(r, g, b);
        return;
    }

    uint32_t now = millis();
    if (now - lastToggleMs >= LED_BLINK_INTERVAL_MS) {
        lastToggleMs = now;
        blinkOn = !blinkOn;
        setColor(blinkOn ? r : 0, blinkOn ? g : 0, blinkOn ? b : 0);
    }
}
}  // namespace

namespace LedStatus {

bool init() {
    delay(10);
#if SOC_RMT_SUPPORTED
    neopixelWrite(LED_PIN, 0, 0, 0);
    initialized = true;
#else
    initialized = ws2812Init(LED_PIN);
    if (!initialized) {
        Serial.println("[LED] init failed, continuing without LED feedback");
    } else {
        ws2812SetPixel(LED_PIN, 0, 0, 0);
        Serial.println("[LED] using bitbang WS2812 driver (no RMT on ESP32-C61)");
    }
#endif
    return initialized;
}

void update(DeviceState state, bool provisioningAfterFailure) {
    if (!initialized) {
        return;
    }

    switch (state) {
        case DeviceState::PROVISIONING:
            if (provisioningAfterFailure) {
                applyPattern(255, 0, 0, true);
            } else {
                applyPattern(0, 0, 255, true);
            }
            break;
        case DeviceState::STA_CONNECTING:
        case DeviceState::RECONNECTING:
            applyPattern(255, 200, 0, true);
            break;
        case DeviceState::PRODUCTION:
            applyPattern(0, 255, 0, false);
            break;
        default:
            break;
    }
}

void flashWhite() {
    if (!initialized) {
        return;
    }
    setColor(255, 255, 255);
    delay(200);
    setColor(0, 0, 0);
}

bool isInitialized() { return initialized; }

}  // namespace LedStatus
