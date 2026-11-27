#!/bin/bash
# run_tests.sh [src dir]: main/audio/audio_core.c on the host with ASan + UBSan
#  - every stream of gen.sh decoded in chunks of random size (1..3000, and 1..3 bytes over the first 96 KB)
#    against ffmpeg (lossless: max diff <= 2, lossy: SNR >= 40 dB after alignment)
#  - flush at 40 %, the stream goes on at 60 % (WAV / MP3 / FLAC; Ogg refuses)
#  - garbage as every format, the resampler
H=$(cd "$(dirname "$0")" && pwd); cd "$H"
SRC=${1:-$H/../../src/main/audio}
mkdir -p build
# the decoders are 1.3 MB of C: built once (~40 s), then again on a change
if [ ! -x build/audio_test ] || [ -n "$(find "$SRC" audio_test.c -newer build/audio_test -name '*.[ch]' | head -1)" ]; then
  gcc -std=gnu11 -g -O1 -Wall -I"$SRC" -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$SRC/audio_core.c" audio_test.c -o build/audio_test -lm -lpthread 2>&1 | grep -v third_party | grep -E "error|warning:" && exit 1
fi
[ -f build/streams/garbage.bin ] || ./gen.sh build/streams || exit 1
cd build/streams
err=0
run() { r=$(timeout 120 ../audio_test "$@" 2>&1); rc=$?; if [ $rc != 0 ] || echo "$r" | grep -q -E "FAIL|runtime error|Sanitizer"; then echo "FAIL $*"; echo "$r" | grep -E "FAIL|runtime error|Sanitizer|failed|SUMMARY" | head -5; err=1; else echo "ok   $* | $(echo "$r" | grep -E '^(decode|after|resume)' | tr '\n' ' ' | cut -c1-110)"; fi; }
for f in w16.wav w8.wav w24.wav wf32.wav wmono.wav walaw.wav f16.flac f24.flac fmono.flac m44.mp3 m48.mp3 mmono.mp3 mcover.mp3 mraw.mp3 o44.ogg o48.ogg; do
  case $f in *.mp3|*.ogg) k=lossy;; *) k=lossless;; esac
  run dec $f $f.ref 3000 1 $k
  run dec $f $f.ref 3 2 $k   # 1..3 bytes for the first 96 KB
  run flush $f $f.ref 3000 3 $k
done
for fmt in 0 1 2 3 4; do run junk garbage.bin $fmt; done
run junk m44.mp3 4
run rs
[ $err = 0 ] && echo "audio_host: 0 errors" || echo "audio_host: ERRORS"
exit $err
