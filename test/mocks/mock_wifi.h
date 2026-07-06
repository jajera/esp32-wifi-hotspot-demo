#pragma once

#include <cstdint>

struct MockWifiState {
    bool connected = false;
    int disconnectReason = 0;
};

inline MockWifiState gMockWifi;
