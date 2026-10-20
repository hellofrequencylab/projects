#!/usr/bin/env bash
# Mirror the CYD's screen to a browser on this Mac and drive it over USB serial.
#   ./remote.sh                 auto-detects the CH340 port, opens http://localhost:8765
# Optional: PORT=/dev/cu.usbserial-XXXX ./remote.sh   ·   ./remote.sh --http 9000
# Note: connecting reboots the board (the CH340 resets the ESP32 on port open).
set -euo pipefail
cd "$(dirname "$0")"
if [[ ! -x .venv/bin/python ]] || ! .venv/bin/python -c 'import serial' 2>/dev/null; then
  echo "No .venv with pyserial yet — run ./build.sh once (it installs esptool, which brings pyserial)." >&2
  exit 1
fi
exec .venv/bin/python remote/bruce_remote.py "$@"
