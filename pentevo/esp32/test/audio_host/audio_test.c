// Host test of main/audio/audio_core.c: a producer thread pushes a stream into
// the input ring in chunks of random size (as the DMA chunks of the Z80), the
// decoder runs in its own thread with blocking reads, as on the ESP.
//
//   audio_test dec   <stream> <ref> <max chunk> <seed> <kind>  decode, compare with ffmpeg
//   audio_test flush <stream> <ref> <max chunk> <seed> <kind>  flush at 40 %, go on at 60 %
//   audio_test junk  <stream> <fmt>                            no crash, no frames or few
//   audio_test rs                                              resampler: sine, passthrough
// kind: lossless (max diff <= 2) or lossy (SNR >= 40 dB after alignment)

#include "audio_core.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint8_t *file_buf;
static uint8_t *ref_buf;   // the reference of dec / flush, freed at the end
static size_t file_len;

static aud_ring_t ring;
static volatile int end_flag, abort_flag;
static aud_io_t io;

// producer: from..to of the file
static size_t prod_from, prod_to, max_chunk;
static volatile size_t prod_pos;
static volatile int prod_stop;

static void sleep_us(int us)
{
  struct timespec ts = { 0, us * 1000L };
  nanosleep(&ts, NULL);
}

static void wait_cb(void *arg)
{
  (void)arg;
  sleep_us(20);
}

static void *producer(void *arg)
{
  (void)arg;
  size_t pos = prod_from;
  while (pos < prod_to && !prod_stop)
  {
    // tiny chunks (<= 16 bytes) for the headers and the first frames only:
    // byte by byte the whole stream is slow under ASan
    size_t mc = max_chunk <= 16 && pos - prod_from > 96 * 1024 ? 3000 : max_chunk;
    size_t n = 1 + (size_t)rand() % mc;
    if (n > prod_to - pos) n = prod_to - pos;
    while (aud_ring_free(&ring) < n && !prod_stop) sleep_us(20);
    if (prod_stop) break;
    aud_ring_write(&ring, file_buf + pos, n);
    pos += n;
    prod_pos = pos;
  }
  // the end of the stream (AUDIO_END after the last PUSH); not on a stop
  if (!prod_stop && prod_to == file_len) end_flag = 1;
  return NULL;
}

static void start_producer(pthread_t *t, size_t from, size_t to)
{
  prod_from = from;
  prod_to = to;
  prod_pos = from;
  prod_stop = 0;
  pthread_create(t, NULL, producer, NULL);
}

static int16_t *out;
static size_t out_frames, out_cap;

static void out_add(const int16_t *f, size_t n)
{
  if (out_frames + n > out_cap)
  {
    out_cap = (out_frames + n) * 2;
    out = (int16_t*)realloc(out, out_cap * 4);
  }
  memcpy(out + out_frames * 2, f, n * 4);
  out_frames += n;
}

static int load(const char *path, uint8_t **buf, size_t *len)
{
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fseek(f, 0, SEEK_END);
  *len = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  *buf = (uint8_t*)malloc(*len ? *len : 1);   // exact size: ASan sees reads past it
  size_t r = fread(*buf, 1, *len, f);
  fclose(f);
  return r == *len;
}

// Format of the stream as the engine finds it: sniff the first bytes
static int sniff_ring(void)
{
  uint8_t head[16];
  while (1)
  {
    size_t n = aud_ring_peek(&ring, head, sizeof(head));
    int fmt = aud_sniff(head, n);
    if (fmt >= 0) return fmt;
    if (end_flag && aud_ring_used(&ring) == n) return aud_sniff(head, 12) < 0 ? 0 : fmt;
    sleep_us(50);
  }
}

static aud_dec_t *dec;
static aud_params_t params;

static void decode_all(void)
{
  int16_t f[1024 * 2];
  size_t n;
  while ((n = aud_dec_read(dec, f, 1 + (size_t)rand() % 1024)) > 0) out_add(f, n);
}

