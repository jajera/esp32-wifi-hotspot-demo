# Implementation Plan: ESP32 WiFi Provisioning

## Overview

This plan implements **Phase 1 WiFi onboarding** firmware for ESP32-C61-DevKitC-1 v2.0. The implementation follows a bottom-up approach: project scaffolding and configuration first, then core modules (NVS, LED, boot button), transport layers (captive portal default, BLE optional), state machine orchestration, application hook wiring, documentation, and hardware demo validation.

Two transport profiles share the same core modules; only the provisioning transport differs:

| Profile | PlatformIO env | Transport |
|---------|----------------|-----------|
| Default demo | `esp32-c61-softap` | SoftAP + captive portal |
| Production-oriented | `esp32-c61-ble` | BLE + ESP BLE Prov app |

Property-based tests using RapidCheck validate correctness properties on the host-native build. Hardware integration validates Req 12 on physical boards.

### Milestone order

```
Scaffold → core modules → transports → state machine → wire main → README → hardware demo
```

Phase 1 does **not** implement AWS IoT, MQTT, or `aws_provisioned` writes.

## Tasks

- [ ] 1. Set up project structure and build configuration
  - [ ] 1.1 Create PlatformIO project with board configuration and environments
    - Create `platformio.ini` with three environments: `esp32-c61-softap`, `esp32-c61-ble`, `native`
    - Configure `board = esp32-c61-devkitc-1`, `framework = arduino`, `monitor_speed = 115200`
    - Add `-Wall` and `-DFIRMWARE_VERSION=...` to build_flags per Req 11.4 and NF4
    - Pin `arduino-esp32` v3.x via `platform_packages` if C61 is not yet in default registry (see design research notes)
    - Add RapidCheck as a dependency for the native test environment
    - Create directory structure: `src/`, `test/native/`, `test/mocks/`, `test/embedded/`
    - Add `.gitignore` for `.pio/`, build artifacts
    - _Requirements: 8.5, 11.1, 11.2, NF4_

  - [ ] 1.2 Create `config.h` with compile-time constants
    - Define all timeouts: `STA_CONNECT_TIMEOUT_MS`, `WIFI_RECONNECT_TIMEOUT_MS`, `FACTORY_RESET_HOLD_MS`, `BOOT_DECISION_MAX_MS`, `BUTTON_DEBOUNCE_MS`
    - Define SoftAP constants: `CAPTIVE_PORTAL_IP`, optional `SETUP_AP_PASSWORD`
    - Define BLE constants: `PROV_POP` (demo default `abcd1234`), optional `PROV_SERVICE_NAME`
    - Define LED constants: `LED_PIN` (8), `LED_COUNT`, `LED_BLINK_INTERVAL_MS`
    - Define serial and firmware constants: `SERIAL_BAUD` (115200), `FIRMWARE_VERSION`, `HEARTBEAT_INTERVAL_MS`
  - Do **not** embed site WiFi SSID or password
    - _Requirements: 3.7, 5.1, 8.3, 8.4, 11.4_

  - [ ] 1.3 Create test mock headers for HAL abstraction
    - Create `test/mocks/mock_nvs.h` — in-memory key-value store simulating Preferences API (include separate `aws` namespace for Property 7)
    - Create `test/mocks/mock_wifi.h` — stub WiFi with injectable connect/disconnect events and disconnect reason codes
    - Create `test/mocks/mock_led.h` — capture LED state changes for verification
    - _Design: testing strategy, host-compilation strategy_

- [ ] 2. Implement NVS credential store
  - [ ] 2.1 Implement `nvs_store.h` and `nvs_store.cpp`
    - Define `WiFiCredentials` struct with `ssid`, `password`, `configured` fields
    - Implement `NvsStore::init()` opening `"wifi"` namespace via Preferences
    - Implement `NvsStore::loadCredentials()` reading `wifi_ssid`, `wifi_password`, `wifi_configured`
    - Implement `NvsStore::saveCredentials()` writing all three keys atomically
    - Implement `NvsStore::eraseWiFiCredentials()` clearing WiFi keys without touching `"aws"` namespace or `aws_provisioned`
    - Implement `NvsStore::isConfigured()` reading the boolean flag
    - Handle NVS read failures gracefully (treat as unconfigured per Req 1.2)
    - _Requirements: 4.1, 4.2, 4.4, 4.5_

  - [ ]* 2.2 Write property test for NVS round-trip (Property 6)
    - **Property 6: NVS credential round-trip**
    - Generate random valid credential pairs (SSID 1–32 bytes, password 0–64 bytes)
    - Write via `saveCredentials()`, read back via `loadCredentials()`, assert equality and `configured=true`
    - Use mock NVS backend for host-native execution; tag: `// Feature: esp32-wifi-provisioning, Property 6: NVS credential round-trip`
    - Minimum 100 RapidCheck iterations
    - **Validates: Requirements 4.1, 9.5**

  - [ ]* 2.3 Write property test for factory reset selective erase (Property 7)
    - **Property 7: Factory reset selective erase**
    - Generate random NVS states with various `aws_provisioned` values in `"aws"` namespace
    - Call `eraseWiFiCredentials()`, verify WiFi keys cleared and `aws_provisioned` unchanged
    - **Validates: Requirements 4.5, 5.2**

