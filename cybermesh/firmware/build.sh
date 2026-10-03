#!/usr/bin/env bash
# Build (and optionally flash) the box firmware for one chip.
#   ./build.sh esp32                       # CYD (nm-rf-hat board)
#   ./build.sh esp32s2                     # AIO Board
#   ./build.sh esp32 -p /dev/cu.X flash monitor
# Needs ESP-IDF at the version pinned in ../CLAUDE.md, with export.sh sourced
# (or IDF_PATH set; this script sources it for you).
set -euo pipefail
cd "$(dirname "$0")"

IDF_VERSION=v6.0.3
target="${1:-}"
case "$target" in
  esp32|esp32s2|esp32s3|esp32c3|esp32c6) shift ;;
  *) echo "usage: $0 <esp32|esp32s2|esp32s3|esp32c3|esp32c6> [idf.py args, default: build]" >&2; exit 2 ;;
esac

if ! command -v idf.py >/dev/null; then
  : "${IDF_PATH:?set IDF_PATH to an ESP-IDF $IDF_VERSION checkout, or source its export.sh}"
  # shellcheck disable=SC1091
  source "$IDF_PATH/export.sh" >/dev/null
fi
have="$(idf.py --version 2>/dev/null | awk '{print $2}')"
if [[ "$have" != "$IDF_VERSION" ]]; then
  echo "ESP-IDF is $have; this project is pinned to $IDF_VERSION" >&2; exit 1
fi

out="build/$target"
idf.py -B "$out" -D SDKCONFIG="$out/sdkconfig" -D IDF_TARGET="$target" "${@:-build}"
