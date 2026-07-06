# Requirements Document

## Introduction

This document defines the requirements for **Phase 1: first-boot WiFi onboarding** on **ESP32-C61** devices.

The firmware ships with no site WiFi configured. On first boot, an Installer provisions site credentials using one of two transports:

1. **Default build — SoftAP + captive portal:** join a device-hosted setup network and complete a browser form (no phone app).
2. **Optional build — BLE:** use the Espressif **ESP BLE Prov** app when `BLE_TRANSPORT` is enabled at compile time.

Credentials are stored in NVS. Subsequent boots connect in STA mode without repeating setup.

This is the WiFi milestone that must work before any cloud onboarding. **Phase 2 (out of scope here)** adds AWS IoT fleet provisioning after WiFi is established. The architecture must support a single firmware binary per transport profile across all devices and rolling enrollment of ~1000 devices over time (one device at a time, not bulk).

### Design note (transport choice)

| Profile | Primary transport | Best for |
|---------|-------------------|----------|
| **Default (`BLE_TRANSPORT` off)** | SoftAP + captive portal | Demo without app; universal browser UX |
| **Production-oriented (`BLE_TRANSPORT` on)** | BLE + Espressif Unified Provisioning | Field install; aligns with consumer IoT (Tapo/Reolink-style BLE path) |

Both profiles SHALL share the same NVS layout, state machine, LED semantics, factory reset, and `startMainApp` hook. Only the provisioning transport differs.

### Scope

| In scope (Phase 1) | Out of scope (Phase 2+) |
|--------------------|-------------------------|
| Boot decision: provision vs connect | AWS IoT fleet provisioning |
| SoftAP + captive portal (default build) | Claim cert handling, RegisterThing |
| Optional BLE provisioning (`BLE_TRANSPORT`) | MQTT, staging → approve → live |
| WiFi STA connect + reconnect | OTA, Jobs, Device Shadow |
| NVS persistence of WiFi credentials | Custom mobile app development |
| Factory reset, LED, serial diagnostics | Bulk AWS registration / device manifests |
| `startMainApp` placeholder hook | Hardcoded site WiFi at compile time |

### Milestone order

```
Power on → WiFi onboarding (this spec) → startMainApp → [later] AWS fleet provision
```

`wifi_configured` and `aws_provisioned` are **independent NVS state**. Phase 1 implements only the WiFi path. Phase 1 SHALL NOT set `aws_provisioned`.

### Target hardware

| Item | Value |
|------|--------|
| Board | ESP32-C61-DevKitC-1 **v2.0** |
| Module | ESP32-C61-WROOM-1 (8 MB flash / 2 MB PSRAM typical) |
| SoC | ESP32-C61, RISC-V single-core, 2.4 GHz WiFi 6 + BLE 5 |
| RGB LED | GPIO8 (addressable; strapping pin — driver must not break boot) |
| Boot button | Factory reset trigger |
| USB (flash/serial) | **USB Type-C to UART** port (not chip-native USB as primary; per Espressif v2.0 guide) |
| Serial | 115200 baud |

**Not in scope:** ESP32-S3, ESP32-C3-DevKit-RUST-1, classic `esp32dev` boards.

### Toolchain

| Item | Value |
|------|--------|
| Language | C++ |
| Framework | Arduino (ESP32 core) |
| Build | PlatformIO (primary) or Arduino IDE 2.x |
| Board target | `esp32-c61-devkitc-1` |
| BLE path library | `WiFiProv` (arduino-esp32) when `BLE_TRANSPORT` enabled |

---

## Glossary

| Term | Definition |
|------|------------|
| **Firmware** | Compiled C++/Arduino binary flashed to the device |
| **Device** | ESP32-C61-DevKitC-1 v2.0 running the Firmware |
| **NVS** | Non-volatile flash partition for WiFi credentials and state flags |
| **STA** | WiFi station (client) mode |
| **Provisioning_Mode** | Setup state: SoftAP portal active and/or BLE advertising |
| **Production_Mode** | WiFi connected; `startMainApp` runs |
| **Installer** | Operator configuring site WiFi via browser or ESP BLE Prov app |
| **Setup_AP** | SoftAP started in Provisioning_Mode; SSID `ESP-Setup-XXXX` (`XXXX` = last 2 bytes of MAC, uppercase hex) |
| **Captive_Portal** | HTTP UI on Setup_AP gateway (`192.168.4.1` default); DNS redirect steers clients to setup page |
| **wifi_configured** | NVS bool: valid WiFi credentials stored |
| **aws_provisioned** | NVS bool reserved for Phase 2; never set in Phase 1 |
| **Factory_Reset** | Erase WiFi NVS keys + clear `wifi_configured`; reboot to Provisioning_Mode |
| **LED_Indicator** | Addressable RGB on GPIO8 |
| **State_Machine** | Boot logic selecting Provisioning_Mode vs Production_Mode |
| **BLE_Transport** | Compile-time flag enabling BLE provisioning instead of SoftAP |
| **PoP** | Proof of Possession string for BLE session (`WIFI_PROV_SECURITY_1`); N/A for SoftAP-only build |

