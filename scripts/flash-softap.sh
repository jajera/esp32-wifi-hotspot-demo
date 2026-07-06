#!/usr/bin/env bash
# Flash pre-built SoftAP firmware from a GitHub Release (no PlatformIO required).
set -euo pipefail

usage() {
  echo "Usage: $0 <release-tag> [serial-port]"
  echo "Example: $0 v1.0.0-phase1 /dev/ttyUSB0"
  echo ""
  echo "Environment:"
  echo "  GITHUB_REPO   GitHub repo (default: auto-detect from git remote or jajera/esp32-wifi-hotspot-demo)"
  echo "  PORT          Serial port if not passed as second argument"
  exit 1
}

[[ $# -ge 1 ]] || usage

TAG="$1"
if [[ ! "$TAG" =~ ^v ]]; then
  TAG="v${TAG}"
fi

PORT="${2:-${PORT:-}}"
if [[ -z "$PORT" ]]; then
  if [[ -e /dev/ttyUSB0 ]]; then
    PORT=/dev/ttyUSB0
  elif [[ -e /dev/ttyACM0 ]]; then
    PORT=/dev/ttyACM0
  else
    echo "No default serial port found. Pass port as second argument or set PORT."
    exit 1
  fi
fi

if [[ -z "${GITHUB_REPO:-}" ]]; then
  if git remote get-url origin &>/dev/null; then
    GITHUB_REPO="$(git remote get-url origin | sed -E 's#.*github.com[:/](.+)(\.git)?$#\1#')"
  else
    GITHUB_REPO="jajera/esp32-wifi-hotspot-demo"
  fi
fi

ARTIFACT="esp32-c61-softap.factory.bin"
BASE_URL="https://github.com/${GITHUB_REPO}/releases/download/${TAG}"
TMP_DIR="$(mktemp -d)"
BIN="${TMP_DIR}/${ARTIFACT}"
CHECKSUMS="${TMP_DIR}/SHA256SUMS"

cleanup() { rm -rf "$TMP_DIR"; }
trap cleanup EXIT

echo "Downloading ${BASE_URL}/${ARTIFACT}"
curl -fsSL "${BASE_URL}/${ARTIFACT}" -o "$BIN"

if curl -fsSL "${BASE_URL}/SHA256SUMS" -o "$CHECKSUMS" 2>/dev/null; then
  echo "Verifying SHA256..."
  (cd "$TMP_DIR" && sha256sum -c <(grep "$ARTIFACT" SHA256SUMS))
else
  echo "Warning: SHA256SUMS not found on release; skipping checksum verify."
fi

if ! command -v esptool.py &>/dev/null; then
  echo "Installing esptool..."
  python3 -m pip install --user esptool
  export PATH="${HOME}/.local/bin:${PATH}"
fi

echo "Flashing ${PORT} (chip esp32c61)..."
esptool.py --chip esp32c61 -p "$PORT" -b 460800 write_flash 0x0 "$BIN"

echo "Done. Open serial monitor: pio device monitor -b 115200  (or screen ${PORT} 115200)"
echo "Then follow WiFi setup in docs/walkthrough.md"
