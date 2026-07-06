#pragma once

#include <Arduino.h>

String macToSetupApSsid(const uint8_t mac[6]);
String macToBleServiceName(const uint8_t mac[6]);
void formatMacAddress(const uint8_t mac[6], char* out, size_t outLen);
