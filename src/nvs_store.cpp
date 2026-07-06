#include "nvs_store.h"

#include <Preferences.h>

namespace {
constexpr const char* kWifiNamespace = "wifi";
constexpr const char* kKeySsid = "wifi_ssid";
constexpr const char* kKeyPassword = "wifi_password";
constexpr const char* kKeyConfigured = "wifi_configured";

Preferences prefs;
bool prefsReady = false;
}  // namespace

namespace NvsStore {

bool init() {
    prefsReady = prefs.begin(kWifiNamespace, false);
    return prefsReady;
}

WiFiCredentials loadCredentials() {
    WiFiCredentials creds;
    if (!prefsReady) {
        return creds;
    }

    creds.configured = prefs.getBool(kKeyConfigured, false);
    creds.ssid = prefs.getString(kKeySsid, "");
    creds.password = prefs.getString(kKeyPassword, "");

    if (creds.configured && creds.ssid.length() == 0) {
        creds.configured = false;
    }

    return creds;
}

bool saveCredentials(const String& ssid, const String& password) {
    if (!prefsReady) {
        return false;
    }

    if (!prefs.putString(kKeySsid, ssid)) {
        return false;
    }
    if (!prefs.putString(kKeyPassword, password)) {
        return false;
    }
    if (!prefs.putBool(kKeyConfigured, true)) {
        return false;
    }

    return true;
}

void eraseWiFiCredentials() {
    if (!prefsReady) {
        return;
    }

    prefs.remove(kKeySsid);
    prefs.remove(kKeyPassword);
    prefs.putBool(kKeyConfigured, false);
}

bool isConfigured() {
    if (!prefsReady) {
        return false;
    }
    return prefs.getBool(kKeyConfigured, false);
}

}  // namespace NvsStore
