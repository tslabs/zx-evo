// Audio stream player: rings, decoders (dr_wav, dr_mp3, dr_flac, stb_vorbis),
// resampler. No FreeRTOS here: audio_stream.cpp gives the waits, the host test
// (test/audio_host) builds this file as it is.

#include "audio_core.h"
#include "audio_aac.h"

#include <string.h>
#include <stdlib.h>

#ifndef AUDIO_WAV
#define AUDIO_WAV  1
#endif
#ifndef AUDIO_MP3
#define AUDIO_MP3  1
#endif
#ifndef AUDIO_FLAC
#define AUDIO_FLAC 1
#endif
#ifndef AUDIO_OGG
#define AUDIO_OGG  1
#endif

#if AUDIO_WAV
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_WCHAR
#include "third_party/dr_wav.h"
#endif

#if AUDIO_MP3
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "third_party/dr_mp3.h"
#endif

#if AUDIO_FLAC
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_WCHAR
#define DR_FLAC_NO_OGG
#include "third_party/dr_flac.h"
#endif

#if AUDIO_OGG
#define STB_VORBIS_NO_PULLDATA_API
#define STB_VORBIS_NO_STDIO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "third_party/stb_vorbis.h"
#pragma GCC diagnostic pop
#endif

// ------------- Ring

void aud_ring_init(aud_ring_t *r, void *buf, uint32_t size)
{
  r->buf = (uint8_t*)buf;
  r->size = size;
  r->head = r->tail = 0;
}

void aud_ring_reset(aud_ring_t *r)
{
  r->head = r->tail = 0;
}

size_t aud_ring_used(const aud_ring_t *r)
{
  uint32_t h = __atomic_load_n(&r->head, __ATOMIC_ACQUIRE);
  uint32_t t = __atomic_load_n(&r->tail, __ATOMIC_ACQUIRE);
  return (uint32_t)(h - t);
}

size_t aud_ring_free(const aud_ring_t *r)
{
  return r->size - aud_ring_used(r);
}

size_t aud_ring_write(aud_ring_t *r, const void *src, size_t n)
{
  size_t f = aud_ring_free(r);
  if (n > f) n = f;
  if (!n) return 0;

  uint32_t h = r->head;
  uint32_t o = h & (r->size - 1);
  size_t a = r->size - o;
  if (a > n) a = n;
  memcpy(r->buf + o, src, a);
  if (n > a) memcpy(r->buf, (const uint8_t*)src + a, n - a);
  __atomic_store_n(&r->head, h + (uint32_t)n, __ATOMIC_RELEASE);
  return n;
}

size_t aud_ring_peek(const aud_ring_t *r, void *dst, size_t n)
{
  size_t u = aud_ring_used(r);
  if (n > u) n = u;
  if (!n) return 0;

  uint32_t o = r->tail & (r->size - 1);
  size_t a = r->size - o;
  if (a > n) a = n;
  memcpy(dst, r->buf + o, a);
  if (n > a) memcpy((uint8_t*)dst + a, r->buf, n - a);
  return n;
}

size_t aud_ring_read(aud_ring_t *r, void *dst, size_t n)
{
  n = aud_ring_peek(r, dst, n);
  if (n) __atomic_store_n(&r->tail, r->tail + (uint32_t)n, __ATOMIC_RELEASE);
  return n;
}

// ------------- Decoder input

size_t aud_io_read_some(aud_io_t *io, void *dst, size_t n)
{
  while (n)
  {
    size_t got = aud_ring_read(io->in, dst, n);
    if (got)
    {
      io->pos += got;
      return got;
    }
    if (*io->abort) return 0;
    // end: data written before the end flag is in the ring, look once more
    if (*io->end && !aud_ring_used(io->in)) return 0;
    io->wait(io->wait_arg);
  }
  return 0;
}

size_t aud_io_read(aud_io_t *io, void *dst, size_t n)
{
  size_t done = 0;
  while (done < n)
  {
    size_t got = aud_io_read_some(io, (uint8_t*)dst + done, n - done);
    if (!got) break;
    done += got;
  }
  return done;
}

