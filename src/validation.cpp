#include "validation.h"

CredentialValidationResult validateCredentials(const String& ssid, const String& password) {
    if (ssid.length() == 0) {
        return {false, "ssid_empty"};
    }
    if (ssid.length() > 32) {
        return {false, "ssid_too_long"};
    }
    if (password.length() > 64) {
        return {false, "password_too_long"};
    }
    return {true, "ok"};
}
