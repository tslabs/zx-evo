#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
HDR="${1:-$SCRIPT_DIR/../src/esp_spi_defs.h}"

if [ ! -f "$HDR" ]; then
  echo "bump-feat: file not found: $HDR" >&2
  exit 1
fi

old="$(sed -nE 's/^[[:space:]]*#define[[:space:]]+FEAT_VER[[:space:]]+([0-9]+).*/\1/p' "$HDR" | head -n1)"

if [ -z "$old" ]; then
  echo "bump-feat: FEAT_VER not found in $HDR" >&2
  exit 1
fi

if [ "$old" -ge 65535 ]; then
  echo "bump-feat: FEAT_VER is 2 bytes and already at maximum 65535" >&2
  exit 1
fi

new=$((old + 1))

sed -i -E "s/^([[:space:]]*#define[[:space:]]+FEAT_VER[[:space:]]+)[0-9]+/\1${new}/" "$HDR"

echo "bump-feat: FEAT ${old} -> ${new}"
