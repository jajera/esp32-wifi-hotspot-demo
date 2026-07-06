#pragma once

#include "provisioning_types.h"

void startMainApp();

void initProvisioning();
void loopProvisioning();
DeviceState getCurrentState();
bool isProvisioningAfterFailure();
