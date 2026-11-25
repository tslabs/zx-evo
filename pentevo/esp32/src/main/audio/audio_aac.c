// AAC (ADTS) and M4A (AAC in MP4) with esp_audio_codec of Espressif: AAC-LC,
// HE-AAC and HE-AACv2. The library is a binary for the ESP chips only, so this
// file is not in the host test (audio_core.c without AUDIO_AAC).
#include "audio_aac.h"

#if AUDIO_AAC

#include <stdlib.h>
#include <string.h>

#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "impl/esp_aac_dec.h"
#include "impl/esp_m4a_dec.h"

#define AAC_IN  4096          // input bytes handed to the parser at a time
#define AAC_PCM 8192          // one HE-AAC frame: 2048 stereo int16 frames (grows if asked)
#define AAC_MAX_ERR 200       // broken frames in a row before the stream is given up

struct aud_aac
{
  aud_io_t *io;
  esp_audio_simple_dec_handle_t dec;
  uint8_t in[AAC_IN];
  uint32_t in_len;
  int eos;
  uint8_t *pcm;               // decoded frame: interleaved int16
  uint32_t pcm_cap, pcm_len, pcm_pos;   // bytes
  int channels;
};

static int registered;

static void aac_register(void)
{
  if (registered) return;
  // only the decoders used here: the defaults link every codec of the library
  esp_aac_dec_register();
  esp_m4a_dec_register();
  registered = 1;
}

// One more decoded frame into pcm. 0: the end of the stream, an abort, or
// nothing decodable any more
static int aac_decode(aud_aac_t *a)
{
  int errors = 0;
  while (1)
  {
    if (*a->io->abort) return 0;   // also with a full input buffer
    if (!a->eos && a->in_len < AAC_IN)
    {
      size_t got = aud_io_read_some(a->io, a->in + a->in_len, AAC_IN - a->in_len);
      if (!got) a->eos = 1;
      a->in_len += (uint32_t)got;
    }
    if (!a->in_len && a->eos) return 0;

    esp_audio_simple_dec_raw_t raw = { .buffer = a->in, .len = a->in_len, .eos = a->eos != 0 };
    esp_audio_simple_dec_out_t out = { .buffer = a->pcm, .len = a->pcm_cap };
    esp_audio_err_t ret = esp_audio_simple_dec_process(a->dec, &raw, &out);
    if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH)
    {
      uint8_t *p = (uint8_t*)realloc(a->pcm, out.needed_size);
      if (!p) return 0;
      a->pcm = p;
      a->pcm_cap = out.needed_size;
      continue;
    }

    uint32_t used = raw.consumed <= a->in_len ? raw.consumed : a->in_len;
    if (used)
    {
      memmove(a->in, a->in + used, a->in_len - used);
      a->in_len -= used;
    }
    if (out.decoded_size)
    {
      a->pcm_len = out.decoded_size;
      a->pcm_pos = 0;
      return 1;
    }
    if (ret != ESP_AUDIO_ERR_OK && ret != ESP_AUDIO_ERR_DATA_LACK && ret != ESP_AUDIO_ERR_CONTINUE)
    {
      if (++errors > AAC_MAX_ERR) return 0;
      // a broken frame: drop a byte so the parser looks for the next one
      if (!used && a->in_len)
      {
        memmove(a->in, a->in + 1, a->in_len - 1);
        a->in_len--;
      }
      continue;
    }
    if (a->eos && !used) return 0;   // nothing more comes out of what is left
    if (!used && a->in_len == AAC_IN)
    {
      // the parser wants more than the buffer holds: not a stream it knows
      a->in_len = 0;
    }
  }
}

aud_aac_t *aud_aac_open(aud_io_t *io, int m4a, aud_params_t *p)
{
  aac_register();
  aud_aac_t *a = (aud_aac_t*)calloc(1, sizeof(aud_aac_t));
  if (!a) return NULL;
  a->io = io;
  // a buffer from the start: the library takes NULL as an invalid parameter
  a->pcm = (uint8_t*)malloc(AAC_PCM);
  if (!a->pcm)
  {
    aud_aac_close(a);
    return NULL;
  }
  a->pcm_cap = AAC_PCM;

  // HE-AAC on: most AAC radio. M4A has a config of its own (the track, AAC+)
  esp_aac_dec_cfg_t aac = ESP_AAC_DEC_CONFIG_DEFAULT();
  aac.aac_plus_enable = true;
  esp_m4a_dec_cfg_t mp4 = { .track_idx = 0, .aac_plus_enable = true };
  esp_audio_simple_dec_cfg_t cfg = {
    .dec_type = m4a ? ESP_AUDIO_SIMPLE_DEC_TYPE_M4A : ESP_AUDIO_SIMPLE_DEC_TYPE_AAC,
    .dec_cfg = m4a ? (void*)&mp4 : (void*)&aac,
    .cfg_size = m4a ? (int)sizeof(mp4) : (int)sizeof(aac),
  };
  if (esp_audio_simple_dec_open(&cfg, &a->dec) != ESP_AUDIO_ERR_OK || !a->dec)
  {
    a->dec = NULL;
    aud_aac_close(a);
    return NULL;
  }

  // the parameters come with the first frame
  esp_audio_simple_dec_info_t info = { 0 };
  if (!aac_decode(a) || esp_audio_simple_dec_get_info(a->dec, &info) != ESP_AUDIO_ERR_OK ||
      info.bits_per_sample != 16 || !info.channel)
  {
    aud_aac_close(a);
    return NULL;
  }
  a->channels = info.channel;
  p->rate = info.sample_rate;
  p->channels = info.channel;
  p->bits = 0;
  p->bitrate = info.bitrate;
  return a;
}

size_t aud_aac_read(aud_aac_t *a, int16_t *out, size_t frames)
{
  size_t done = 0;
  uint32_t fb = (uint32_t)a->channels * 2;   // bytes per frame
  while (done < frames)
  {
    if (a->pcm_pos + fb > a->pcm_len)
    {
      if (!aac_decode(a)) break;
      continue;
    }
    size_t n = (a->pcm_len - a->pcm_pos) / fb;
    if (n > frames - done) n = frames - done;
    memcpy(out + done * a->channels, a->pcm + a->pcm_pos, n * fb);
    a->pcm_pos += (uint32_t)(n * fb);
    done += n;
  }
  return done;
}

uint32_t aud_aac_bitrate(aud_aac_t *a)
{
  esp_audio_simple_dec_info_t info = { 0 };
  if (esp_audio_simple_dec_get_info(a->dec, &info) != ESP_AUDIO_ERR_OK) return 0;
  return info.bitrate;
}

void aud_aac_close(aud_aac_t *a)
{
  if (!a) return;
  if (a->dec) esp_audio_simple_dec_close(a->dec);
  free(a->pcm);
  free(a);
}

#endif
