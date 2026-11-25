# Tracker loaders on the host

The library part of `src/main/tracker.cpp` (loaders, player) and the XM host-stream parser
built for Linux with `g++`, ASan and UBSan, with stubs of the ESP-IDF headers.

- `run_tests.sh [src dir] [module dir]`: builds, generates the damaged modules of
  `gen_bad.py` (one per loader bound: song length, rows, pattern data, envelope points,
  sustain / loop points, sample loops, a 16-bit sample of odd byte length, a 16-bit
  sample at an odd address) and runs them on both load paths; with a directory of
  modules also these (no sanitizer finding, no hang, the XM sound the same on both
  paths). Prints `tracker_host: 0 errors`. Seconds.
- `build.sh [src dir] [name] [cflags]`: `build/<name>` loads a module as `TRACK_INIT`
  does (`xm_ / mod_ / s3m_create_context_safe`), `build/<name>_s` feeds an XM to
  `xm_host_stream_build_context()` in chunks of random size as `XM_STREAM_LOAD`;
  both render 44.1 kHz and print a hash of the sound: `<bin> <module> [seconds] [seed]`.
- `xm_scan.py <xm>...`: the header values of an XM which the loaders bound.
