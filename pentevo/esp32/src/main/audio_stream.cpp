// Audio stream player: the commands of the host, the decoder task, the render
// into the mix of player_task. The decoding itself is audio/audio_core.c.
//
// Who does what:
//   command ISR    START / FLUSH / STOP: a request to the task (the task gives
//                  the status); END / PAUSE / VOLUME / GET_STATE at once; PUSH:
//                  the size check, then the DMA of the receiver task
//   receiver task  the PUSH data into the input ring
//   audio task     the decoder: input ring -> PCM ring (stereo int16, source rate)
//   player_task    the resampler: PCM ring -> mix (44.1 kHz)

#include "audio_stream.h"

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "audio/audio_core.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"

#define TAG "audio"

// at least one tick: pdMS_TO_TICKS(5) is 0 at 100 Hz, and a wait of 0 spun the
// task on core 1 (IDLE1 starved, the task watchdog rebooted after 30 s)
#define WAIT_MS(ms) (pdMS_TO_TICKS(ms) ? pdMS_TO_TICKS(ms) : 1)

#define AUD_IN_SIZE     (512 * 1024)   // host data: ~10 s of MP3 at 320 kbps, ~3 s of FLAC CD
#define AUD_PCM_SIZE    (64 * 1024)    // decoded: 16384 frames, 0.37 s at 44.1 kHz
#define AUD_DEC_FRAMES  1024
#define AUD_TASK_STACK  16384          // stb_vorbis and dr_flac
#define AUD_TASK_PRIO   8              // core 1, under player_task (10) and i2s (12)

enum
{
  REQ_START = 1,
  REQ_FLUSH = 2,
  REQ_STOP  = 4,
};

static struct
{
  volatile u8 state;
  volatile u8 fmt;            // asked by START (AUTO) or found
  volatile u8 err;
  volatile u8 atten;          // AUDIO_VOLUME: 0.5 dB steps, as VS1053 (#FE: silence)
  u8 gain_atten;              // player_task: gain is for this attenuation
  float gain;
  volatile int paused;
  volatile int end;           // AUDIO_END received
  volatile int abort;         // a request: the decoder gives up its blocking reads
  volatile u32 req;           // REQ_*
  volatile u8 req_fmt;
  volatile u32 underruns;
  volatile int seekable;
  volatile u32 flush_at;      // FLUSH: file offset of the next pushed byte

  u8 *in_buf;
  u8 *pcm_buf;
  aud_ring_t in;
  aud_ring_t pcm;
  aud_io_t io;

  aud_params_t params;        // of the stream, kept over a flush
  int have_params;
  int resume;                 // the next decoder starts after a flush

  volatile u32 rx_bytes;      // stats: host bytes received
  u32 dec_us;                 // stats: time in the decoder
  aud_rs_t rs;                // player_task only (reset under lock)
  int starved;                // the last render ran out of samples
  SemaphoreHandle_t lock;     // render <-> rings / resampler reset
  TaskHandle_t task;
} A;

// ------------- Registers (also from the command ISR)

static u32 IRAM_ATTR ring_used(const aud_ring_t *r)
{
  return (u32)(r->head - r->tail);
}

static void IRAM_ATTR audio_regs()
{
  u8 flags = (A.seekable ? AUDIO_FLAG_SEEKABLE : 0) | (A.paused ? AUDIO_FLAG_PAUSED : 0) | (A.end ? AUDIO_FLAG_END : 0);
  u32 free_bytes = A.in_buf ? A.in.size - ring_used(&A.in) : 0;
  // no PUSH before the task took a START / FLUSH / STOP
  if (A.req) free_bytes = 0;
  u16 und = (u16)(A.underruns > 0xFFFF ? 0xFFFF : A.underruns);

  wr_reg8(ESP_REG_AUDIO_STATE, A.state);
  wr_reg8(ESP_REG_AUDIO_FMT, A.have_params ? A.fmt : 0);
  wr_reg8(ESP_REG_AUDIO_CHANNELS, A.have_params ? A.params.channels : 0);
  wr_reg8(ESP_REG_AUDIO_FLAGS, flags);
  wr_reg32(ESP_REG_AUDIO_FREE, free_bytes);
  wr_reg32(ESP_REG_AUDIO_RATE, A.have_params ? A.params.rate : 0);
  wr_reg32(ESP_REG_AUDIO_PLAYED, (u32)A.rs.played);
  wr_reg32(ESP_REG_AUDIO_TOTAL, A.have_params ? (u32)A.params.total_frames : 0);
  wr_reg32(ESP_REG_AUDIO_BUFFERED, A.in_buf ? ring_used(&A.in) : 0);
  wr_regs(ESP_REG_AUDIO_UNDERRUNS, &und, 2);
  wr_reg8(ESP_REG_AUDIO_ERROR, A.err);
  wr_reg8(ESP_REG_AUDIO_BITS, A.have_params ? A.params.bits : 0);
  wr_reg32(ESP_REG_AUDIO_BITRATE, A.have_params ? A.params.bitrate : 0);
}

