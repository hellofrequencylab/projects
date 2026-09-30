#!/usr/bin/env bash
# Build Bruce for the CYD + NM-RF-HAT with the boot text from boot.conf.
# Output: build/app.bin (app only, keeps settings) and build/full.bin (whole flash image).
set -euo pipefail
cd "$(dirname "$0")"

BRUCE_TAG=1.16.1
PIO_ENV=CYD-2432S028
PLATFORMIO_VERSION=6.2.0
ESPTOOL_VERSION=5.4.0

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
extra_flags=""
for var in BOOT_CAT_SCENE BOOT_TRADEMARK; do
  val="${!var:-}"
  if [[ "$val" != 0 && "$val" != 1 ]]; then
    echo "boot.conf: $var must be 0 or 1 (got '$val')" >&2; exit 1
  fi
  if [[ "$val" == 1 ]]; then extra_flags+=" -D$var"; fi
done
if [[ "$BOOT_CAT_SCENE" == 1 && ! -f assets/cat_sprite.h ]]; then
  echo "BOOT_CAT_SCENE=1 needs assets/cat_sprite.h" >&2; exit 1
fi

# TRIM_UNUSED_MENUS: 0/1 -> -DNM_TRIM_MENUS
if [[ "${TRIM_UNUSED_MENUS:-0}" != 0 && "${TRIM_UNUSED_MENUS:-0}" != 1 ]]; then
  echo "boot.conf: TRIM_UNUSED_MENUS must be 0 or 1 (got '${TRIM_UNUSED_MENUS:-}')" >&2; exit 1
fi
[[ "${TRIM_UNUSED_MENUS:-0}" == 1 ]] && extra_flags+=" -DNM_TRIM_MENUS"

# STARTUP_APP: empty = normal menu; otherwise must be a known Bruce startup app.
STARTUP_APP="${STARTUP_APP:-}"
if [[ -n "$STARTUP_APP" ]]; then
  valid=("WebUI" "Sniffer" "Brucegotchi" "Wardriving" "WardrivingWifiOnly" \
    "WardrivingBTEOnly" "WardrivingNoRadio" "Custom SubGHz" "PN532 BLE" "PN532 UART" \
    "JS Interpreter" "Clock" "Mass Storage" "GPS Tracker")
  ok=0; for a in "${valid[@]}"; do [[ "$STARTUP_APP" == "$a" ]] && ok=1 && break; done
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
# Our assets (e.g. the boot cat sprite) are untracked in .bruce, so the reset
# above doesn't touch them; recopy so edits in assets/ always land.
if [[ -f assets/cat_sprite.h ]]; then
  mkdir -p .bruce/src/nm_assets
  cp assets/cat_sprite.h .bruce/src/nm_assets/cat_sprite.h
fi
# Show the real version on the boot screen instead of upstream's local "dev".
sed -i '' "s/-DBRUCE_VERSION='\"dev\"'/-DBRUCE_VERSION='\"$BRUCE_TAG-nm\"'/" .bruce/platformio.ini

# Bruce's patch.py shells out to a bare `pio`; without this it half-patches
# libnet80211.a (renames it to .old) and the link fails.
export PATH="$PWD/.venv/bin:$PATH"
export PLATFORMIO_BUILD_FLAGS="-DBOOT_TITLE='\"$BOOT_TITLE\"' -DBOOT_TAGLINE='\"$BOOT_TAGLINE\"'$extra_flags"
(cd .bruce && ../.venv/bin/pio run -e "$PIO_ENV")

mkdir -p build
cp ".bruce/.pio/build/$PIO_ENV/firmware.bin" build/app.bin
cp ".bruce/Bruce-$PIO_ENV.bin" build/full.bin
ls -l build/*.bin
