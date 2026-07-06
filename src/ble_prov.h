#pragma once

#include "nvs_store.h"

#ifdef BLE_TRANSPORT
namespace BleProv {
    void start(const String& serviceName, const String& pop);
    void stop();
    void loop();
    bool hasNewCredentials();
    WiFiCredentials getCredentials();
}
#endif
