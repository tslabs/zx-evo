#!/bin/bash
# run_tests.sh [src dir]: the ICY parser of main/audio_radio.cpp on the host with ASan + UBSan (seconds)
H=$(cd "$(dirname "$0")" && pwd); cd "$H"
SRC=${1:-$H/../../src/main}
mkdir -p build/src
# its headers include "main.h" from their own directory: copies next to the stub one
cp "$SRC/audio_radio.cpp" "$SRC/audio_radio.h" "$SRC/audio_stream.h" "$SRC/esp_spi_defs.h" build/src/
cp stubs/main.h build/src/
g++ -std=gnu++17 -g -O1 -Wall -Wno-unused-function -Ibuild/src -Istubs -fsanitize=address,undefined -fno-omit-frame-pointer \
  radio_test.cpp -o build/radio_test || exit 1
./build/radio_test