static void IRAM_ATTR request(u32 r)
{
  // the ISR runs on the other core than the task: atomics, req before abort
  __atomic_fetch_or(&A.req, r, __ATOMIC_SEQ_CST);
  A.abort = 1;
  BaseType_t hp = pdFALSE;
  if (A.task) vTaskNotifyGiveFromISR(A.task, &hp);
  portYIELD_FROM_ISR(hp);
}

void IRAM_ATTR audio_command(u8 cmd)
{
  switch (cmd)
  {
    case ESP_CMD_AUDIO_START:
      A.req_fmt = rd_reg8(ESP_REG_AUDIO_FORMAT);
      request(REQ_START);   // the task gives READY (or OUT_OF_MEMORY)
      break;

    case ESP_CMD_AUDIO_FLUSH:
      if (A.state == AUDIO_ST_IDLE) { set_status(ESP_ERR_INV_STATE); break; }
      A.flush_at = rd_reg32(ESP_REG_DATA_OFFSET);
      request(REQ_FLUSH);
      break;

    case ESP_CMD_AUDIO_STOP:
      request(REQ_STOP);
      break;

    case ESP_CMD_AUDIO_END:
      if (A.state == AUDIO_ST_IDLE) { set_status(ESP_ERR_INV_STATE); break; }
      A.end = 1;
      if (A.task) { BaseType_t hp = pdFALSE; vTaskNotifyGiveFromISR(A.task, &hp); portYIELD_FROM_ISR(hp); }
      audio_regs();
      set_status(ESP_ST_READY);
      break;

    case ESP_CMD_AUDIO_PAUSE:
      A.paused = rd_reg8(ESP_REG_AUDIO_PARAM) ? 1 : 0;
      audio_regs();
      set_status(ESP_ST_READY);
      break;

    case ESP_CMD_AUDIO_VOLUME:
      A.atten = rd_reg8(ESP_REG_AUDIO_PARAM);   // the gain: in player_task (no float in the ISR)
      set_status(ESP_ST_READY);
      break;

    case ESP_CMD_AUDIO_GET_STATE:
      audio_regs();
      set_status(ESP_ST_READY);
      break;

    default:
      set_status(ESP_ERR_INV_COMMAND);
  }
}

bool IRAM_ATTR audio_push_check(size_t size)
{
  if (A.req || !A.in_buf || A.end || (A.state != AUDIO_ST_WAIT && A.state != AUDIO_ST_PLAY))
  {
    audio_regs();
    set_status(ESP_ERR_INV_STATE);
    return false;
  }
  if (!size || size > DMA_BUF_SIZE || size > A.in.size - ring_used(&A.in))
  {
    audio_regs();
    set_status(ESP_ERR_INV_SIZE);
    return false;
  }
  return true;
}

// ------------- Receiver task

void audio_rx(const u8 *data, size_t size)
{
  // checked by audio_push_check(); a request in between drops the data
  if (!A.req && A.in_buf) aud_ring_write(&A.in, data, size);
  A.rx_bytes = A.rx_bytes + size;
  if (A.task) xTaskNotifyGive(A.task);
  audio_regs();
  set_status(ESP_ST_READY);
}

// ------------- Audio task