- [ ] 3. Implement credential validation
  - [ ] 3.1 Implement credential validation function (in `provisioning.cpp` or `validation.h`)
    - Accept SSID if non-empty and ≤32 bytes
    - Accept password if ≤64 bytes (empty allowed for open networks)
    - Return validation result struct with error reason for logging
    - Reject invalid inputs without modifying NVS state
    - _Requirements: 2.6_

  - [ ]* 3.2 Write property test for credential validation (Property 5)
    - **Property 5: Credential validation**
    - Generate random strings of length 0–100 including unicode and whitespace
    - Assert acceptance iff SSID is non-empty AND ≤32 bytes, password ≤64 bytes
    - Assert invalid inputs never modify NVS state
    - **Validates: Requirements 2.6**

- [ ] 4. Implement LED status driver
  - [ ] 4.1 Implement `led_status.h` and `led_status.cpp`
    - Initialize NeoPixel on GPIO8 **after** boot completes; do not drive GPIO8 low during reset (strapping pin per design)
    - Resolve O1: try Adafruit NeoPixel or `neopixelWrite`; fall back if timing fails on C61
    - Implement `LedStatus::update(DeviceState, bool provisioningAfterFailure)` with pattern mapping:
      - PROVISIONING + !afterFailure → blink blue (500 ms on/off)
      - PROVISIONING + afterFailure → blink red (500 ms on/off)
      - STA_CONNECTING or RECONNECTING → blink yellow (500 ms on/off)
      - PRODUCTION → solid green
    - Implement `LedStatus::flashWhite()` for factory reset acknowledgment (200 ms flash)
    - Log warning and continue if RGB driver init fails
    - _Requirements: 6.1, 6.2_

  - [ ]* 4.2 Write property test for LED state mapping (Property 9)
    - **Property 9: LED state mapping**
    - For all (DeviceState, provisioningAfterFailure) combinations, assert correct (color, pattern) output
    - **Validates: Requirements 6.1**

- [ ] 5. Implement boot button with debounce and hold detection
  - [ ] 5.1 Implement `boot_button.h` and `boot_button.cpp`
    - Configure Boot button GPIO as active-low input with pull-up (DevKitC standard wiring)
    - Implement debounce logic with `BUTTON_DEBOUNCE_MS` (50 ms) window
    - Track continuous hold duration; trigger factory reset at `FACTORY_RESET_HOLD_MS` (5000 ms)
    - Expose `BootButton::init()`, `BootButton::update()`, `BootButton::isHeld()`
    - _Requirements: 5.1, 5.4_

  - [ ]* 5.2 Write property test for button hold threshold (Property 8)
    - **Property 8: Button hold threshold and debounce**
    - Generate random durations 0–30000 ms with debounce noise simulation
    - Assert reset triggers iff debounced pressed duration ≥ `FACTORY_RESET_HOLD_MS`
    - Assert rapid toggles shorter than `BUTTON_DEBOUNCE_MS` do not trigger
    - **Validates: Requirements 5.1, 5.4**

- [ ] 6. Checkpoint — Core modules complete
  - Run `pio test -e native` for completed property tests
  - Ensure mocks compile; ask the user if questions arise

