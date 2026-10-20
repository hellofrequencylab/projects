#!/usr/bin/env bash
# Flash the build to the CYD over its CH340 USB-serial port.
#   ./flash.sh            app only at 0x10000 — keeps Bruce settings (color, rotation, pins)
#   ./flash.sh --full     whole image at 0x0 — first install, or to recover a bad bootloader
# Optional: PORT=/dev/cu.usbserial-XXXX ./flash.sh
set -euo pipefail
cd "$(dirname "$0")"

PORT="${PORT:-$(ls /dev/cu.usbserial-* /dev/cu.wchusbserial-* 2>/dev/null | head -1 || true)}"
if [[ -z "$PORT" ]]; then
  echo "No CYD serial port found. Use a USB-A to USB-C cable into the CYD's own port." >&2; exit 1
fi

# 460800+ gave serial corruption on this board; 230400 is verified.
ESPTOOL=(.venv/bin/esptool --chip esp32 --port "$PORT" --baud 230400)
if [[ "${1:-}" == "--full" ]]; then
  "${ESPTOOL[@]}" write-flash 0x0 build/full.bin
else
  "${ESPTOOL[@]}" write-flash 0x10000 build/app.bin
fi
