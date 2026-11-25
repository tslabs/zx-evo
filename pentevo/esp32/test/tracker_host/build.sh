#!/bin/bash
# build.sh [src dir] [out name] [cflags...]: host builds of the tracker library part
#   build/<out>   loads through the *_create_context_safe() builders (TRACK_INIT; drv.cpp)
#   build/<out>_s the XM host-stream parser fed in chunks of random size (XM_STREAM_LOAD; sdrv_main.inc)
# The parts of tracker.cpp are cut out at its section comments and function heads.
set -e
H=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "${1:-$H/../../src/main}" && pwd); OUT=${2:-tracker}; shift 2 || true
B=$H/build; mkdir -p "$B/inc"
cp "$SRC/tracker.h" "$SRC/opl.h" "$B/inc/"
T="$SRC/tracker.cpp"
L() { grep -n "$1" "$T" | head -1 | cut -d: -f1; }
s=$(L '^// ESP32 tracker task / CLI / stream integration')
i=$(L '^void initialize_xm()$')
a=$(L '^void xm_host_stream_free_parser_buffers()$')
b=$(L '^void xm_host_stream_set_info(.*)$')
c=$(L '^int xm_host_stream_build_context(.*)$')
d=$(L '^void xm_host_mod_stream_set_info(.*)$')
head -n $((s - 2)) "$T" > "$B/$OUT.lib.cpp"
{ cat "$B/$OUT.lib.cpp"; sed -n "$((s - 1)),$((i - 1))p" "$T"; sed -n "${a},$((b - 1))p" "$T"; sed -n "${c},$((d - 1))p" "$T"; cat "$H/sdrv_main.inc"; } > "$B/$OUT.stream.cpp"
CXX="g++ -std=gnu++17 -g -O1 -w -I$B/inc -I$H/stubs"
$CXX "$@" "$B/$OUT.lib.cpp" "$H/drv.cpp" "$H/opl_stub.cpp" -o "$B/$OUT"
$CXX "$@" "$B/$OUT.stream.cpp" "$H/opl_stub.cpp" -o "$B/${OUT}_s"
