#include "app_main.h"

#include <WiFi.h>

#include "config.h"

namespace {
bool appRunning = false;
uint32_t lastHeartbeatMs = 0;
}  // namespace

void startMainApp() {
    appRunning = true;
    lastHeartbeatMs = millis();
    Serial.println("[APP] Phase 1 started (AWS provisioning reserved for Phase 2)");
    Serial.printf("[APP] Heartbeat IP=%s uptime=%lums\n", WiFi.localIP().toString().c_str(),
                  static_cast<unsigned long>(millis()));
}

void loopMainApp() {
    if (!appRunning) {
        return;
    }

    uint32_t now = millis();
    if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatMs = now;
        Serial.printf("[APP] Heartbeat IP=%s uptime=%lums\n", WiFi.localIP().toString().c_str(),
                      static_cast<unsigned long>(now));
    }
}