// Best alignment of a (n frames) against ref around expected, by the sum of
// absolute differences of the left channel; returns the offset in ref.
static long align(const int16_t *a, size_t n, const int16_t *ref, size_t ref_frames, long expected, long range)
{
  long best = expected;
  double best_err = 1e300;
  for (long o = expected - range; o <= expected + range; o++)
  {
    if (o < 0 || (size_t)o + n > ref_frames) continue;
    double e = 0;
    for (size_t i = 0; i < n; i += 4) e += fabs((double)a[2 * i] - ref[2 * (o + (long)i)]);
    if (e < best_err) { best_err = e; best = o; }
  }
  return best;
}

static int compare(const int16_t *a, size_t n, const int16_t *ref, size_t ref_frames, long off, int lossy, const char *what)
{
  if (off < 0) return 0;
  if ((size_t)off + n > ref_frames) n = ref_frames - (size_t)off;
  double sig = 0, err = 0;
  int maxd = 0;
  for (size_t i = 0; i < n * 2; i++)
  {
    int d = abs((int)a[i] - ref[(size_t)off * 2 + i]);
    if (d > maxd) maxd = d;
    sig += (double)ref[(size_t)off * 2 + i] * ref[(size_t)off * 2 + i];
    err += (double)d * d;
  }
  double snr = err ? 10 * log10(sig / err) : 999;
  int ok = lossy ? snr >= 40 : maxd <= 2;
  printf("%s: %zu frames at ref %ld, max diff %d, SNR %.1f dB: %s\n", what, n, off, maxd, snr, ok ? "ok" : "FAIL");
  return ok;
}

static int open_stream(int resume)
{
  int fmt = sniff_ring();
  if (fmt <= 0) { printf("sniff: unknown format\n"); return 0; }
  dec = aud_dec_open(fmt, &io, resume, resume ? &params : NULL, 0);
  if (!dec) { printf("open (fmt %d, resume %d): failed\n", fmt, resume); return 0; }
  params = *aud_dec_params(dec);
  return fmt;
}

static void setup(size_t ring_size)
{
  static uint8_t rb[256 * 1024];
  aud_ring_init(&ring, rb, (uint32_t)ring_size);
  io.in = &ring;
  io.end = &end_flag;
  io.abort = &abort_flag;
  io.wait = wait_cb;
  io.pos = 0;
}

static int test_dec(const char *stream, const char *refp, int lossy)
{
  uint8_t *rb; size_t rl;
  if (!load(stream, &file_buf, &file_len) || !load(refp, &rb, &rl)) { printf("load failed\n"); return 0; }
  ref_buf = rb;
  const int16_t *ref = (const int16_t*)rb;
  size_t ref_frames = rl / 4;

  setup(256 * 1024);
  pthread_t t;
  start_producer(&t, 0, file_len);
  int fmt = 0;
  // the end flag comes after the last byte (as AUDIO_END after the last PUSH)
  pthread_t waiter;
  (void)waiter;
  if (!(fmt = open_stream(0))) { prod_stop = 1; pthread_join(t, NULL); return 0; }
  printf("fmt %d, %u Hz, %u ch, %u bits, %llu frames\n", fmt, params.rate, params.channels, params.bits, (unsigned long long)params.total_frames);
  // END once the producer is through (decoding goes on meanwhile)
  while (prod_pos < file_len)
  {
    int16_t f[1024 * 2];
    size_t n = aud_dec_read(dec, f, 1 + (size_t)rand() % 1024);
    if (!n) break;
    out_add(f, n);
  }
  pthread_join(t, NULL);
  end_flag = 1;
  decode_all();
  aud_dec_close(dec);

  // after the first second: the start of a stream may be silent
  size_t a0 = out_frames > 44100 + 2048 ? 44100 : 0;
  long off = lossy ? align(out + a0 * 2, out_frames > 2048 ? 2048 : out_frames, ref, ref_frames, (long)a0, 3000) - (long)a0 : 0;
  int ok = compare(out, out_frames, ref, ref_frames, off, lossy, "decode");
  long len_diff = (long)out_frames + off - (long)ref_frames;
  printf("length: %zu frames, ref %zu, end diff %ld\n", out_frames, ref_frames, len_diff);
  if (labs(len_diff) > 3000) { printf("length: FAIL\n"); ok = 0; }
  if (params.total_frames && params.total_frames != out_frames) { printf("total %llu != decoded %zu: FAIL\n", (unsigned long long)params.total_frames, out_frames); ok = 0; }
  return ok;
}