---

## Requirements

### Requirement 1: Boot Decision Logic

**User Story:** As an Installer, I want the device to automatically detect whether WiFi credentials exist, so that configured devices connect immediately and unconfigured devices enter setup mode.

#### Acceptance Criteria

1. WHEN the Device powers on AND `wifi_configured` is true AND stored credentials are present, THE State_Machine SHALL enter the STA connect path toward Production_Mode.
2. WHEN the Device powers on AND (`wifi_configured` is false OR credentials are missing OR NVS read fails), THE State_Machine SHALL enter Provisioning_Mode.
3. THE State_Machine SHALL complete the boot decision within **2 seconds** of power-on (excluding intentional Factory_Reset handling).
4. THE State_Machine SHALL NOT start AWS IoT, MQTT, or TLS connections to cloud endpoints in Phase 1.
5. IF STA connection fails after the timeout in Requirement 3, THEN THE State_Machine SHALL clear or invalidate the connection attempt, log the failure, and re-enter Provisioning_Mode (credentials remain in NVS unless validation determines they are corrupt — see Requirement 4).

---

### Requirement 2: SoftAP and Captive Portal (default build)

**User Story:** As an Installer, I want the device to host a setup WiFi network with a browser configuration page on first boot, so that I can enter site WiFi credentials without reflashing or installing an app.

**Applies when:** `BLE_TRANSPORT` is **disabled**.

#### Acceptance Criteria

1. WHEN the Device enters Provisioning_Mode, THE Device SHALL start Setup_AP with SSID `ESP-Setup-XXXX` derived from the device MAC at runtime.
2. Setup_AP SHALL use an **open** network (no WPA password) unless a compile-time `SETUP_AP_PASSWORD` is defined; if defined, the password SHALL be identical for all devices in the fleet (demo default: open).
3. THE Captive_Portal SHALL be reachable at `192.168.4.1` (configurable constant; default as specified).
4. THE Device SHALL run DNS hijack on the Setup_AP so clients are steered to the Captive_Portal without manual URL entry.
5. THE Captive_Portal SHALL present a form accepting target WiFi SSID and password; it SHOULD offer a scan list of visible 2.4 GHz networks when feasible on ESP32-C61.
6. WHEN the Installer submits credentials, THE Device SHALL validate non-empty SSID, write `wifi_ssid` and `wifi_password` to NVS, set `wifi_configured` true, and log success without logging the password.
7. WHEN credentials are stored, THE Device SHALL stop Setup_AP, switch to STA mode, and connect using the submitted credentials.
8. IF STA connection succeeds within **30 seconds**, THEN THE Device SHALL enter Production_Mode.
9. IF STA connection fails after **30 seconds**, THEN THE Device SHALL log the failure, remain with `wifi_configured` true (credentials kept), re-enter Provisioning_Mode, and restart Setup_AP so the Installer can correct SSID/password.

---

### Requirement 3: WiFi Connection

**User Story:** As an Installer, I want the device to connect to the configured WiFi network reliably, so that it is ready for its application workload and future cloud provisioning.

#### Acceptance Criteria

1. WHEN valid credentials exist in NVS, THE Device SHALL connect in STA mode on boot.
2. WHEN WiFi is connected, THE Device SHALL enter Production_Mode and invoke `startMainApp` exactly once per successful transition.
3. `startMainApp` SHALL run only after WiFi association and IP assignment (or explicit connected state from the WiFi stack); it SHALL NOT assume `aws_provisioned` or internet reachability beyond the LAN.
4. IF boot-time STA connection fails after **30 seconds**, THE Device SHALL follow Requirement 1.5 / 2.9 (re-enter Provisioning_Mode).
5. THE Device SHALL support **2.4 GHz** networks only (802.11 b/g/n/ax). Documentation SHALL state that 5 GHz-only SSIDs are unsupported.
6. WHILE in Production_Mode, IF WiFi disconnects, THE Device SHALL attempt automatic reconnection with stored credentials for at least **60 seconds** before re-entering Provisioning_Mode.
7. THE Firmware SHALL NOT embed site-specific SSID or password at compile time.

---

### Requirement 4: Credential Persistence

**User Story:** As an Installer, I want WiFi credentials to survive reboots and power cycles, so that setup is a one-time operation per device per network.

