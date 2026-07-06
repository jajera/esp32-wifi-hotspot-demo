#pragma once

#include "provisioning_types.h"

namespace LedStatus {
    bool init();
    void update(DeviceState state, bool provisioningAfterFailure);
    void flashWhite();
    bool isInitialized();
}
