# Decoders of the audio stream player

Single-file libraries, included by `../audio_core.c` (not compiled on their own).

| File | Source | Licence |
|---|---|---|
| `dr_mp3.h` | [mackron/dr_libs](https://github.com/mackron/dr_libs) tag `mp3-0.7.4` (based on minimp3) | public domain (Unlicense) or MIT-0 |
| `dr_flac.h` | dr_libs tag `flac-0.13.4` | public domain (Unlicense) or MIT-0 |
| `dr_wav.h` | dr_libs tag `wav-0.14.6` | public domain (Unlicense) or MIT-0 |
| `stb_vorbis.h` | [nothings/stb](https://github.com/nothings/stb) `stb_vorbis.c` at 6b5799b (v1.22), renamed so the build glob does not compile it alone | public domain or MIT |

Unchanged copies; update them by replacing the files.
