
// ---- host driver (appended to the extracted stream parser) ----
#include <stdio.h>
void *xm_malloc(size_t s) { return malloc(s); }
u16 xm_rd_le16(const u8 *p) { return tracker_rd_le16(p); }
u32 xm_rd_le32(const u8 *p) { return tracker_rd_le32(p); }

int main(int argc, char **argv)
{
  int seconds = argc > 2 ? atoi(argv[2]) : 20;
  unsigned seed = argc > 3 ? atoi(argv[3]) : 1;
  size_t max_chunk = argc > 4 ? atoi(argv[4]) : 3000;  // 1..: also every byte in its own chunk
  if (!max_chunk) max_chunk = 1;
  FILE *f = fopen(argv[1], "rb");
  if (!f) { printf("open failed\n"); return 2; }
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *buf = (unsigned char*)malloc(n ? n : 1);
  if (fread(buf, 1, n, f) != (size_t)n) return 2;
  fclose(f);
  srand(seed);

  xm_host_stream_clear_state();
  g_xm_host_stream.active = true;
  g_xm_host_stream.format = TRACK_HOST_STREAM_FORMAT_XM;
  g_xm_host_stream.total_size = n;
  xm_context_t *ctx = NULL; size_t used = 0; int rc = TRACK_HOST_BUILD_NEED_MORE;
  while (rc == TRACK_HOST_BUILD_NEED_MORE)
  {
    size_t pos = g_xm_host_stream.pos, left = n - pos;
    if (!left) { rc = -100; break; }
    size_t sz = 1 + rand() % max_chunk;  // odd sizes too: words split between chunks
    if (sz > left) sz = left;
    // each chunk in its own exact-size buffer, so ASan sees reads past it
    unsigned char *chunk = (unsigned char*)malloc(sz);
    memcpy(chunk, buf + pos, sz);
    g_xm_host_stream.chunk_data = chunk;
    g_xm_host_stream.chunk_size = sz;
    g_xm_host_stream.chunk_pos = 0;
    g_xm_host_stream.chunk_active = true;
    rc = xm_host_stream_build_context(&ctx, &used);
    xm_host_stream_release_chunk();
    free(chunk);
  }
  free(buf);
  if (rc < 0 || !ctx) { printf("rc=%d\n", rc); fflush(stdout); return 0; }
  xm_set_max_loop_count(ctx, 1);
  static float out[2 * 4800];
  uint64_t h = 1469598103934665603ULL;
  for (int b = 0; b < seconds * 10; b++) {
    xm_render(ctx, out, 4800);
    const unsigned char *p = (const unsigned char*)out;
    for (size_t i = 0; i < sizeof(out); i++) { h ^= p[i]; h *= 1099511628211ULL; }
  }
  printf("rc=0 hash=%016llx\n", (unsigned long long)h);
  fflush(stdout);
  xm_free_context(ctx);
  return 0;
}
