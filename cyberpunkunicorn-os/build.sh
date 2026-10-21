#!/usr/bin/env bash
# Build CyberUnicorn 5000 (Bruce + our patches) for the CYD + NM-RF-HAT.
# Output: build/app.bin (app only, keeps settings) and build/full.bin (whole flash image).
set -euo pipefail
cd "$(dirname "$0")"

BRUCE_TAG=1.16.1
PIO_ENV=CYD-2432S028
PLATFORMIO_VERSION=6.2.0
ESPTOOL_VERSION=5.4.0
OS_NAME="CyberUnicorn 5000"
OS_VERSION=0.1.3

source ./boot.conf
for var in BOOT_TITLE BOOT_TAGLINE; do
  val="${!var}"
  if [[ -z "$val" || ${#val} -gt 26 ]]; then
    echo "boot.conf: $var must be 1-26 characters (got ${#val}): '$val'" >&2; exit 1
  fi
  if [[ "$val" == *[\"\'\\]* ]]; then
    echo "boot.conf: $var must not contain quotes or backslashes: $val" >&2; exit 1
  fi
done
# The unicorn boot scene (apps/boot, patch 0007) is always on.
extra_flags=" -DCYBER_UNICORN_BOOT"

# TRIM_UNUSED_MENUS: 0/1 -> -DNM_TRIM_MENUS
if [[ "${TRIM_UNUSED_MENUS:-0}" != 0 && "${TRIM_UNUSED_MENUS:-0}" != 1 ]]; then
  echo "boot.conf: TRIM_UNUSED_MENUS must be 0 or 1 (got '${TRIM_UNUSED_MENUS:-}')" >&2; exit 1
fi
[[ "${TRIM_UNUSED_MENUS:-0}" == 1 ]] && extra_flags+=" -DNM_TRIM_MENUS"

# TRIM_ABSENT_HARDWARE: 0/1 -> -DCU_TRIM_ABSENT_HW (patch 0017): compiles out LoRa,
# Ethernet, FM and the external NFC gadgets (Chameleon, Amiibolink, PN532 BLE/UART).
if [[ "${TRIM_ABSENT_HARDWARE:-0}" != 0 && "${TRIM_ABSENT_HARDWARE:-0}" != 1 ]]; then
  echo "boot.conf: TRIM_ABSENT_HARDWARE must be 0 or 1 (got '${TRIM_ABSENT_HARDWARE:-}')" >&2; exit 1
fi
[[ "${TRIM_ABSENT_HARDWARE:-0}" == 1 ]] && extra_flags+=" -DCU_TRIM_ABSENT_HW"

# NEVER_DIM: 0/1 -> -DCU_NEVER_DIM: turn the screen dimmer off once per unit at boot
# (apps/hooks.cpp, NVS cyberhw/nodim), so a dim time picked later in Config stays.
if [[ "${NEVER_DIM:-0}" != 0 && "${NEVER_DIM:-0}" != 1 ]]; then
  echo "boot.conf: NEVER_DIM must be 0 or 1 (got '${NEVER_DIM:-}')" >&2; exit 1
fi
[[ "${NEVER_DIM:-0}" == 1 ]] && extra_flags+=" -DCU_NEVER_DIM"

# STARTUP_APP: empty = normal menu; otherwise must be a known Bruce startup app.
STARTUP_APP="${STARTUP_APP:-}"
if [[ -n "$STARTUP_APP" ]]; then
  valid=("WebUI" "Sniffer" "Unicornagotchi" "Wardriving" "WardrivingWifiOnly" \
    "WardrivingBTEOnly" "WardrivingNoRadio" "Custom SubGHz" "PN532 BLE" "PN532 UART" \
    "JS Interpreter" "Clock" "Mass Storage" "GPS Tracker")
  ok=0; for a in "${valid[@]}"; do [[ "$STARTUP_APP" == "$a" ]] && ok=1 && break; done
  if [[ "${TRIM_ABSENT_HARDWARE:-0}" == 1 && ( "$STARTUP_APP" == "PN532 BLE" || "$STARTUP_APP" == "PN532 UART" ) ]]; then
    echo "boot.conf: STARTUP_APP '$STARTUP_APP' is compiled out by TRIM_ABSENT_HARDWARE=1" >&2; exit 1
  fi
  if [[ "$ok" != 1 ]]; then
    echo "boot.conf: STARTUP_APP '$STARTUP_APP' is not a known startup app" >&2; exit 1
  fi
  if [[ "$STARTUP_APP" == *[\"\\]* ]]; then
    echo "boot.conf: STARTUP_APP must not contain quotes or backslashes" >&2; exit 1
  fi
  extra_flags+=" -DNM_STARTUP_APP='\"$STARTUP_APP\"'"
fi

if [[ ! -x .venv/bin/pio ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -q "platformio==$PLATFORMIO_VERSION" "esptool==$ESPTOOL_VERSION"
fi

if [[ ! -d .bruce ]]; then
  git clone --depth 1 --branch "$BRUCE_TAG" https://github.com/BruceDevices/firmware.git .bruce
fi
actual_tag=$(git -C .bruce describe --tags --exact-match 2>/dev/null || echo "?")
if [[ "$actual_tag" != "$BRUCE_TAG" ]]; then
  echo ".bruce is at '$actual_tag', expected $BRUCE_TAG. Delete .bruce/ and rerun." >&2; exit 1
fi

# Reset to pristine upstream, then apply our patches, so reruns are idempotent.
git -C .bruce checkout -q -- .
for p in patches/*.patch; do git -C .bruce apply "../$p"; done
# Our apps: plain source in apps/, compiled as src/cyber_apps/ (Bruce builds
# everything under src/). Recreate it so files deleted from apps/ disappear too.
rm -rf .bruce/src/cyber_apps
cp -R apps .bruce/src/cyber_apps

# Report the real Bruce version instead of upstream's local "dev"; our own
# name/version go in OS_NAME/OS_VERSION (patch 0005).
# (-i.bak + rm: the one in-place form both BSD sed on macOS and GNU sed on Linux CI accept.)
sed -i.bak "s/-DBRUCE_VERSION='\"dev\"'/-DBRUCE_VERSION='\"$BRUCE_TAG\"'/" .bruce/platformio.ini
rm -f .bruce/platformio.ini.bak

# The commit this build came from, reported by system.status and the boot hello.
# "-dirty" when apps/, patches/ or this script have uncommitted changes.
FW_COMMIT=$(git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
if [[ "$FW_COMMIT" != unknown ]] && ! git diff --quiet HEAD -- apps patches build.sh boot.conf 2>/dev/null; then
  FW_COMMIT+="-dirty"
fi

# Bruce's patch.py shells out to a bare `pio`; without this it half-patches
# libnet80211.a (renames it to .old) and the link fails.
export PATH="$PWD/.venv/bin:$PATH"
export PLATFORMIO_BUILD_FLAGS="-DOS_NAME='\"$OS_NAME\"' -DOS_VERSION='\"$OS_VERSION\"' -DFW_COMMIT='\"$FW_COMMIT\"' -DBOOT_TITLE='\"$BOOT_TITLE\"' -DBOOT_TAGLINE='\"$BOOT_TAGLINE\"'$extra_flags"
# One build at a time per machine: Bruce's patch.py edits libnet80211.a in the
# shared ~/.platformio cache, so parallel builds (e.g. several worktrees) corrupt it.
LOCK="$HOME/.platformio/.cyberunicorn-build.lock" # next to the shared cache it protects
mkdir -p "$(dirname "$LOCK")" # a fresh machine (or CI runner) has no ~/.platformio yet
until mkdir "$LOCK" 2>/dev/null; do
  echo "Another CyberUnicorn build holds $LOCK (pid $(cat "$LOCK/pid" 2>/dev/null)); waiting..." >&2
  if [[ -f "$LOCK/pid" ]] && ! kill -0 "$(cat "$LOCK/pid")" 2>/dev/null; then rm -rf "$LOCK"; continue; fi
  sleep 20
done
echo $$ > "$LOCK/pid"
trap 'rm -rf "$LOCK"' EXIT
(cd .bruce && ../.venv/bin/pio run -e "$PIO_ENV")

mkdir -p build
cp ".bruce/.pio/build/$PIO_ENV/firmware.bin" build/app.bin
cp ".bruce/Bruce-$PIO_ENV.bin" build/full.bin

# What the website's updater reads (plan Phase 6): which build this is and how to
# check it arrived whole. The updater flashes app.bin at 0x10000 only, like
# ./flash.sh without --full, so NVS (scanner key, Wi-Fi) survives.
python3 - "$OS_VERSION" "$FW_COMMIT" "$BRUCE_TAG" <<'PY'
import hashlib, json, os, sys
version, commit, bruce = sys.argv[1:4]
app = open("build/app.bin", "rb").read()
json.dump({
    "board": "cyd-2432s028r",
    "os": "CyberUnicorn 5000",
    "version": version,
    "commit": commit,
    "bruce_version": bruce,
    "proto": 1,
    "app": {"file": "app.bin", "offset": 0x10000, "size": len(app),
            "sha256": hashlib.sha256(app).hexdigest()},
    "app_partition_size": 0x3C0000,
}, open("build/manifest.json", "w"), indent=2)
PY
ls -l build/*.bin build/manifest.json
