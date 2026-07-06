#pragma once

#include "provisioning_types.h"

DeviceState bootDecision(const BootDecisionInput& input);
StateTransitionResult transitionState(DeviceState current, Event event, bool provisioningAfterFailure);

#ifdef UNIT_TEST
struct LedOutput {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool blink;
};

LedOutput ledForState(DeviceState state, bool provisioningAfterFailure);
bool shouldFactoryReset(uint32_t debouncedHoldMs);
#endif
