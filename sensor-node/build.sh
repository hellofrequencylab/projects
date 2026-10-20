#!/usr/bin/env bash
# Build the CyberUnicorn sensor node firmware.
#   ./build.sh            build every env in platformio.ini
#   ./build.sh esp32dev   build one env
#   ./build.sh esp32dev upload [--upload-port /dev/cu.xxx]   build and flash
# Output: build/<env>.bin
set -euo pipefail
cd "$(dirname "$0")"

PLATFORMIO_VERSION=6.2.0

# Private PlatformIO core: our pinned platform and toolchain live in .pio-core/,
# never in the shared ~/.platformio (other projects patch files in there).
export PLATFORMIO_CORE_DIR="$PWD/.pio-core"

if [[ ! -x .venv/bin/pio ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -q "platformio==$PLATFORMIO_VERSION"
fi
actual=$(.venv/bin/pio --version | awk '{print $NF}')
if [[ "$actual" != "$PLATFORMIO_VERSION" ]]; then
  echo ".venv has PlatformIO $actual, expected $PLATFORMIO_VERSION. Delete .venv/ and rerun." >&2; exit 1
fi

envs=()
if [[ $# -gt 0 && "$1" != -* ]]; then envs=("$1"); shift; fi
if [[ ${#envs[@]} -eq 0 ]]; then
  envs=($(sed -n 's/^\[env:\(.*\)\]$/\1/p' platformio.ini))
fi

mkdir -p build
for env in "${envs[@]}"; do
  if [[ "${1:-}" == upload ]]; then
    shift
    .venv/bin/pio run -e "$env" -t upload "$@"
  else
    .venv/bin/pio run -e "$env"
  fi
  cp ".pio/build/$env/firmware.bin" "build/$env.bin"
done
ls -l build/*.bin
