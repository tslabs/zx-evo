#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
HDR="${1:-$SCRIPT_DIR/../src/esp_spi_defs.h}"

if [ ! -f "$HDR" ]; then
  echo "bump: file not found: $HDR" >&2
  exit 1
fi

set -- $(sed -nE 's/^[[:space:]]*#define[[:space:]]+PROD_VER([0-2])[[:space:]]+([0-9]+).*/\1 \2/p' "$HDR" | sort | awk '{print $2}')

if [ "$#" -ne 3 ]; then
  echo "bump: PROD_VER0..2 not found in $HDR" >&2
  exit 1
fi

major="$1"
minor="$2"
old="$3"
new=$((old + 1))

sed -i -E "s/^([[:space:]]*#define[[:space:]]+PROD_VER2[[:space:]]+)[0-9]+/\1${new}/" "$HDR"

echo "bump: ver.${major}.${minor}.${old} -> ver.${major}.${minor}.${new}"
