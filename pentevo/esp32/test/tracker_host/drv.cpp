// Host driver: load a module through the same builders as TRACK_INIT, render, hash.
#include "tracker.h"
#define XM_SAMPLE_RATE 44100  // as tracker.cpp (the ESP renders at this rate)
#include "opl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
int mod_create_context_safe(xm_context_t **, void *, size_t, uint32_t, MFUNC, size_t *);
int s3m_create_context_safe(xm_context_t **, void *, size_t, uint32_t, MFUNC, size_t *);

int main(int argc, char **argv)
{
  int seconds = argc > 2 ? atoi(argv[2]) : 20;
  FILE *f = fopen(argv[1], "rb");
  if (!f) { printf("open failed\n"); return 2; }
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  // exact-size buffer so ASan sees any read past the end of the file
  char *buf = (char*)malloc(n ? n : 1);
  if (fread(buf, 1, n, f) != (size_t)n) return 2;
  fclose(f);
  const char *ext = strrchr(argv[1], '.');
  xm_context_t *ctx = NULL; int rc; size_t need = 0;
  if (ext && !strcasecmp(ext, ".mod")) rc = mod_create_context_safe(&ctx, buf, n, XM_SAMPLE_RATE, malloc, &need);
  else if (ext && !strcasecmp(ext, ".s3m")) rc = s3m_create_context_safe(&ctx, buf, n, XM_SAMPLE_RATE, malloc, &need);
  else rc = xm_create_context_safe(&ctx, buf, n, XM_SAMPLE_RATE, malloc);
  if (rc < 0 || !ctx) { printf("rc=%d\n", rc); fflush(stdout); free(buf); return 0; }
  xm_set_max_loop_count(ctx, 1);
  static float out[2 * 4800];
  uint64_t h = 1469598103934665603ULL;
  for (int b = 0; b < seconds * 10; b++) {
    xm_render(ctx, out, 4800);
    const unsigned char *p = (const unsigned char*)out;
    for (size_t i = 0; i < sizeof(out); i++) { h ^= p[i]; h *= 1099511628211ULL; }
  }
  printf("rc=%d hash=%016llx\n", rc, (unsigned long long)h);
  fflush(stdout);
  xm_free_context(ctx);
  free(buf);
  return 0;
}
