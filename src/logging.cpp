#include "logging.h"

#include <cstdio>

#include "config.h"
#include "identifiers.h"

const char* transportName() {
#ifdef BLE_TRANSPORT
    return "ble";
#else
    return "softap";
#endif
}

const char* deviceStateName(DeviceState state) {
    switch (state) {
        case DeviceState::BOOT_DECISION:
            return "BOOT_DECISION";
        case DeviceState::PROVISIONING:
            return "PROVISIONING";
        case DeviceState::STA_CONNECTING:
            return "STA_CONNECTING";
        case DeviceState::PRODUCTION:
            return "PRODUCTION";
        case DeviceState::RECONNECTING:
            return "RECONNECTING";
        case DeviceState::FACTORY_RESET:
            return "FACTORY_RESET";
    }
    return "UNKNOWN";
}

void logBootBanner(const uint8_t mac[6], bool wifiConfigured) {
    char macStr[18];
    formatMacAddress(mac, macStr, sizeof(macStr));
    Serial.printf("[BOOT] FW=%s chip=ESP32-C61 mac=%s transport=%s wifi_configured=%s\n",
                  FIRMWARE_VERSION, macStr, transportName(), wifiConfigured ? "true" : "false");
}

void logStateTransition(DeviceState from, DeviceState to, uint32_t uptimeMs) {
    Serial.printf("[STATE] %s -> %s uptime=%lums\n", deviceStateName(from), deviceStateName(to),
                  static_cast<unsigned long>(uptimeMs));
}

void logSoftApStart(const String& ssid, const IPAddress& gateway) {
    Serial.printf("[SOFTAP] SSID=%s gateway=%s\n", ssid.c_str(), gateway.toString().c_str());
}

void logBleStart(const String& serviceName) {
    Serial.printf("[BLE] service=%s pop_required=true\n", serviceName.c_str());
}

void logCredentialResult(bool success, const char* reason, size_t ssidLength) {
    Serial.printf("[CREDS] %s reason=%s SSID length=%u\n", success ? "ok" : "fail", reason,
                  static_cast<unsigned>(ssidLength));
}

void logWifiConnected(const IPAddress& ip, int32_t rssi) {
    Serial.printf("[WIFI] Connected IP=%s RSSI=%ld\n", ip.toString().c_str(), static_cast<long>(rssi));
}

void logWifiFailure(const char* context, int reason, uint32_t timeoutMs) {
    Serial.printf("[WIFI] %s failed reason=%d timeout=%lums\n", context, reason,
                  static_cast<unsigned long>(timeoutMs));
}

#ifdef UNIT_TEST
String testFormatBootBanner(const uint8_t mac[6], bool wifiConfigured) {
    char macStr[18];
    formatMacAddress(mac, macStr, sizeof(macStr));
    char buf[256];
    snprintf(buf, sizeof(buf), "[BOOT] FW=%s chip=ESP32-C61 mac=%s transport=%s wifi_configured=%s",
             FIRMWARE_VERSION, macStr, transportName(), wifiConfigured ? "true" : "false");
    return String(buf);
}

String testFormatStateTransition(DeviceState from, DeviceState to, uint32_t uptimeMs) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[STATE] %s -> %s uptime=%ums", deviceStateName(from),
             deviceStateName(to), uptimeMs);
    return String(buf);
}

String testFormatCredentialLog(bool success, const char* reason, size_t ssidLength) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[CREDS] %s reason=%s SSID length=%u", success ? "ok" : "fail", reason,
             static_cast<unsigned>(ssidLength));
    return String(buf);
}
#endif
