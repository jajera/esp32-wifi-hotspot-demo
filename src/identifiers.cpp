#include "identifiers.h"

#include <cstdio>

namespace {
String macSuffixHex(const uint8_t mac[6]) {
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
    return String(buf);
}
}  // namespace

String macToSetupApSsid(const uint8_t mac[6]) {
    return String("ESP-Setup-") + macSuffixHex(mac);
}

String macToBleServiceName(const uint8_t mac[6]) {
    return String("PROV_") + macSuffixHex(mac);
}

void formatMacAddress(const uint8_t mac[6], char* out, size_t outLen) {
    snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5]);
}