- [ ] 7. Implement MAC-to-identifier derivation and logging utilities
  - [ ] 7.1 Implement identifier derivation helpers
    - Implement `macToSetupApSsid(const uint8_t mac[6])` → `"ESP-Setup-EEFF"`
    - Implement `macToBleServiceName(const uint8_t mac[6])` → `"PROV_EEFF"`
    - Both use last 2 bytes of MAC as uppercase hex suffix; deterministic per MAC
    - _Requirements: 2.1, 8.1, 9.3_

  - [ ]* 7.2 Write property test for MAC derivation (Property 4)
    - **Property 4: MAC-to-identifier derivation**
    - Generate random 6-byte arrays
    - Assert output matches `ESP-Setup-XXXX` / `PROV_XXXX` pattern with correct hex
    - Assert determinism (same MAC → same output)
    - **Validates: Requirements 2.1, 8.1, 9.3**

  - [ ] 7.3 Implement serial log formatting functions
    - Boot log: firmware version, chip model (ESP32-C61), MAC, transport (`softap`/`ble`), `wifi_configured`
    - State transition log: from-state, to-state, uptime ms
    - SoftAP start log: SSID, gateway IP
    - BLE start log: service name, `pop_required=true` (never log PoP value)
    - Credential log: success/failure with SSID length only, never password
    - WiFi failure log: disconnect reason or error code when available from WiFi stack
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7_

  - [ ]* 7.4 Write property test for password never logged (Property 10)
    - **Property 10: Password never logged**
    - Generate random passwords including special chars, empty, very long
    - Capture log output from credential handler, assert password not in output
    - **Validates: Requirements 7.6**

  - [ ]* 7.5 Write property test for log format completeness (Property 11)
    - **Property 11: Log format completeness**
    - Generate random MACs, states, uptimes, transports
    - Assert boot log contains all required fields; transition log contains from/to/uptime
    - Assert SoftAP log has SSID+gateway; BLE log has service name + pop_required (not value)
    - **Validates: Requirements 7.2, 7.3, 7.4, 7.5**

- [ ] 8. Implement SoftAP captive portal transport
  - [ ] 8.1 Implement `captive_portal.h` and `captive_portal.cpp`
    - Start SoftAP with MAC-derived SSID (open or with `SETUP_AP_PASSWORD` if defined)
    - Initialize `DNSServer` on port 53 redirecting all queries to gateway IP (`192.168.4.1`)
    - Initialize `WebServer` on port 80 with routes: `/`, `/scan`, `/save`, captive detect endpoints
    - Serve minimal HTML form for SSID + password entry (keep page small for NF2)
    - Handle `POST /save`: validate via shared validation function, extract credentials, set `hasNewCredentials` flag
    - Implement `GET /scan` returning JSON array of visible **2.4 GHz** networks when scan is feasible
    - Implement captive portal detection redirects: `/generate_204`, `/hotspot-detect.html`, `/connecttest.txt`
    - Implement `CaptivePortal::loop()` processing DNS + HTTP each main loop iteration
    - Implement `CaptivePortal::stop()` tearing down AP, DNS, and WebServer
    - Compile-guard entire module with `#ifndef BLE_TRANSPORT`
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.7, 3.5, 9.6, NF2_

  - [ ]* 8.2 Write unit tests for captive portal
    - Test SSID format matches `ESP-Setup-XXXX` for known MAC
    - Test open AP when no password defined
    - Test HTML form contains ssid and password fields
    - _Requirements: 2.1, 2.2, 2.5_

- [ ] 9. Implement BLE provisioning transport
  - [ ] 9.1 Implement `ble_prov.h` and `ble_prov.cpp`
    - Start BLE advertising with `WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, serviceName)`
    - PoP: compile-time `PROV_POP` (demo `abcd1234`); never log PoP value
    - Service name: MAC-derived `PROV_XXXX` or compile-time `PROV_SERVICE_NAME`
    - Handle `ARDUINO_EVENT_PROV_CRED_RECV` to extract SSID/password
    - Implement `BleProv::stop()` releasing BTDM memory before STA connect
    - Compile-guard entire module with `#ifdef BLE_TRANSPORT`
    - Do NOT start SoftAP when BLE transport is active
    - _Requirements: 9.1, 9.2, 9.3, 9.4, 9.5, 9.6_

  - [ ]* 9.2 Write unit tests for BLE provisioning
    - Test BLE not initialized when `BLE_TRANSPORT` undefined
    - Test BLE resources freed after provisioning completes
    - _Requirements: 9.4, 9.6_