// Seeks of the decoders: forward only (skipping a tag or a chunk); from the
// start (dr_wav goes to the data chunk so) only to here or further on
static int io_skip(aud_io_t *io, int offset);

static int io_seek(aud_io_t *io, int offset, int from_start)
{
  if (from_start)
  {
    if ((uint64_t)offset < io->pos) return 0;
    offset = (int)((uint64_t)offset - io->pos);
  }
  return io_skip(io, offset);
}

static int io_skip(aud_io_t *io, int offset)
{
  uint8_t tmp[256];
  if (offset < 0) return 0;
  while (offset > 0)
  {
    size_t n = offset > (int)sizeof(tmp) ? sizeof(tmp) : (size_t)offset;
    if (aud_io_read(io, tmp, n) != n) return 0;
    offset -= (int)n;
  }
  return 1;
}

// ------------- Format

int aud_sniff(const uint8_t *p, size_t n)
{
  if (n < 12) return -1;
  if (!memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4)) return AUD_FMT_WAV;
  if (!memcmp(p, "OggS", 4)) return AUD_FMT_OGG;
  if (!memcmp(p, "fLaC", 4)) return AUD_FMT_FLAC;
  if (!memcmp(p, "ID3", 3)) return AUD_FMT_MP3;
  if (!memcmp(p + 4, "ftyp", 4)) return AUD_FMT_M4A;
  // ADTS: the sync of 12 ones, layer 00
  if (p[0] == 0xFF && (p[1] & 0xF6) == 0xF0) return AUD_FMT_AAC;
  // MPEG audio frame sync: 11 ones, a layer (not 00)
  if (p[0] == 0xFF && (p[1] & 0xE0) == 0xE0 && (p[1] & 0x06)) return AUD_FMT_MP3;
  return 0;
}

int aud_dec_seekable(int fmt)
{
  return fmt == AUD_FMT_WAV || fmt == AUD_FMT_MP3 || fmt == AUD_FMT_FLAC || (AUDIO_AAC && fmt == AUD_FMT_AAC);
}

// ------------- Decoders

#define DEC_CHUNK   1024   // frames per decoder call
#define DEC_MAXCH   8
#define OGG_BUF     (72 * 1024)   // > one Ogg page (65307 bytes at most)

struct aud_dec
{
  int fmt;
  aud_io_t *io;
  aud_params_t p;
  int raw;                       // WAV after a flush: raw sample data
  int16_t *tmp;                  // DEC_CHUNK frames of p.channels samples
#if AUDIO_WAV
  drwav wav;
  uint8_t *rawbuf;
#endif
#if AUDIO_MP3
  drmp3 mp3;
#endif
#if AUDIO_FLAC
  drflac *flac;
#endif
#if AUDIO_AAC
  aud_aac_t *aac;
#endif
#if AUDIO_OGG
  stb_vorbis *vorbis;
  uint8_t *obuf;
  size_t olen;
  float **opcm;
  int opcm_n, opcm_pos;
#endif
};

static size_t cb_read(void *ud, void *buf, size_t n)
{
  return aud_io_read((aud_io_t*)ud, buf, n);
}

#if AUDIO_WAV
static drwav_bool32 cb_seek_wav(void *ud, int offset, drwav_seek_origin origin)
{
  return (origin == DRWAV_SEEK_CUR || origin == DRWAV_SEEK_SET) && io_seek((aud_io_t*)ud, offset, origin == DRWAV_SEEK_SET);
}
#endif
#if AUDIO_FLAC
static drflac_bool32 cb_seek_flac(void *ud, int offset, drflac_seek_origin origin)
{
  return (origin == DRFLAC_SEEK_CUR || origin == DRFLAC_SEEK_SET) && io_seek((aud_io_t*)ud, offset, origin == DRFLAC_SEEK_SET);
}
#endif

#if AUDIO_OGG
// Appends host data to the Ogg buffer; 0: end of stream (or full)
static size_t ogg_fill(aud_dec_t *d)
{
  if (d->olen >= OGG_BUF) return 0;
  size_t got = aud_io_read_some(d->io, d->obuf + d->olen, OGG_BUF - d->olen);
  d->olen += got;
  return got;
}

