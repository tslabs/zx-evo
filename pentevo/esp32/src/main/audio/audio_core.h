// Audio stream player: the parts without FreeRTOS (rings, decoders, resampler),
// also built on the host by test/audio_host.
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Formats (ESP_REG_AUDIO_FORMAT)
enum
{
  AUD_FMT_AUTO = 0,   // from the first bytes of the stream
  AUD_FMT_WAV  = 1,   // RIFF WAVE: PCM 8/16/24/32, float, A-law, u-law
  AUD_FMT_MP3  = 2,   // MPEG-1/2/2.5 Layer III (also I/II), ID3v2 skipped
  AUD_FMT_OGG  = 3,   // Ogg Vorbis
  AUD_FMT_FLAC = 4,   // native FLAC
};

// Single producer / single consumer byte ring; size is a power of 2.
// head and tail run freely (wrap at 2^32), used = head - tail.
typedef struct
{
  uint8_t *buf;
  uint32_t size;
  volatile uint32_t head;   // written by the producer only
  volatile uint32_t tail;   // written by the consumer only
} aud_ring_t;

void   aud_ring_init(aud_ring_t *r, void *buf, uint32_t size);
void   aud_ring_reset(aud_ring_t *r);   // both sides stopped
size_t aud_ring_used(const aud_ring_t *r);
size_t aud_ring_free(const aud_ring_t *r);
size_t aud_ring_write(aud_ring_t *r, const void *src, size_t n);   // up to free
size_t aud_ring_read(aud_ring_t *r, void *dst, size_t n);          // up to used
size_t aud_ring_peek(const aud_ring_t *r, void *dst, size_t n);    // no consume

// Input of a decoder: the byte ring of the host data. Reads block (wait) until
// the bytes are there, the host ended the stream (end) or the player aborts.
typedef struct
{
  aud_ring_t *in;
  volatile int *end;          // no more host data
  volatile int *abort;        // stop / flush: the decoder gives up
  void (*wait)(void *arg);    // sleep until more data (a few ms at most)
  void *wait_arg;
  uint64_t pos;               // bytes consumed by the decoder
} aud_io_t;

size_t aud_io_read(aud_io_t *io, void *dst, size_t n);        // all n, or less at end / abort
size_t aud_io_read_some(aud_io_t *io, void *dst, size_t n);   // >= 1, or 0 at end / abort

// Parameters of the stream, kept over a flush (a WAV continues as raw PCM)
typedef struct
{
  uint32_t rate;
  uint8_t  channels;
  uint8_t  bits;            // of the source samples (0: compressed)
  uint16_t block_align;     // WAV: bytes per frame
  uint16_t wav_format;      // WAV: 1 PCM, 3 float, 6 A-law, 7 u-law
  uint64_t data_offset;     // WAV: offset of the sample data in the file
  uint64_t total_frames;    // 0: not known
} aud_params_t;

// Format from the first bytes: AUD_FMT_*, -1 = need more bytes, 0 = unknown
int aud_sniff(const uint8_t *p, size_t n);

typedef struct aud_dec aud_dec_t;

// Opens a decoder: reads the headers from io (blocks). resume = after a flush:
// the stream continues at an offset chosen by the host (MP3 / FLAC find their
// frames, WAV goes on as raw PCM with prev). NULL: not decodable / aborted.
aud_dec_t *aud_dec_open(int fmt, aud_io_t *io, int resume, const aud_params_t *prev);
const aud_params_t *aud_dec_params(const aud_dec_t *d);
int  aud_dec_seekable(int fmt);   // the host may flush and continue at an offset
// Decodes up to frames stereo int16 frames (mono doubled, >2 channels: the
// first two). Returns frames, 0 at the end of the stream (or abort).
size_t aud_dec_read(aud_dec_t *d, int16_t *out, size_t frames);
void aud_dec_close(aud_dec_t *d);

// Linear-interpolation resampler from the PCM ring (stereo int16 frames, 4
// bytes each) to the mix rate. Source rate = mix rate: the samples as they are.
typedef struct
{
  uint32_t src_rate;
  float    t;              // position between f0 and f1, 0..1
  int16_t  f0[2], f1[2];
  int      primed;         // f0 / f1 valid
  uint64_t played;         // source frames consumed
} aud_rs_t;

void aud_rs_reset(aud_rs_t *rs, uint32_t src_rate);
// Adds up to n frames, scaled by gain (int16 full scale -> gain), to mix
// (stereo float). Returns the frames made; < n: the PCM ring ran empty.
size_t aud_rs_render(aud_rs_t *rs, aud_ring_t *pcm, float *mix, size_t n, uint32_t out_rate, float gain);

#ifdef __cplusplus
}
#endif