static int test_flush(const char *stream, const char *refp, int lossy)
{
  uint8_t *rb; size_t rl;
  if (!load(stream, &file_buf, &file_len) || !load(refp, &rb, &rl)) { printf("load failed\n"); return 0; }
  ref_buf = rb;
  const int16_t *ref = (const int16_t*)rb;
  size_t ref_frames = rl / 4;

  setup(64 * 1024);
  pthread_t t;
  start_producer(&t, 0, file_len);
  int fmt = open_stream(0);
  if (!fmt) { prod_stop = 1; pthread_join(t, NULL); return 0; }

  // the audio data: after an ID3v2 tag (the host seeks only in there)
  size_t audio0 = 0;
  if (file_len > 10 && !memcmp(file_buf, "ID3", 3))
    audio0 = 10 + ((size_t)(file_buf[6] & 0x7F) << 21 | (size_t)(file_buf[7] & 0x7F) << 14 | (size_t)(file_buf[8] & 0x7F) << 7 | (file_buf[9] & 0x7F));
  size_t alen = file_len - audio0;

  // decode up to 40 % of the audio data, then flush (abort, drop the ring)
  while (io.pos < audio0 + alen * 4 / 10)
  {
    int16_t f[1024 * 2];
    size_t n = aud_dec_read(dec, f, 512);
    if (!n) break;
    out_add(f, n);
  }
  size_t before = out_frames;
  abort_flag = 1;
  prod_stop = 1;
  pthread_join(t, NULL);
  int16_t f[1024 * 2];
  if (aud_dec_read(dec, f, 512) > 1024) printf("read after abort?\n");
  aud_dec_close(dec);
  aud_ring_reset(&ring);
  abort_flag = 0;
  end_flag = 0;   // a short file: the first producer may have got to the end

  // the host goes on at 60 %, at any byte: a WAV continues at the next frame
  size_t at = audio0 + alen * 6 / 10 + 1;
  long expected = fmt == AUD_FMT_WAV ? (long)((at - params.data_offset + params.block_align - 1) / params.block_align)
                                     : (long)((double)(at - audio0) / alen * ref_frames);

  start_producer(&t, at, file_len);
  dec = aud_dec_open(fmt, &io, 1, &params, at);
  if (!aud_dec_seekable(fmt))
  {
    printf("resume of fmt %d: %s (not seekable)\n", fmt, dec ? "FAIL, opened" : "refused, ok");
    prod_stop = 1;
    pthread_join(t, NULL);
    if (dec) aud_dec_close(dec);
    return dec == NULL;
  }
  if (!dec) { printf("resume: open failed: FAIL\n"); prod_stop = 1; pthread_join(t, NULL); return 0; }
  decode_all();   // to the end flag of the producer
  pthread_join(t, NULL);
  aud_dec_close(dec);

  size_t n = out_frames - before;
  printf("before flush %zu frames, after %zu (expected at ref %ld)\n", before, n, expected);
  int ok = compare(out, before, ref, ref_frames, 0, lossy, "before");
  // after the flush: where it lands in ref (a VBR / FLAC frame boundary: a
  // window around the proportional position)
  const int16_t *a = out + before * 2;
  size_t win = n > 2048 ? 2048 : n;   // the search window (the compare takes all)
  // skip the first frames after resync (a decoder may need one frame to settle)
  size_t skip = n > win + 4096 ? 4096 : 0;
  long off = align(a + skip * 2, win, ref, ref_frames, expected + (long)skip, fmt == AUD_FMT_WAV ? 0 : 44100);
  ok &= compare(a + skip * 2, n - skip, ref, ref_frames, off, lossy, "after");
  return ok;
}

