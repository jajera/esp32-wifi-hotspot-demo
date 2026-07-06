#pragma once

#include <Arduino.h>

struct WiFiCredentials {
    String ssid;
    String password;
    bool configured = false;
};

namespace NvsStore {
    bool init();
    WiFiCredentials loadCredentials();
    bool saveCredentials(const String& ssid, const String& password);
    void eraseWiFiCredentials();
    bool isConfigured();
}
