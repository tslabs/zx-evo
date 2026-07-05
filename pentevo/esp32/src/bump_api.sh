#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
HDR="${1:-$SCRIPT_DIR/../src/esp_spi_defs.h}"

if [ ! -f "$HDR" ]; then
  echo "bump-api: file not found: $HDR" >&2
  exit 1
fi

old="$(sed -nE 's/^[[:space:]]*#define[[:space:]]+API_VER[[:space:]]+([0-9]+).*/\1/p' "$HDR" | head -n1)"

if [ -z "$old" ]; then
  echo "bump-api: API_VER not found in $HDR" >&2
  exit 1
fi

if [ "$old" -ge 255 ]; then
  echo "bump-api: API_VER is 1 byte and already at maximum 255" >&2
  exit 1
fi

new=$((old + 1))

sed -i -E "s/^([[:space:]]*#define[[:space:]]+API_VER[[:space:]]+)[0-9]+/\1${new}/" "$HDR"

echo "bump-api: API ${old} -> ${new}"
