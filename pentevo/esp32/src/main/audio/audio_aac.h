// AAC (ADTS) and M4A decoder of the audio stream player: esp_audio_codec of
// Espressif (a binary library, ESP chips only; off on the host: AUDIO_AAC 0).
#pragma once

#include "audio_core.h"

#ifndef AUDIO_AAC
#ifdef ESP_PLATFORM
#define AUDIO_AAC 1
#else
#define AUDIO_AAC 0
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aud_aac aud_aac_t;

// Reads up to the first frame (blocks); sets rate, channels, bitrate of p.
// m4a: an MP4 file (its moov before mdat), else ADTS frames. NULL: not decodable.
aud_aac_t *aud_aac_open(aud_io_t *io, int m4a, aud_params_t *p);
// Interleaved int16 frames of the source channels; 0 at the end (or abort)
size_t aud_aac_read(aud_aac_t *a, int16_t *out, size_t frames);
uint32_t aud_aac_bitrate(aud_aac_t *a);
void aud_aac_close(aud_aac_t *a);

#ifdef __cplusplus
}
#endif
