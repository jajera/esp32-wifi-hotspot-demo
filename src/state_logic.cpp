#include "state_logic.h"

#include <Arduino.h>

#include "config.h"
#include "provisioning_types.h"

DeviceState bootDecision(const BootDecisionInput& input) {
    if (input.wifiConfigured && input.hasSsid) {
        return DeviceState::STA_CONNECTING;
    }
    return DeviceState::PROVISIONING;
}

StateTransitionResult transitionState(DeviceState current, Event event, bool provisioningAfterFailure) {
    StateTransitionResult result{current, provisioningAfterFailure, false};

    switch (current) {
        case DeviceState::BOOT_DECISION:
            if (event == Event::CREDS_PRESENT) {
                result.nextState = DeviceState::STA_CONNECTING;
            } else if (event == Event::CREDS_MISSING) {
                result.nextState = DeviceState::PROVISIONING;
                result.provisioningAfterFailure = false;
            }
            break;

        case DeviceState::PROVISIONING:
            if (event == Event::CREDENTIALS_RECEIVED) {
                result.nextState = DeviceState::STA_CONNECTING;
                result.provisioningAfterFailure = false;
            } else if (event == Event::BUTTON_HELD) {
                result.nextState = DeviceState::FACTORY_RESET;
            }
            break;

        case DeviceState::STA_CONNECTING:
            if (event == Event::WIFI_CONNECTED) {
                result.nextState = DeviceState::PRODUCTION;
                result.callStartMainApp = true;
            } else if (event == Event::WIFI_TIMEOUT) {
                result.nextState = DeviceState::PROVISIONING;
                result.provisioningAfterFailure = true;
            } else if (event == Event::BUTTON_HELD) {
                result.nextState = DeviceState::FACTORY_RESET;
            }
            break;

        case DeviceState::PRODUCTION:
            if (event == Event::WIFI_DISCONNECT) {
                result.nextState = DeviceState::RECONNECTING;
            } else if (event == Event::BUTTON_HELD) {
                result.nextState = DeviceState::FACTORY_RESET;
            }
            break;

        case DeviceState::RECONNECTING:
            if (event == Event::WIFI_CONNECTED) {
                result.nextState = DeviceState::PRODUCTION;
                result.callStartMainApp = true;
            } else if (event == Event::RECONNECT_TIMEOUT) {
                result.nextState = DeviceState::PROVISIONING;
                result.provisioningAfterFailure = true;
            } else if (event == Event::BUTTON_HELD) {
                result.nextState = DeviceState::FACTORY_RESET;
            }
            break;

        case DeviceState::FACTORY_RESET:
            break;
    }

    return result;
}

#ifdef UNIT_TEST
#include "config.h"

LedOutput ledForState(DeviceState state, bool provisioningAfterFailure) {
    LedOutput out{0, 0, 0, false};

    switch (state) {
        case DeviceState::PROVISIONING:
            if (provisioningAfterFailure) {
                out = {255, 0, 0, true};
            } else {
                out = {0, 0, 255, true};
            }
            break;
        case DeviceState::STA_CONNECTING:
        case DeviceState::RECONNECTING:
            out = {255, 200, 0, true};
            break;
        case DeviceState::PRODUCTION:
            out = {0, 255, 0, false};
            break;
        case DeviceState::FACTORY_RESET:
            out = {255, 255, 255, false};
            break;
        default:
            break;
    }

    return out;
}

bool shouldFactoryReset(uint32_t debouncedHoldMs) {
    return debouncedHoldMs >= FACTORY_RESET_HOLD_MS;
}
#endif
