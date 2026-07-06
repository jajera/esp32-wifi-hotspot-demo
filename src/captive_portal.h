#pragma once

#include <Arduino.h>

struct PortalCredentials {
    bool received = false;
    String ssid;
    String password;
};

#ifndef BLE_TRANSPORT
namespace CaptivePortal {
    void start(const String& apSsid);
    void stop();
    void loop();
    bool hasNewCredentials();
    PortalCredentials getCredentials();
    const char* portalHtml();
}
#endif