static int test_junk(const char *stream, int fmt)
{
  if (!load(stream, &file_buf, &file_len)) return 0;
  setup(64 * 1024);
  max_chunk = 4096;
  pthread_t t;
  start_producer(&t, 0, file_len);
  uint8_t head[16];
  while (aud_ring_used(&ring) < sizeof(head) && !end_flag) sleep_us(20);
  int s = aud_sniff(head, aud_ring_peek(&ring, head, sizeof(head)));
  dec = aud_dec_open(fmt, &io, 0, NULL, 0);
  size_t frames = 0;
  if (dec)
  {
    int16_t f[1024 * 2];
    size_t n;
    while ((n = aud_dec_read(dec, f, 1024)) > 0) frames += n;
    aud_dec_close(dec);
  }
  prod_stop = 1;   // a decoder that gave up early leaves data unread
  pthread_join(t, NULL);
  printf("junk as fmt %d: sniff %d, open %s, %zu frames: ok\n", fmt, s, dec ? "yes" : "no", frames);
  return 1;
}

static int test_rs(void)
{
  int ok = 1;
  static uint8_t pb[1 << 20];
  aud_ring_t pcm;

  // 44.1 kHz: the samples as they are
  aud_ring_init(&pcm, pb, sizeof(pb));
  for (int i = 0; i < 4410; i++) { int16_t f[2] = { (int16_t)(i * 7), (int16_t)-i }; aud_ring_write(&pcm, f, 4); }
  aud_rs_t rs;
  aud_rs_reset(&rs, 44100);
  static float mix[2 * 8192];
  memset(mix, 0, sizeof(mix));
  size_t n = 0, got;
  while ((got = aud_rs_render(&rs, &pcm, mix + 2 * n, 441, 44100, 1.f)) > 0) n += got;
  int exact = 1;
  for (size_t i = 0; i < n; i++)
    if (mix[2 * i] != (float)(int16_t)(i * 7) || mix[2 * i + 1] != (float)-(int)i) exact = 0;
  printf("rs 44100 -> 44100: %zu frames (of 4410, the last one stays), exact %d: %s\n", n, exact, exact && n == 4409 ? "ok" : "FAIL");
  ok &= exact && n == 4409;

  // 1 kHz sine at 48 kHz and 22.05 kHz -> 44.1 kHz: error against the ideal sine
  uint32_t rates[2] = { 48000, 22050 };
  for (int k = 0; k < 2; k++)
  {
    uint32_t sr = rates[k];
    aud_ring_init(&pcm, pb, sizeof(pb));
    for (uint32_t i = 0; i < sr; i++)
    {
      int16_t v = (int16_t)lrint(16000 * sin(2 * M_PI * 1000.0 * i / sr));
      int16_t f[2] = { v, v };
      aud_ring_write(&pcm, f, 4);
    }
    aud_rs_reset(&rs, sr);
    static float m2[2 * 44100];
    memset(m2, 0, sizeof(m2));
    n = 0;
    while (n < 40000 && (got = aud_rs_render(&rs, &pcm, m2 + 2 * n, 441, 44100, 1.f)) > 0) n += got;
    double sig = 0, err = 0;
    for (size_t i = 0; i < n; i++)
    {
      double ideal = 16000 * sin(2 * M_PI * 1000.0 * i / 44100);
      sig += ideal * ideal;
      err += (m2[2 * i] - ideal) * (m2[2 * i] - ideal);
    }
    double snr = 10 * log10(sig / err);
    int good = snr >= 35 && n >= 40000;
    printf("rs %u -> 44100: 1 kHz sine, %zu frames, SNR %.1f dB, played %llu: %s\n", sr, n, snr, (unsigned long long)rs.played, good ? "ok" : "FAIL");
    ok &= good;
  }
  return ok;
}

int main(int argc, char **argv)
{
  if (argc < 2) return 2;
  setvbuf(stdout, NULL, _IOLBF, 0);
  int ok;
  if (!strcmp(argv[1], "rs")) ok = test_rs();
  else if (!strcmp(argv[1], "junk") && argc >= 4) ok = test_junk(argv[2], atoi(argv[3]));
  else if (argc >= 7)
  {
    max_chunk = (size_t)atoi(argv[4]);
    srand((unsigned)atoi(argv[5]));
    int lossy = !strcmp(argv[6], "lossy");
    ok = !strcmp(argv[1], "dec") ? test_dec(argv[2], argv[3], lossy) : test_flush(argv[2], argv[3], lossy);
  }
  else return 2;
  free(out);
  free(file_buf);
  free(ref_buf);
  return ok ? 0 : 1;
}
