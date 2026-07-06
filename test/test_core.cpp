#include <unity.h>

#include "identifiers.h"
#include "state_logic.h"
#include "validation.h"

// Feature: esp32-wifi-provisioning, Property 4: MAC-to-identifier derivation
void test_mac_derivation_known() {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    TEST_ASSERT_EQUAL_STRING("ESP-Setup-EEFF", macToSetupApSsid(mac).c_str());
    TEST_ASSERT_EQUAL_STRING("PROV_EEFF", macToBleServiceName(mac).c_str());
}

// Feature: esp32-wifi-provisioning, Property 5: Credential validation
void test_credential_validation() {
    auto ok = validateCredentials("MyNetwork", "secret");
    TEST_ASSERT_TRUE(ok.valid);

    auto empty = validateCredentials("", "secret");
    TEST_ASSERT_FALSE(empty.valid);

    String longSsid(33, 'a');
    auto longName = validateCredentials(longSsid, "");
    TEST_ASSERT_FALSE(longName.valid);

    String longPass(65, 'p');
    auto longPassword = validateCredentials("net", longPass);
    TEST_ASSERT_FALSE(longPassword.valid);
}

// Feature: esp32-wifi-provisioning, Property 1: Boot decision correctness
void test_boot_decision() {
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceState::STA_CONNECTING),
                      static_cast<int>(bootDecision({true, true})));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceState::PROVISIONING),
                      static_cast<int>(bootDecision({false, false})));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceState::PROVISIONING),
                      static_cast<int>(bootDecision({true, false})));
}

// Feature: esp32-wifi-provisioning, Property 2: State machine transitions
void test_state_transitions() {
    auto timeout =
        transitionState(DeviceState::STA_CONNECTING, Event::WIFI_TIMEOUT, false);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceState::PROVISIONING), static_cast<int>(timeout.nextState));
    TEST_ASSERT_TRUE(timeout.provisioningAfterFailure);

    auto connected =
        transitionState(DeviceState::STA_CONNECTING, Event::WIFI_CONNECTED, false);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceState::PRODUCTION), static_cast<int>(connected.nextState));
    TEST_ASSERT_TRUE(connected.callStartMainApp);
}

// Feature: esp32-wifi-provisioning, Property 9: LED state mapping
void test_led_mapping() {
    auto blue = ledForState(DeviceState::PROVISIONING, false);
    TEST_ASSERT_EQUAL_UINT8(0, blue.r);
    TEST_ASSERT_EQUAL_UINT8(0, blue.g);
    TEST_ASSERT_EQUAL_UINT8(255, blue.b);
    TEST_ASSERT_TRUE(blue.blink);

    auto red = ledForState(DeviceState::PROVISIONING, true);
    TEST_ASSERT_EQUAL_UINT8(255, red.r);
    TEST_ASSERT_TRUE(red.blink);

    auto green = ledForState(DeviceState::PRODUCTION, false);
    TEST_ASSERT_EQUAL_UINT8(0, green.r);
    TEST_ASSERT_EQUAL_UINT8(255, green.g);
    TEST_ASSERT_FALSE(green.blink);
}

// Feature: esp32-wifi-provisioning, Property 8: Button hold threshold
void test_factory_reset_threshold() {
    TEST_ASSERT_FALSE(shouldFactoryReset(1000));
    TEST_ASSERT_FALSE(shouldFactoryReset(4999));
    TEST_ASSERT_TRUE(shouldFactoryReset(5000));
    TEST_ASSERT_TRUE(shouldFactoryReset(8000));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mac_derivation_known);
    RUN_TEST(test_credential_validation);
    RUN_TEST(test_boot_decision);
    RUN_TEST(test_state_transitions);
    RUN_TEST(test_led_mapping);
    RUN_TEST(test_factory_reset_threshold);
    return UNITY_END();
}
