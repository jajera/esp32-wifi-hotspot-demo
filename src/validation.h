#pragma once

#include <Arduino.h>

struct CredentialValidationResult {
    bool valid = false;
    const char* reason = "";
};

CredentialValidationResult validateCredentials(const String& ssid, const String& password);