- [ ] 10. Implement state machine orchestrator
  - [ ] 10.1 Implement `provisioning.h` and `provisioning.cpp` state machine
    - Define `DeviceState` enum: BOOT_DECISION, PROVISIONING, STA_CONNECTING, PRODUCTION, RECONNECTING, FACTORY_RESET
    - Define `Event` enum: CREDS_PRESENT, CREDS_MISSING, CREDENTIALS_RECEIVED, WIFI_CONNECTED, WIFI_TIMEOUT, WIFI_DISCONNECT, RECONNECT_TIMEOUT, BUTTON_HELD
    - Implement boot decision: read NVS → STA_CONNECTING if configured+creds present, else PROVISIONING; complete within `BOOT_DECISION_MAX_MS`
    - Do NOT start AWS IoT, MQTT, or TLS to cloud endpoints in Phase 1
    - Implement transition function handling all valid (state, event) pairs per design transition table
    - Track `provisioningAfterFailure` flag: set true on STA/reconnect timeouts, cleared on new credentials received
    - Expose `isProvisioningAfterFailure()` for LED driver
    - Implement 30 s STA connection timeout → PROVISIONING with after_failure=true (creds kept in NVS)
    - Implement 60 s reconnect timeout in RECONNECTING → PROVISIONING with after_failure=true
    - On WiFi failure, log disconnect reason when available
    - Ensure `startMainApp()` called exactly once per PRODUCTION entry (WiFi connected + IP assigned)
    - Wire transport: SoftAP path calls `CaptivePortal::*`; BLE path calls `BleProv::*` (compile-guarded)
    - Wire factory reset: any state + BUTTON_HELD → erase WiFi NVS, flash white LED, reboot into Provisioning_Mode
    - Implement `initProvisioning()` (setup) and `loopProvisioning()` (loop)
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 2.8, 2.9, 3.2, 3.4, 3.6, 5.2, 5.3, 7.7_

  - [ ]* 10.2 Write property test for boot decision correctness (Property 1)
    - **Property 1: Boot decision correctness**
    - Generate random NvsState structs (bool configured, String ssid 0–32, String pass 0–64)
    - Assert: configured AND non-empty ssid → STA_CONNECTING; otherwise → PROVISIONING
    - **Validates: Requirements 1.1, 1.2**

  - [ ]* 10.3 Write property test for state machine transitions (Property 2)
    - **Property 2: State machine transition correctness**
    - Generate all valid (DeviceState, Event) pairs
    - Assert each transition produces correct next state per transition table
    - Assert `after_failure` flag set/cleared correctly
    - **Validates: Requirements 1.5, 2.8, 2.9, 3.4, 3.6**

  - [ ]* 10.4 Write property test for startMainApp exactly-once (Property 3)
    - **Property 3: startMainApp exactly-once invariant**
    - Generate random event sequences of length 1–50
    - Simulate transitions, count `startMainApp` invocations
    - Assert called exactly once per PRODUCTION entry, never in other states
    - **Validates: Requirements 3.2, 3.3, 10.1, 10.2**

- [ ] 11. Checkpoint — All modules implemented
  - Run `pio test -e native`; fix failures
  - Verify `pio run -e esp32-c61-softap` and `pio run -e esp32-c61-ble` compile with `-Wall`
  - Ask the user if questions arise

- [ ] 12. Wire application entry point and main loop
  - [ ] 12.1 Implement `app_main.cpp` with `startMainApp()` hook
    - Log periodic heartbeat with IP address every `HEARTBEAT_INTERVAL_MS` (30 s)
    - MAY log Phase 2 placeholder text; SHALL NOT require AWS credentials or cloud connectivity
    - Verify WiFi is connected before starting heartbeat
    - Keep module separate from provisioning logic (`app_main.cpp` only)
    - _Requirements: 10.1, 10.2, 10.3, 10.4_

  - [ ] 12.2 Implement `main.cpp` with `setup()` and `loop()`
    - `setup()`: serial at 115200, `LedStatus::init()`, `BootButton::init()`, `NvsStore::init()`, `initProvisioning()`
    - `loop()`: `BootButton::update()`, `loopProvisioning()`, `LedStatus::update(getCurrentState(), isProvisioningAfterFailure())`
    - Log boot banner via shared log formatters (task 7.3)
    - _Requirements: 7.1, 7.2, 10.3, 11.4_

  - [ ]* 12.3 Write property test for transport exclusivity (Property 12)
    - **Property 12: Transport exclusivity**
    - Verify that with BLE_TRANSPORT enabled only BLE starts (no SoftAP)
    - Verify that with BLE_TRANSPORT disabled only SoftAP starts (no BLE init)
    - Verify both paths write identical NVS keys
    - **Validates: Requirements 9.1, 9.5, 9.6**

- [ ] 13. Write README and project documentation
  - [ ] 13.1 Update root `README.md`
    - Project purpose: Phase 1 WiFi onboarding (SoftAP default, BLE optional)
    - Target hardware: ESP32-C61-DevKitC-1 v2.0, UART USB port for flash/serial
    - Flash commands: `pio run -e esp32-c61-softap -t upload` (and BLE variant)
    - Monitor: `pio device monitor -b 115200`
    - Boot+Reset recovery instructions for flash mode
    - Note: 2.4 GHz WiFi only; 5 GHz SSIDs unsupported (Req 3.5)
    - BLE build: ESP BLE Prov app required; demo PoP `abcd1234` (document production rotation per Req 9.2)
    - Phase 2 scope boundary: no AWS in this firmware yet
    - _Requirements: 3.5, 9.2, 11.3, 12.6_

  - [ ] 13.2 Resolve open item O4 — demo walkthrough location
    - Default: document minimal demo steps in README; full walkthrough in separate `esp32-wifi-provisioning-walkthrough` repo if following jajera convention
    - Walkthrough prerequisites: 2.4 GHz WiFi, phone/laptop, ESP BLE Prov app if BLE build
    - _Requirements: 12.6, Open item O4_

