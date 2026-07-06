#ifdef BLE_TRANSPORT

#include "ble_prov.h"

#include <WiFi.h>
#include <WiFiProv.h>

#include "logging.h"
#include "validation.h"

namespace {
WiFiCredentials pending;
bool running = false;
bool credsReady = false;

void onProvEvent(arduino_event_t* event) {
    if (event->event_id == ARDUINO_EVENT_PROV_CRED_RECV) {
        wifi_sta_config_t* cred = &event->event_info.prov_cred_recv;
        String ssid(reinterpret_cast<const char*>(cred->ssid));
        String password(reinterpret_cast<const char*>(cred->password));

        auto validation = validateCredentials(ssid, password);
        if (!validation.valid) {
            logCredentialResult(false, validation.reason, ssid.length());
            return;
        }

        pending.ssid = ssid;
        pending.password = password;
        pending.configured = true;
        credsReady = true;
        logCredentialResult(true, validation.reason, ssid.length());
        return;
    }

    if (event->event_id == ARDUINO_EVENT_PROV_CRED_SUCCESS) {
        WiFiProv.endProvision();
    }
}
}  // namespace

namespace BleProv {

void start(const String& serviceName, const String& pop) {
    WiFi.onEvent(onProvEvent);
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BLE,
                            NETWORK_PROV_SECURITY_1, pop.c_str(), serviceName.c_str(), nullptr,
                            nullptr, false);
    logBleStart(serviceName);
    pending = WiFiCredentials{};
    credsReady = false;
    running = true;
}

void stop() {
    if (!running) {
        return;
    }
    WiFiProv.endProvision();
    running = false;
}

void loop() {
    (void)running;
}

bool hasNewCredentials() { return credsReady; }

WiFiCredentials getCredentials() {
    WiFiCredentials creds = pending;
    credsReady = false;
    return creds;
}

}  // namespace BleProv

#endif