static void ogg_consume(aud_dec_t *d, size_t n)
{
  memmove(d->obuf, d->obuf + n, d->olen - n);
  d->olen -= n;
}

static int ogg_open(aud_dec_t *d)
{
  d->obuf = (uint8_t*)malloc(OGG_BUF);
  if (!d->obuf) return 0;

  while (1)
  {
    int used = 0, err = 0;
    d->vorbis = stb_vorbis_open_pushdata(d->obuf, (int)d->olen, &used, &err, NULL);
    if (d->vorbis)
    {
      ogg_consume(d, (size_t)used);
      break;
    }
    if (err != VORBIS_need_more_data || !ogg_fill(d)) return 0;
  }

  stb_vorbis_info info = stb_vorbis_get_info(d->vorbis);
  d->p.rate = info.sample_rate;
  d->p.channels = (uint8_t)info.channels;
  return 1;
}

static size_t ogg_read(aud_dec_t *d, int16_t *out, size_t frames)
{
  size_t done = 0;
  int ch = d->p.channels;

  while (done < frames)
  {
    if (d->opcm_pos < d->opcm_n)
    {
      size_t n = (size_t)(d->opcm_n - d->opcm_pos);
      if (n > frames - done) n = frames - done;
      for (size_t i = 0; i < n; i++)
        for (int c = 0; c < ch; c++)
        {
          float v = d->opcm[c][d->opcm_pos + i] * 32768.f;
          if (v > 32767.f) v = 32767.f;
          if (v < -32768.f) v = -32768.f;
          out[(done + i) * ch + c] = (int16_t)v;
        }
      d->opcm_pos += (int)n;
      done += n;
      continue;
    }

    int channels = 0, samples = 0;
    int used = stb_vorbis_decode_frame_pushdata(d->vorbis, d->obuf, (int)d->olen, &channels, &d->opcm, &samples);
    if (used == 0 && samples == 0)
    {
      // a page is not complete: more data, or the end
      if (!ogg_fill(d)) break;
      continue;
    }
    ogg_consume(d, (size_t)used);
    d->opcm_n = samples;
    d->opcm_pos = 0;
  }
  return done;
}
#endif

