#include "provisioning.h"

#include <WiFi.h>
#include <esp_mac.h>

#include "app_main.h"
#include "boot_button.h"
#include "config.h"
#include "identifiers.h"
#include "led_status.h"
#include "logging.h"
#include "nvs_store.h"
#include "state_logic.h"

#ifndef BLE_TRANSPORT
#include "captive_portal.h"
#else
#include "ble_prov.h"
#endif

namespace {
DeviceState currentState = DeviceState::BOOT_DECISION;
bool provisioningAfterFailure = false;
bool transportActive = false;
bool mainAppStarted = false;
uint32_t stateEnteredMs = 0;
uint32_t bootMs = 0;
uint8_t deviceMac[6] = {0};

void readDeviceMac() {
    esp_read_mac(deviceMac, ESP_MAC_WIFI_STA);
}

void enterState(DeviceState next) {
    if (next != currentState) {
        logStateTransition(currentState, next, millis() - bootMs);
    }
    currentState = next;
    stateEnteredMs = millis();
}

void stopTransport() {
    if (!transportActive) {
        return;
    }
#ifndef BLE_TRANSPORT
    CaptivePortal::stop();
#else
    BleProv::stop();
#endif
    transportActive = false;
}

void startTransport() {
    stopTransport();
#ifndef BLE_TRANSPORT
    String apSsid = macToSetupApSsid(deviceMac);
    CaptivePortal::start(apSsid);
    logSoftApStart(apSsid, CAPTIVE_PORTAL_IP);
#else
    String serviceName =
#ifdef PROV_SERVICE_NAME
        PROV_SERVICE_NAME;
#else
        macToBleServiceName(deviceMac);
#endif
    BleProv::start(serviceName, PROV_POP);
#endif
    transportActive = true;
}

void beginStaConnect(const String& ssid, const String& password) {
    stopTransport();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    WiFi.begin(ssid.c_str(), password.c_str());
    enterState(DeviceState::STA_CONNECTING);
}

void handleFactoryReset() {
    enterState(DeviceState::FACTORY_RESET);
    LedStatus::flashWhite();
    NvsStore::eraseWiFiCredentials();
    delay(200);
    ESP.restart();
}

void onWifiGotIp() {
    if (currentState != DeviceState::STA_CONNECTING && currentState != DeviceState::RECONNECTING) {
        return;
    }

    logWifiConnected(WiFi.localIP(), WiFi.RSSI());
    StateTransitionResult result =
        transitionState(currentState, Event::WIFI_CONNECTED, provisioningAfterFailure);
    provisioningAfterFailure = result.provisioningAfterFailure;
    enterState(result.nextState);

    if (result.callStartMainApp && !mainAppStarted) {
        mainAppStarted = true;
        startMainApp();
    }
}

void onWifiEvent(arduino_event_t* event) {
    switch (event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            onWifiGotIp();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (currentState == DeviceState::PRODUCTION) {
                logWifiFailure("disconnect", event->event_info.wifi_sta_disconnected.reason, 0);
                StateTransitionResult result =
                    transitionState(currentState, Event::WIFI_DISCONNECT, provisioningAfterFailure);
                provisioningAfterFailure = result.provisioningAfterFailure;
                enterState(result.nextState);
            }
            break;
        default:
            break;
    }
}

void pollTimeouts() {
    uint32_t elapsed = millis() - stateEnteredMs;

    if (currentState == DeviceState::STA_CONNECTING && elapsed >= STA_CONNECT_TIMEOUT_MS) {
        logWifiFailure("connect", WiFi.status(), STA_CONNECT_TIMEOUT_MS);
        StateTransitionResult result =
            transitionState(currentState, Event::WIFI_TIMEOUT, provisioningAfterFailure);
        provisioningAfterFailure = result.provisioningAfterFailure;
        enterState(result.nextState);
        startTransport();
        return;
    }

    if (currentState == DeviceState::RECONNECTING && elapsed >= WIFI_RECONNECT_TIMEOUT_MS) {
        logWifiFailure("reconnect", WiFi.status(), WIFI_RECONNECT_TIMEOUT_MS);
        StateTransitionResult result =
            transitionState(currentState, Event::RECONNECT_TIMEOUT, provisioningAfterFailure);
        provisioningAfterFailure = result.provisioningAfterFailure;
        enterState(result.nextState);
        startTransport();
    }
}

void pollProvisioningTransport() {
    if (currentState != DeviceState::PROVISIONING) {
        return;
    }

#ifndef BLE_TRANSPORT
    CaptivePortal::loop();
    if (CaptivePortal::hasNewCredentials()) {
        PortalCredentials portalCreds = CaptivePortal::getCredentials();
        if (NvsStore::saveCredentials(portalCreds.ssid, portalCreds.password)) {
            beginStaConnect(portalCreds.ssid, portalCreds.password);
            provisioningAfterFailure = false;
        }
    }
#else
    BleProv::loop();
    if (BleProv::hasNewCredentials()) {
        WiFiCredentials bleCreds = BleProv::getCredentials();
        if (NvsStore::saveCredentials(bleCreds.ssid, bleCreds.password)) {
            beginStaConnect(bleCreds.ssid, bleCreds.password);
            provisioningAfterFailure = false;
        }
    }
#endif
}

void runBootDecision() {
    WiFiCredentials creds = NvsStore::loadCredentials();
    BootDecisionInput input{creds.configured, creds.ssid.length() > 0};
    DeviceState next = bootDecision(input);

    if (next == DeviceState::STA_CONNECTING) {
        enterState(DeviceState::STA_CONNECTING);
        beginStaConnect(creds.ssid, creds.password);
    } else {
        provisioningAfterFailure = false;
        enterState(DeviceState::PROVISIONING);
        startTransport();
    }
}
}  // namespace

void initProvisioning() {
    bootMs = millis();
    readDeviceMac();
    WiFi.onEvent(onWifiEvent);
    WiFi.setAutoReconnect(true);
    enterState(DeviceState::BOOT_DECISION);
    runBootDecision();
}

void loopProvisioning() {
    if (BootButton::update()) {
        handleFactoryReset();
        return;
    }

    pollTimeouts();
    pollProvisioningTransport();

    if (currentState == DeviceState::RECONNECTING && WiFi.status() == WL_CONNECTED &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
        onWifiGotIp();
    }

    loopMainApp();
}

DeviceState getCurrentState() { return currentState; }

bool isProvisioningAfterFailure() { return provisioningAfterFailure; }
