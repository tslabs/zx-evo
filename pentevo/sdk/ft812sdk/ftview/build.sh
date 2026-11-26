#!/bin/bash
# build.bat for Linux (SDCC 4.2+, python3): ftview.wmf
#  --sdcccall 0: the prebuilt libs of ../lib and the inline asm take the
#    arguments on the stack (SDCC before 4.1.12); newer SDCC passes them in
#    registers by default, and the plugin hung.
#  _HOME: newer SDCC libs put code there; with --code-loc the linker placed it
#    after _DATA (past 0xC000), so it is linked again right after _CODE.
set -e
cd "$(dirname "$0")"
O=obj; rm -rf $O; mkdir -p $O

ihx2bin() {   # Intel HEX -> binary from the lowest address, gaps FF (as hex2bin)
python3 -I - "$1" "$2" <<'PY'
import sys
mem = {}; base = 0
for l in open(sys.argv[1]):
    l = l.strip()
    if not l.startswith(':'): continue
    b = bytes.fromhex(l[1:]); n, a, t = b[0], (b[1] << 8) | b[2], b[3]
    if t == 0:
        for i in range(n): mem[base + a + i] = b[4 + i]
    elif t == 4: base = ((b[4] << 8) | b[5]) << 16
lo, hi = min(mem), max(mem)
open(sys.argv[2], 'wb').write(bytes(mem.get(i, 0xFF) for i in range(lo, hi + 1)))
PY
}

sdasz80 -o $O/crt0.rel src/crt0.s
CC="sdcc -Isrc -I../lib/sdk -I../lib/ft812 -I../lib/tsconf -I../lib/esp32 -I../../../esp32/src/main -mz80 --sdcccall 0 --std-sdcc11 --opt-code-speed --no-std-crt0 $O/crt0.rel --code-loc 0x8003 --data-loc 0xAE00"
LIBS="../lib/ft812/ft812.lib ../lib/tsconf/ts.lib ../lib/sdk/sdk.lib"
$CC src/main.c $LIBS -o $O/out.hex
home=$(awk '/^_CODE /{printf "0x%04X", strtonum("0x"$2) + strtonum("0x"$3); exit}' $O/out.map)
$CC -Wl-b_HOME=$home src/main.c $LIBS -o $O/out.hex
ihx2bin $O/out.hex $O/out.bin

sz=$(( ($(stat -c%s $O/out.bin) - 1) / 512 + 1 ))
sdcc -DN_BLK0=$sz -Isrc -I../lib/sdk -I../lib/ft812 -I../lib/tsconf --no-std-crt0 -mz80 --sdcccall 0 --std-sdcc11 --opt-code-speed src/wc_h.c -o $O/hdr.hex
ihx2bin $O/hdr.hex $O/hdr.bin

cat $O/hdr.bin $O/out.bin > ftview.wmf
ls -la ftview.wmf