aud_dec_t *aud_dec_open(int fmt, aud_io_t *io, int resume, const aud_params_t *prev, uint64_t at)
{
  aud_dec_t *d = (aud_dec_t*)calloc(1, sizeof(aud_dec_t));
  if (!d) return NULL;
  d->fmt = fmt;
  d->io = io;
  io->pos = 0;   // the stream of this decoder starts here (after a flush: at the host's offset)
  int ok = 0;

  switch (fmt)
  {
#if AUDIO_WAV
    case AUD_FMT_WAV:
      if (resume)
      {
        // the samples go on at the next frame after at
        if (!prev || !prev->block_align || !prev->channels || prev->channels > DEC_MAXCH) break;
        d->p = *prev;
        d->raw = 1;
        d->rawbuf = (uint8_t*)malloc((size_t)DEC_CHUNK * prev->block_align);
        ok = d->rawbuf != NULL;
        if (ok && at > prev->data_offset)
        {
          uint32_t part = (uint32_t)((at - prev->data_offset) % prev->block_align);
          if (part && !io_skip(io, (int)(prev->block_align - part))) ok = 0;
        }
        break;
      }
      if (!drwav_init(&d->wav, cb_read, cb_seek_wav, NULL, io, NULL)) break;
      d->p.rate = d->wav.sampleRate;
      d->p.channels = (uint8_t)d->wav.channels;
      d->p.bits = (uint8_t)d->wav.bitsPerSample;
      d->p.block_align = d->wav.fmt.blockAlign;
      d->p.wav_format = d->wav.translatedFormatTag;
      d->p.data_offset = d->wav.dataChunkDataPos;
      d->p.total_frames = d->wav.totalPCMFrameCount;
      ok = 1;
      break;
#endif
#if AUDIO_MP3
    case AUD_FMT_MP3:
      // no seek: without ID3v2 dr_mp3 would seek back to the start, with
      // none it keeps the bytes it read (and skips a tag by reading)
      if (!drmp3_init(&d->mp3, cb_read, NULL, NULL, NULL, io, NULL)) break;
      d->p.rate = d->mp3.sampleRate;
      d->p.channels = (uint8_t)d->mp3.channels;
      ok = 1;
      break;
#endif
#if AUDIO_FLAC
    case AUD_FMT_FLAC:
      d->flac = resume ? drflac_open_relaxed(cb_read, cb_seek_flac, NULL, drflac_container_native, io, NULL)
                       : drflac_open(cb_read, cb_seek_flac, NULL, io, NULL);
      if (!d->flac) break;
      d->p.rate = d->flac->sampleRate;
      d->p.channels = (uint8_t)d->flac->channels;
      d->p.bits = (uint8_t)d->flac->bitsPerSample;
      d->p.total_frames = resume && prev ? prev->total_frames : d->flac->totalPCMFrameCount;
      ok = 1;
      break;
#endif
#if AUDIO_OGG
    case AUD_FMT_OGG:
      if (resume) break;   // Vorbis needs its headers: no seek
      ok = ogg_open(d);
      break;
#endif
#if AUDIO_AAC
    case AUD_FMT_AAC:
    case AUD_FMT_M4A:
      // ADTS after a flush: the parser finds the next frame; M4A needs its moov
      if (resume && fmt == AUD_FMT_M4A) break;
      d->aac = aud_aac_open(io, fmt == AUD_FMT_M4A, &d->p);
      ok = d->aac != NULL;
      break;
#endif
    default:
      break;
  }

  if (ok && (!d->p.rate || !d->p.channels || d->p.channels > DEC_MAXCH)) ok = 0;
  if (ok)
  {
    d->tmp = (int16_t*)malloc((size_t)DEC_CHUNK * d->p.channels * sizeof(int16_t));
    if (!d->tmp) ok = 0;
  }
  if (!ok)
  {
    aud_dec_close(d);
    return NULL;
  }
  return d;
}

const aud_params_t *aud_dec_params(const aud_dec_t *d)
{
  return &d->p;
}

#if AUDIO_WAV
// Raw WAV sample data (after a flush), with the converters of dr_wav
static size_t raw_read(aud_dec_t *d, int16_t *out, size_t frames)
{
  size_t ba = d->p.block_align;
  size_t bytes = aud_io_read(d->io, d->rawbuf, frames * ba);
  size_t n = bytes / ba;
  size_t s = n * d->p.channels;
  int bps = ba / d->p.channels;   // bytes per sample

  switch (d->p.wav_format)
  {
    case DR_WAVE_FORMAT_PCM:
      if (bps == 1) drwav_u8_to_s16(out, d->rawbuf, s);
      else if (bps == 2) memcpy(out, d->rawbuf, s * 2);
      else if (bps == 3) drwav_s24_to_s16(out, d->rawbuf, s);
      else if (bps == 4) drwav_s32_to_s16(out, (const drwav_int32*)d->rawbuf, s);
      else return 0;
      break;
    case DR_WAVE_FORMAT_IEEE_FLOAT:
      if (bps == 4) drwav_f32_to_s16(out, (const float*)d->rawbuf, s);
      else if (bps == 8) drwav_f64_to_s16(out, (const double*)d->rawbuf, s);
      else return 0;
      break;
    case DR_WAVE_FORMAT_ALAW:  drwav_alaw_to_s16(out, d->rawbuf, s); break;
    case DR_WAVE_FORMAT_MULAW: drwav_mulaw_to_s16(out, d->rawbuf, s); break;
    default: return 0;
  }
  return n;
}
#endif

