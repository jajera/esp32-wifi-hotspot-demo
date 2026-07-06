#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "boot_button.h"
#include "config.h"
#include "led_status.h"
#include "logging.h"
#include "nvs_store.h"
#include "provisioning.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    if (!LedStatus::init()) {
        Serial.println("[LED] init failed, continuing without LED feedback");
    }

    BootButton::init();
    NvsStore::init();
    logBootBanner(mac, NvsStore::isConfigured());
    initProvisioning();
}

void loop() {
    loopProvisioning();
    LedStatus::update(getCurrentState(), isProvisioningAfterFailure());
    delay(10);
}