- [ ] 14. Hardware integration and demo acceptance
  - [ ] 14.1 Execute Req 12 checklist on physical ESP32-C61-DevKitC-1 v2.0 (minimum 2 boards, SoftAP build)
    1. Flash same binary to Device A and Device B (Req 12.1)
    2. Device A: first boot → join `ESP-Setup-XXXX` → captive portal → Production (green LED, serial IP) (Req 12.2)
    3. Device A: reboot → STA connect without Setup_AP within 30 s (Req 12.3)
    4. Device B: repeat steps 2–3 independently; verify distinct MAC-derived SSIDs (Req 12.4)
    5. Factory reset (Boot 5 s, white flash) → re-provision same or different SSID (Req 12.5)
    6. Wrong password → 30 s timeout → red LED provisioning, creds kept, correct via portal (Req 2.9)
    7. Disconnect router during Production → yellow reconnect 60 s → red provisioning if no recovery (Req 3.6)
    - Record pass/fail in walkthrough doc or README demo section
    - _Requirements: 12.1, 12.2, 12.3, 12.4, 12.5, 2.9, 3.6_

  - [ ]* 14.2 Repeat critical paths on `esp32-c61-ble` build (optional)
    - First boot via ESP BLE Prov app with PoP
    - Reboot persistence and factory reset
    - _Requirements: 9.1–9.5, 12.2–12.5_

- [ ] 15. Final checkpoint — Full build verification
  - `pio test -e native` — all property tests pass (100+ iterations each)
  - `pio run -e esp32-c61-softap` and `pio run -e esp32-c61-ble` — compile with `-Wall`, zero warnings in project code (NF4)
  - Boot to Provisioning_Mode or first STA attempt observed within 5 s on hardware (NF1)
  - Hardware demo checklist (task 14.1) signed off
  - Ask the user if questions arise before Phase 2 planning

## Notes

- Tasks marked with `*` are optional and can be skipped for a faster MVP; recommended before calling Phase 1 complete
- Each task references specific requirements for traceability
- Checkpoints at tasks 6, 11, and 15 ensure incremental validation
- Property tests use RapidCheck on `[env:native]` with minimum 100 iterations and reproducible seeds
- Unit tests cover specific examples; integration tests require physical C61 boards
- Compile guards (`BLE_TRANSPORT`) produce correct binaries per transport profile — no runtime switching
- Implementation language: C++ with Arduino framework on PlatformIO
- Phase 1 SHALL NOT write `aws_provisioned` or start cloud connectivity (Req 1.4)
- NF3 (avoid heap alloc in hot path) is recommended, not blocking for demo

### Open items (resolve during implementation)

| ID | Item | Task owner |
|----|------|------------|
| O1 | RGB LED library on GPIO8 | 4.1 |
| O2 | WebServer + DNSServer (not AsyncWebServer) | 8.1 |
| O3 | Fixed fleet PoP `abcd1234` for BLE demo | 1.2, 9.1, 13.1 |
| O4 | Walkthrough repo vs README-only demo | 13.2 |

## Task dependency graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2", "1.3"] },
    { "id": 1, "tasks": ["2.1", "3.1", "4.1", "5.1"] },
    { "id": 2, "tasks": ["2.2", "2.3", "3.2", "4.2", "5.2"] },
    { "id": 3, "tasks": ["7.1", "7.3", "8.1", "9.1"] },
    { "id": 4, "tasks": ["7.2", "7.4", "7.5", "8.2", "9.2"] },
    { "id": 5, "tasks": ["10.1"] },
    { "id": 6, "tasks": ["10.2", "10.3", "10.4"] },
    { "id": 7, "tasks": ["12.1", "12.2", "12.3"] },
    { "id": 8, "tasks": ["13.1", "13.2"] },
    { "id": 9, "tasks": ["14.1", "14.2"] }
  ]
}
```

## Revision history

| Version | Date | Notes |
|---------|------|-------|
| 0.1 | 2026-07-06 | Initial implementation plan |
| 0.2 | 2026-07-06 | Review: README/demo tasks (Req 11–12), NF refs, GPIO8 strapping, WiFi disconnect logging, open items, dependency waves |
