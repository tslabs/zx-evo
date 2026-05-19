#!/bin/sh
set -eu

HDR="$1"

set -- $(sed -nE '/CP_STRING/ s/.*ver\.([0-9]+)\.([0-9]+)\.([0-9]+).*/\1 \2 \3/p' "$HDR" | head -n1)

major="${1:-0}"
minor="${2:-0}"
old="${3:-0}"
new=$((old + 1))

sed -i -E "/CP_STRING/ s/ver\.${major}\.${minor}\.${old}/ver.${major}.${minor}.${new}/" "$HDR"

echo "bump: ver.${major}.${minor}.${old} -> ver.${major}.${minor}.${new}"