#### Acceptance Criteria

1. THE Device SHALL persist credentials in NVS namespace `wifi` (or equivalent documented namespace) using keys: `wifi_ssid`, `wifi_password`, `wifi_configured`.
2. Credentials SHALL survive power cycle and reboot.
3. WHEN `wifi_configured` is true on boot, THE Device SHALL NOT start Setup_AP or BLE advertising unless connection fails and Provisioning_Mode is re-entered per Requirement 2.9 / 3.4.
4. `wifi_configured` and `aws_provisioned` SHALL be separate keys so Phase 2 can be added without migrating WiFi NVS layout.
5. Factory_Reset SHALL erase `wifi_ssid`, `wifi_password`, and set `wifi_configured` false; it SHALL NOT modify `aws_provisioned`.

---

### Requirement 5: Factory Reset

**User Story:** As an Installer, I want a physical way to clear WiFi credentials, so that I can re-provision a device for a different network without reflashing.

#### Acceptance Criteria

1. WHEN Boot is held for **5 seconds** (compile-time constant `FACTORY_RESET_HOLD_MS`, default 5000), THE Device SHALL perform Factory_Reset.
2. Factory_Reset SHALL erase WiFi NVS keys, clear `wifi_configured`, leave `aws_provisioned` unchanged, flash LED white per Requirement 6, and reboot.
3. AFTER reboot from Factory_Reset, THE Device SHALL enter Provisioning_Mode.
4. THE Firmware SHALL debounce Boot input to avoid accidental resets during normal handling.

---

### Requirement 6: LED Status Feedback

**User Story:** As an Installer, I want distinct LED patterns for each device state, so that I can visually confirm device behavior without serial.

**Hardware:** Addressable RGB on **GPIO8**. Implementation MUST respect GPIO8 strapping behavior during boot.

#### Acceptance Criteria

| State | LED pattern |
|-------|-------------|
| Provisioning_Mode (Setup_AP or BLE advertising) | Blink **blue** (500 ms on / 500 ms off) |
| STA connecting | Blink **yellow** |
| Production_Mode, WiFi connected | Solid **green** |
| WiFi failure, returned to Provisioning_Mode | Blink **red** |
| Factory_Reset acknowledged | Flash **white** once, then reboot |

1. Patterns SHALL be visible under normal indoor lighting.
2. IF RGB driver init fails, THE Device SHALL continue provisioning; serial logs SHALL note LED failure.

---

### Requirement 7: Serial Log Output

**User Story:** As an Installer, I want serial diagnostics, so that I can troubleshoot setup and confirm device identity.

#### Acceptance Criteria

1. Serial SHALL be **115200** baud.
2. ON boot, log: firmware version string, chip model (ESP32-C61), MAC address, active transport (`softap` or `ble`), `wifi_configured` state.
3. ON each State_Machine transition, log: from-state, to-state, uptime ms.
4. WHEN Setup_AP starts, log: Setup_AP SSID and gateway IP.
5. WHEN BLE_Transport is active, log: BLE service name (e.g. `PROV_XXX`) and whether PoP is required (never log PoP value).
6. ON credential submission, log success or validation error; **passwords SHALL NOT be logged**.
7. ON WiFi failure, log disconnect reason or error code from the WiFi stack when available.

---

### Requirement 8: Uniform Firmware Binary

**User Story:** As a developer, I want one firmware binary per transport profile for all devices, so manufacturing and deployment need no per-device customization.

#### Acceptance Criteria

1. Setup_AP SSID suffix and BLE service name suffix SHALL be derived from MAC at runtime.
2. A given build (`BLE_TRANSPORT` on or off) SHALL produce identical binaries for all devices (no per-device compile flags).
3. Site WiFi credentials SHALL NOT be compile-time embedded.
4. Fleet-wide compile-time constants (reset hold duration, timeouts, PoP string for BLE builds, optional Setup_AP password) SHALL be identical across devices.
5. PlatformIO environment SHALL target `esp32-c61-devkitc-1` only for this project.

---

### Requirement 9: Optional BLE Transport

**User Story:** As a developer, I want an optional BLE provisioning path, so that devices can be configured with the Espressif ESP BLE Prov app when SoftAP is unsuitable.

**Applies when:** `BLE_TRANSPORT` is **enabled**.

#### Acceptance Criteria

