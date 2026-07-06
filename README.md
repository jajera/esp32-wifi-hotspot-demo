# esp32-wifi-hotspot-demo

Phase 1 **first-boot WiFi onboarding** for ESP32-C61-DevKitC-1 v2.0.

- **Default build:** SoftAP + captive portal (`ESP-Setup-XXXX` → browser form at `192.168.4.1`)
- **Optional build:** BLE provisioning via Espressif **ESP BLE Prov** app

Credentials persist in NVS. Phase 2 (AWS IoT fleet provisioning) is not implemented yet.

**Full walkthrough (flash script, code tour, IoT path):** [docs/walkthrough.md](docs/walkthrough.md)

## Quick start (no build)

Download a [release](https://github.com/jajera/esp32-wifi-hotspot-demo/releases) and flash:

```bash
./scripts/flash-softap.sh v1.0.0-phase1
```

Then follow WiFi setup in [docs/walkthrough.md#wifi-setup-installer](docs/walkthrough.md#wifi-setup-installer).

## Hardware

| Item | Value |
|------|--------|
| Board | ESP32-C61-DevKitC-1 v2.0 |
| USB | **UART** Type-C port (for flash + serial) |
| LED | RGB on GPIO8 — blue=setup, yellow=connecting, green=connected, red=failed setup |
| Factory reset | Hold **Boot** 5 seconds |

**2.4 GHz WiFi only.** 5 GHz-only networks are not supported.

## Build from source (developers)

Requires [PlatformIO](https://platformio.org/). ESP32-C61 uses the [pioarduino](https://github.com/pioarduino/platform-espressif32) platform.

```bash
pio run -e esp32-c61-softap -t upload
pio run -e esp32-c61-ble -t upload
pio device monitor -b 115200
pio test -e native
```

Board target: `esp32-c61-devkitc1-n8r2`.

## Releases

Push a tag to publish firmware automatically:

```bash
git tag v1.0.0-phase1
git push origin v1.0.0-phase1
```

GitHub Actions builds and attaches `esp32-c61-softap.factory.bin` (and BLE variant) to the release.

## Project layout

```text
src/              Firmware
docs/             Walkthrough and guides
scripts/          flash-softap.sh (release binary)
.github/workflows CI + release on tag
.kiro/specs/      Requirements, design, tasks
```

## Phase 2 (planned)

AWS IoT fleet provisioning in `startMainApp()` only — no changes to captive portal / WiFi NVS keys.