size_t aud_dec_read(aud_dec_t *d, int16_t *out, size_t frames)
{
  if (frames > DEC_CHUNK) frames = DEC_CHUNK;
  int ch = d->p.channels;
  int16_t *dst = ch == 2 ? out : d->tmp;
  size_t n = 0;

  switch (d->fmt)
  {
#if AUDIO_WAV
    case AUD_FMT_WAV:
      n = d->raw ? raw_read(d, dst, frames) : (size_t)drwav_read_pcm_frames_s16(&d->wav, frames, dst);
      break;
#endif
#if AUDIO_MP3
    case AUD_FMT_MP3:
      n = (size_t)drmp3_read_pcm_frames_s16(&d->mp3, frames, dst);
      break;
#endif
#if AUDIO_FLAC
    case AUD_FMT_FLAC:
      n = (size_t)drflac_read_pcm_frames_s16(d->flac, frames, dst);
      break;
#endif
#if AUDIO_OGG
    case AUD_FMT_OGG:
      n = ogg_read(d, dst, frames);
      break;
#endif
#if AUDIO_AAC
    case AUD_FMT_AAC:
    case AUD_FMT_M4A:
      n = aud_aac_read(d->aac, dst, frames);
      break;
#endif
    default:
      break;
  }

  if (ch == 1)
    for (size_t i = 0; i < n; i++)
      out[2 * i] = out[2 * i + 1] = dst[i];
  else if (ch > 2)
    for (size_t i = 0; i < n; i++)
    {
      out[2 * i] = dst[i * ch];
      out[2 * i + 1] = dst[i * ch + 1];
    }
  return n;
}

void aud_dec_close(aud_dec_t *d)
{
  if (!d) return;
  switch (d->fmt)
  {
#if AUDIO_WAV
    case AUD_FMT_WAV:
      if (!d->raw && d->p.rate) drwav_uninit(&d->wav);
      free(d->rawbuf);
      break;
#endif
#if AUDIO_MP3
    case AUD_FMT_MP3:
      if (d->p.rate) drmp3_uninit(&d->mp3);
      break;
#endif
#if AUDIO_FLAC
    case AUD_FMT_FLAC:
      if (d->flac) drflac_close(d->flac);
      break;
#endif
#if AUDIO_OGG
    case AUD_FMT_OGG:
      if (d->vorbis) stb_vorbis_close(d->vorbis);
      free(d->obuf);
      break;
#endif
#if AUDIO_AAC
    case AUD_FMT_AAC:
    case AUD_FMT_M4A:
      aud_aac_close(d->aac);
      break;
#endif
    default:
      break;
  }
  free(d->tmp);
  free(d);
}

// ------------- Resampler

void aud_rs_reset(aud_rs_t *rs, uint32_t src_rate)
{
  memset(rs, 0, sizeof(*rs));
  rs->src_rate = src_rate;
}

static int pop_frame(aud_ring_t *pcm, int16_t *f)
{
  if (aud_ring_used(pcm) < 4) return 0;
  aud_ring_read(pcm, f, 4);
  return 1;
}

size_t aud_rs_render(aud_rs_t *rs, aud_ring_t *pcm, float *mix, size_t n, uint32_t out_rate, float gain)
{
  if (!rs->src_rate || !out_rate) return 0;

  // f0 and f1: the frames around the output position; primed = 2 both valid,
  // 1 only f0 (the ring ran empty right after a step)
  if (rs->primed == 0)
  {
    if (!pop_frame(pcm, rs->f0)) return 0;
    rs->primed = 1;
  }
  if (rs->primed == 1)
  {
    if (!pop_frame(pcm, rs->f1)) return 0;
    rs->primed = 2;
  }

  const float step = (float)rs->src_rate / (float)out_rate;
  size_t i;

  for (i = 0; i < n; i++)
  {
    float t = rs->t;
    mix[2 * i]     += ((float)rs->f0[0] + (float)(rs->f1[0] - rs->f0[0]) * t) * gain;
    mix[2 * i + 1] += ((float)rs->f0[1] + (float)(rs->f1[1] - rs->f0[1]) * t) * gain;

    t += step;
    while (t >= 1.f)
    {
      t -= 1.f;
      rs->f0[0] = rs->f1[0];
      rs->f0[1] = rs->f1[1];
      rs->played++;
      if (!pop_frame(pcm, rs->f1))
      {
        rs->t = t;
        rs->primed = 1;
        return i + 1;
      }
    }
    rs->t = t;
  }
  return n;
}
