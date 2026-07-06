#pragma once

#include <Arduino.h>

#include "provisioning_types.h"

const char* transportName();
const char* deviceStateName(DeviceState state);

void logBootBanner(const uint8_t mac[6], bool wifiConfigured);
void logStateTransition(DeviceState from, DeviceState to, uint32_t uptimeMs);
void logSoftApStart(const String& ssid, const IPAddress& gateway);
void logBleStart(const String& serviceName);
void logCredentialResult(bool success, const char* reason, size_t ssidLength);
void logWifiConnected(const IPAddress& ip, int32_t rssi);
void logWifiFailure(const char* context, int reason, uint32_t timeoutMs);

#ifdef UNIT_TEST
String testFormatBootBanner(const uint8_t mac[6], bool wifiConfigured);
String testFormatStateTransition(DeviceState from, DeviceState to, uint32_t uptimeMs);
String testFormatCredentialLog(bool success, const char* reason, size_t ssidLength);
#endif
