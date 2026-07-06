#pragma once

#include <stdint.h>

enum class DeviceState : uint8_t {
    BOOT_DECISION = 0,
    PROVISIONING = 1,
    STA_CONNECTING = 2,
    PRODUCTION = 3,
    RECONNECTING = 4,
    FACTORY_RESET = 5
};

enum class Event : uint8_t {
    CREDS_PRESENT,
    CREDS_MISSING,
    CREDENTIALS_RECEIVED,
    WIFI_CONNECTED,
    WIFI_TIMEOUT,
    WIFI_DISCONNECT,
    RECONNECT_TIMEOUT,
    BUTTON_HELD
};

struct StateTransitionResult {
    DeviceState nextState;
    bool provisioningAfterFailure;
    bool callStartMainApp;
};

struct BootDecisionInput {
    bool wifiConfigured;
    bool hasSsid;
};

DeviceState bootDecision(const BootDecisionInput& input);
StateTransitionResult transitionState(DeviceState current, Event event, bool provisioningAfterFailure);