1. WHEN entering Provisioning_Mode, THE Device SHALL NOT start Setup_AP; it SHALL advertise using Espressif Unified Provisioning over BLE via `WiFiProv` / `WIFI_PROV_SCHEME_BLE`.
2. THE Device SHALL use `WIFI_PROV_SECURITY_1` with compile-time `PROV_POP` string (demo default: `abcd1234`; document that production should use per-device or fleet-rotated PoP).
3. THE service name SHALL default to `PROV_XXX` (MAC-derived) unless `PROV_SERVICE_NAME` is set at compile time.
4. AFTER successful provisioning, THE Device SHALL release BLE resources (`WIFI_PROV_SCHEME_HANDLER_FREE_BTDM` or equivalent) before Production_Mode.
5. BLE_Transport SHALL write the same NVS keys as the Captive_Portal path: `wifi_ssid`, `wifi_password`, `wifi_configured`.
6. WHEN `BLE_TRANSPORT` is disabled, THE Device SHALL NOT initialize BLE for provisioning.

---

### Requirement 10: Production Mode Hook

**User Story:** As a developer, I want a clean application entry point, so that Phase 2 (AWS IoT) integrates without changing WiFi onboarding.

#### Acceptance Criteria

1. WHEN entering Production_Mode, THE Firmware SHALL call `startMainApp()` as the **sole** entry point for application logic.
2. `startMainApp()` SHALL only be called when WiFi reports connected with assigned IP (or documented equivalent).
3. WiFi onboarding (SoftAP, captive portal, BLE, state machine, NVS, factory reset, LED) SHALL live in separate compilation units from `startMainApp` (e.g. `provisioning.cpp`, `app_main.cpp`).
4. Phase 1 `startMainApp()` SHALL log a periodic heartbeat (e.g. IP address every 30 s) and MAY log placeholder text for Phase 2; it SHALL NOT require AWS credentials or cloud connectivity.

---

### Requirement 11: Build Configuration

**User Story:** As a developer, I want documented build profiles, so that demo and production-oriented images are reproducible.

#### Acceptance Criteria

1. THE repository SHALL provide PlatformIO environments at minimum:
   - `esp32-c61-softap` — `BLE_TRANSPORT` off (default demo)
   - `esp32-c61-ble` — `BLE_TRANSPORT` on
2. Both environments SHALL use `board = esp32-c61-devkitc-1`, `monitor_speed = 115200`.
3. README SHALL document flash command, Boot+Reset recovery, and which USB port to use (UART bridge).
4. THE Firmware SHALL expose `FIRMWARE_VERSION` string logged at boot.

---

### Requirement 12: Phase 1 Demo Acceptance

**User Story:** As a stakeholder, I want a repeatable two-board demo, so that the repeatable rollout story is proven before Phase 2.

#### Acceptance Criteria

1. Two identical Devices flash the same binary (per transport profile).
2. Device A: first boot → provision → Production_Mode (serial shows IP, LED green).
3. Device A: reboot → connects without Setup_AP/BLE → Production_Mode within 30 s.
4. Device B: repeat steps 2–3 independently.
5. Device A or B: Factory_Reset → Provisioning_Mode → re-provision to same or different SSID.
6. Demo walkthrough document (separate repo or `docs/`) SHALL list prerequisites: 2.4 GHz WiFi, phone/laptop, ESP BLE Prov app if BLE build.

---

## Non-functional requirements

| ID | Requirement |
|----|-------------|
| NF1 | Boot to Provisioning_Mode or first STA attempt within 5 s under normal conditions |
| NF2 | Captive portal page load within 3 s of client associating to Setup_AP on a quiet RF channel |
| NF3 | No dynamic heap allocation in hot-path state transitions (recommended; not blocking for demo) |
| NF4 | Source SHALL compile with `-Wall` or PlatformIO equivalent without warnings in project code |

---

## Phase 2 interface (informational)

Phase 2 SHALL extend `startMainApp()` only:

```
if (!aws_provisioned) runFleetProvisioning();  // claim cert → RegisterThing
connectAwsMqtt();
```

Phase 2 SHALL NOT modify WiFi NVS keys or Provisioning_Mode logic without a versioned migration spec.

---

## Open items (resolve before implementation lock)

| ID | Item | Default if unresolved |
|----|------|------------------------|
| O1 | RGB LED library (FastLED vs NeoPixelBus vs ESP-IDF led_strip) | First that works on GPIO8 |
| O2 | Captive portal: custom AsyncWebServer vs extracted module from C3 hotspot repo | Arduino async pattern |
| O3 | BLE demo PoP printed on serial label vs fixed fleet PoP | Fixed `abcd1234` for demo |
| O4 | Walkthrough in same repo vs `esp32-wifi-provisioning-walkthrough` | Separate repo per jajera convention |

---

## Revision history

| Version | Date | Notes |
|---------|------|-------|
| 0.1 | 2026-07-06 | Initial draft from stakeholder input |
| 0.2 | 2026-07-06 | Review: hardware C61 v2.0, BLE optional profile, NVS keys, reconnect, build envs, demo acceptance |
