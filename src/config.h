#pragma once

#include <Arduino.h>

// --- Transport selection ---

// --- Timeouts ---
#define STA_CONNECT_TIMEOUT_MS 30000
#define WIFI_RECONNECT_TIMEOUT_MS 60000
#define FACTORY_RESET_HOLD_MS 5000
#define BOOT_DECISION_MAX_MS 2000
#define BUTTON_DEBOUNCE_MS 50

// --- SoftAP ---
// #define SETUP_AP_PASSWORD "mypassword"
#define CAPTIVE_PORTAL_IP IPAddress(192, 168, 4, 1)

// --- BLE ---
#define PROV_POP "abcd1234"
// #define PROV_SERVICE_NAME "PROV_CUSTOM"

// --- LED ---
#define LED_PIN 8
#define LED_COUNT 1
#define LED_BLINK_INTERVAL_MS 500

// --- Boot button (ESP32-C61-DevKitC-1) ---
#define BOOT_BUTTON_PIN 9

// --- Serial ---
#define SERIAL_BAUD 115200

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0-phase1"
#endif

// --- Heartbeat ---
#define HEARTBEAT_INTERVAL_MS 30000