static void wait_data(void *arg)
{
  ulTaskNotifyTake(pdTRUE, WAIT_MS(10));
  // an abort lost between the two stores of handle_requests(): the request is there
  if (__atomic_load_n(&A.req, __ATOMIC_SEQ_CST)) A.abort = 1;
}

static void set_error(u8 err)
{
  A.err = err;
  A.state = AUDIO_ST_ERROR;
  ESP_LOGW(TAG, "error %02X", err);
}

// Rings and resampler empty; player_task does not render meanwhile
static void reset_buffers()
{
  xSemaphoreTake(A.lock, portMAX_DELAY);
  aud_ring_reset(&A.in);
  aud_ring_reset(&A.pcm);
  aud_rs_reset(&A.rs, A.have_params ? A.params.rate : 0);
  A.starved = 0;
  xSemaphoreGive(A.lock);
}

static bool alloc_buffers()
{
  if (A.in_buf) return true;
  A.in_buf = (u8*)heap_caps_malloc(AUD_IN_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  A.pcm_buf = (u8*)heap_caps_malloc(AUD_PCM_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!A.in_buf || !A.pcm_buf)
  {
    heap_caps_free(A.in_buf);
    heap_caps_free(A.pcm_buf);
    A.in_buf = A.pcm_buf = NULL;
    return false;
  }
  aud_ring_init(&A.in, A.in_buf, AUD_IN_SIZE);
  aud_ring_init(&A.pcm, A.pcm_buf, AUD_PCM_SIZE);
  return true;
}

// START / FLUSH / STOP: the decoder is closed (its reads returned at abort)
static void handle_requests(aud_dec_t **dec)
{
  u32 r = __atomic_load_n(&A.req, __ATOMIC_SEQ_CST);
  if (!r) return;

  if (*dec)
  {
    aud_dec_close(*dec);
    *dec = NULL;
  }

  // the last one wins: STOP over START over FLUSH
  if (r & REQ_STOP)
  {
    A.state = AUDIO_ST_IDLE;
    A.have_params = 0;
    A.seekable = 0;
    if (A.in_buf) reset_buffers();
  }
  else if (r & REQ_START)
  {
    A.fmt = A.req_fmt;
    A.have_params = 0;
    A.resume = 0;
    A.seekable = 0;
    A.err = 0;
    A.underruns = 0;
    A.paused = 0;
    if (!alloc_buffers())
    {
      set_error(ESP_ERR_OUT_OF_MEMORY);
    }
    else
    {
      reset_buffers();
      A.state = AUDIO_ST_WAIT;
    }
  }
  else if (r & REQ_FLUSH)
  {
    // the stream goes on at the host's offset with the same format
    A.resume = A.have_params;
    if (A.in_buf) reset_buffers();
    A.state = AUDIO_ST_WAIT;
  }

  A.end = 0;
  // requests that came in meanwhile stay for the next round
  if (!(__atomic_and_fetch(&A.req, ~r, __ATOMIC_SEQ_CST))) A.abort = 0;

  audio_regs();
  set_status(A.state == AUDIO_ST_ERROR ? A.err : (u8)ESP_ST_READY);
}

static void audio_task(void *arg)
{
  aud_dec_t *dec = NULL;
  static int16_t pcm[AUD_DEC_FRAMES * 2];

  A.io.in = &A.in;
  A.io.end = &A.end;
  A.io.abort = &A.abort;
  A.io.wait = wait_data;
  A.io.wait_arg = NULL;

  int64_t stat_t = esp_timer_get_time();
  u32 stat_rx = 0;

  while (1)
  {
    handle_requests(&dec);

    // every 5 s while playing: host rate, decoder load, buffer, gaps
    int64_t now = esp_timer_get_time();
    if (now - stat_t >= 5000000)
    {
      if (A.state == AUDIO_ST_PLAY || A.state == AUDIO_ST_DRAIN)
        ESP_LOGI(TAG, "in %lu KB/s, decoder %lu %%, buffer %lu KB, underruns %lu",
                 (unsigned long)((A.rx_bytes - stat_rx) / 5 / 1024), (unsigned long)(A.dec_us / ((now - stat_t) / 100)),
                 (unsigned long)(aud_ring_used(&A.in) / 1024), (unsigned long)A.underruns);
      stat_t = now;
      stat_rx = A.rx_bytes;
      A.dec_us = 0;
    }

    if (A.state != AUDIO_ST_WAIT && A.state != AUDIO_ST_PLAY)
    {
      ulTaskNotifyTake(pdTRUE, WAIT_MS(50));
      continue;
    }

    if (!dec)
    {
      // the format: from the first bytes unless the host said it
      int fmt = A.fmt;
      if (fmt == AUDIO_FMT_AUTO)
      {
        u8 head[16];
        size_t n = aud_ring_peek(&A.in, head, sizeof(head));
        fmt = aud_sniff(head, n);
        if (fmt < 0)
        {
          if (A.end && n == aud_ring_used(&A.in)) set_error(ESP_ERR_AUDIO_FORMAT);
          else ulTaskNotifyTake(pdTRUE, WAIT_MS(20));
          continue;
        }
        if (fmt == 0) { set_error(ESP_ERR_AUDIO_FORMAT); continue; }
      }

      dec = aud_dec_open(fmt, &A.io, A.resume, A.have_params ? &A.params : NULL, A.flush_at);
      if (!dec)
      {
        if (!A.abort) set_error(ESP_ERR_AUDIO_DECODE);
        continue;
      }

      xSemaphoreTake(A.lock, portMAX_DELAY);
      A.params = *aud_dec_params(dec);
      A.fmt = (u8)fmt;
      A.have_params = 1;
      A.seekable = aud_dec_seekable(fmt);
      aud_rs_reset(&A.rs, A.params.rate);
      A.starved = 0;
      xSemaphoreGive(A.lock);
      A.state = AUDIO_ST_PLAY;
      ESP_LOGI(TAG, "fmt %d, %lu Hz, %u ch, %u bits, %llu frames%s", fmt, (unsigned long)A.params.rate,
               A.params.channels, A.params.bits, (unsigned long long)A.params.total_frames, A.resume ? ", resumed" : "");
    }

    if (A.paused)
    {
      ulTaskNotifyTake(pdTRUE, WAIT_MS(20));
      continue;
    }

    size_t room = aud_ring_free(&A.pcm) / 4;
    if (room < 256)
    {
      ulTaskNotifyTake(pdTRUE, WAIT_MS(5));
      continue;
    }

    int64_t t0 = esp_timer_get_time();
    size_t n = aud_dec_read(dec, pcm, room < AUD_DEC_FRAMES ? room : AUD_DEC_FRAMES);
    A.dec_us += (u32)(esp_timer_get_time() - t0);
    if (n)
    {
      aud_ring_write(&A.pcm, pcm, n * 4);
      continue;
    }

    if (A.abort) continue;   // a request: handled at the top

    // the end of the stream: the render plays out what is decoded
    aud_dec_close(dec);
    dec = NULL;
    A.state = AUDIO_ST_DRAIN;
  }
}

// ------------- player_task

void audio_render(float *mix, int samples, int rate)
{
  u8 st = A.state;
  if ((st != AUDIO_ST_PLAY && st != AUDIO_ST_DRAIN) || A.paused) return;
  if (xSemaphoreTake(A.lock, 0) != pdTRUE) return;   // a reset is going on

  u8 at = A.atten;
  if (at != A.gain_atten)
  {
    A.gain_atten = at;
    A.gain = at >= 0xFE ? 0.f : powf(10.f, -(float)at / 40.f);
  }
  size_t got = aud_rs_render(&A.rs, &A.pcm, mix, (size_t)samples, (u32)rate, A.gain);

  if (got < (size_t)samples)
  {
    if (st == AUDIO_ST_DRAIN) A.state = AUDIO_ST_ENDED;
    else if (!A.starved && A.rs.played) A.underruns = A.underruns + 1;   // once per gap, not before the first samples
    A.starved = 1;
  }
  else
    A.starved = 0;

  xSemaphoreGive(A.lock);
}

void audio_init()
{
  memset(&A, 0, sizeof(A));
  A.atten = 0;
  A.gain_atten = 0;
  A.gain = 1.f;
  A.lock = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCoreWithCaps(audio_task, "audio", AUD_TASK_STACK, NULL, AUD_TASK_PRIO, &A.task, 1, task_ram_type_non_critical);
}
