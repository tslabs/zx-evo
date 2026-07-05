// ============================================================================
// Includes
// ============================================================================

// -------------------- Primary project header --------------------
#include "tracker.h"

// -------------------- C / POSIX headers --------------------
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// -------------------- ESP-IDF / FreeRTOS headers --------------------
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include <esp_crc.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"

// -------------------- Project headers --------------------
#include "main.h"
#include "mem_obj.h"
#include "spi_slave.h"
#include "stats.h"
#include "esp_spi_defs.h"
#include "sdmmc.h"
#include "sfx.h"
#include "opl.h"

// ============================================================================
// Common tracker helpers
// ============================================================================


// -------------------- Common byte order helpers --------------------

uint16_t tracker_rd_be16(const uint8_t *p)
{
  return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

uint16_t tracker_rd_le16(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t tracker_rd_le24(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

uint32_t tracker_rd_le32(const uint8_t *p)
{
  return tracker_rd_le16(p) | ((uint32_t)tracker_rd_le16(p + 2) << 16);
}

// -------------------- Common size helpers --------------------

int tracker_size_add(size_t *dst, size_t add)
{
  if (!dst) return 0;
  if (SIZE_MAX - *dst < add) return 0;
  *dst += add;
  return 1;
}

int tracker_size_mul(size_t a, size_t b, size_t *out)
{
  if (!out) return 0;
  if (a && b > SIZE_MAX / a) return 0;
  *out = a * b;
  return 1;
}

// -------------------- Common string/memory helpers --------------------

void tracker_copy_trimmed(char *dst, size_t dst_size, const uint8_t *src, size_t src_size)
{
  size_t n;

  if (!dst || !dst_size) return;

  n = src_size;
  while (n && (src[n - 1] == 0 || src[n - 1] == ' ')) n--;
  if (n >= dst_size) n = dst_size - 1;

  if (n) memcpy(dst, src, n);
  dst[n] = 0;
}

void tracker_memcpy_pad(void *dst, size_t dst_len, const void *src, size_t src_len, size_t offset)
{
  uint8_t *dst_c = (uint8_t*)dst;
  const uint8_t *src_c = (const uint8_t*)src;
  size_t copy_bytes;

  if (!dst || !dst_len) return;

  copy_bytes = (src_len >= offset) ? (src_len - offset) : 0;
  if (copy_bytes > dst_len) copy_bytes = dst_len;

  if (copy_bytes) memcpy(dst_c, src_c + offset, copy_bytes);
  memset(dst_c + copy_bytes, 0, dst_len - copy_bytes);
}

// ============================================================================
// XM context / public API
// ============================================================================

#define OFFSET(ptr) do {										\
		(ptr) = reinterpret_cast<decltype(ptr)>((intptr_t)(ptr) + (intptr_t)(*ctxp));	\
	} while(0)


int xm_group_sizes_add(xm_context_group_sizes_t *sizes, size_t *field, size_t add);
int xm_group_sizes_add_mul(xm_context_group_sizes_t *sizes, size_t *field, size_t a, size_t b);
int xm_measure_group_sizes_from_memory(const char *moddata, size_t moddata_length, xm_context_group_sizes_t *sizes);
void *xm_group_cursor_alloc(char **cursor, char *end, size_t size, bool clear);
void *xm_alloc_and_register_group(xm_context_t *ctx, size_t size, uint8_t type, MFUNC mfunc, bool clear);
int xm_allocate_context_groups(xm_context_t **ctxp, xm_context_group_cursor_t *cursor, const xm_context_group_sizes_t *sizes, MFUNC mfunc);
int xm_setup_runtime_group(xm_context_t *ctx, xm_context_group_cursor_t *cursor, uint32_t rate);

int xm_create_context_safe(xm_context_t** ctxp, void* moddata, size_t moddata_length, uint32_t rate, MFUNC mfunc)
{
  xm_context_group_sizes_t sizes;
  xm_context_group_cursor_t cursor;
  xm_context_t *ctx;

  if (ctxp) *ctxp = NULL;
  if (!ctxp || !moddata || !mfunc) return -1;

  if (XM_DEFENSIVE)
  {
    int ret = xm_check_sanity_preload((const char*)moddata, moddata_length);
    if (ret)
    {
      DEBUG("xm_check_sanity_preload() returned %i, module is not safe to load", ret);
      return -1;
    }
  }

  if (!xm_measure_group_sizes_from_memory((const char*)moddata, moddata_length, &sizes)) return -2;

  if (!xm_allocate_context_groups(ctxp, &cursor, &sizes, mfunc)) return -2;

  ctx = *ctxp;
  ctx->rate = rate;

  if (!xm_load_module(ctx, (const char*)moddata, moddata_length, &cursor))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    return -2;
  }

  if (!xm_setup_runtime_group(ctx, &cursor, rate))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    return -2;
  }

  if (XM_DEFENSIVE)
  {
    int ret = xm_check_sanity_postload(ctx);
    if (ret)
    {
      DEBUG("xm_check_sanity_postload() returned %i, module is not safe to play", ret);
      xm_free_context(ctx);
      *ctxp = NULL;
      return -1;
    }
  }

  return (int)sizes.total_size;
}

void xm_create_context_from_libxmize(xm_context_t** ctxp, char* libxmized, uint32_t rate) {
	size_t i, j;

	*ctxp = (xm_context_t*)libxmized;

	/* Reverse steps of libxmize.c */
	OFFSET((*ctxp)->module.patterns);
	OFFSET((*ctxp)->module.instruments);
	OFFSET((*ctxp)->row_loop_count);
	OFFSET((*ctxp)->channels);

	for(i = 0; i < (*ctxp)->module.num_patterns; ++i) {
		OFFSET((*ctxp)->module.patterns[i].slots);
	}

	for(i = 0; i < (*ctxp)->module.num_instruments; ++i) {
		OFFSET((*ctxp)->module.instruments[i].samples);

		for(j = 0; j < (*ctxp)->module.instruments[i].num_samples; ++j) {
			OFFSET((*ctxp)->module.instruments[i].samples[j].data8);

			if(XM_LIBXMIZE_DELTA_SAMPLES) {
				if((*ctxp)->module.instruments[i].samples[j].length > 1) {
					if((*ctxp)->module.instruments[i].samples[j].bits == 8) {
						for(size_t k = 1; k < (*ctxp)->module.instruments[i].samples[j].length; ++k) {
							(*ctxp)->module.instruments[i].samples[j].data8[k] += (*ctxp)->module.instruments[i].samples[j].data8[k-1];
						}
					} else {
						for(size_t k = 1; k < (*ctxp)->module.instruments[i].samples[j].length; ++k) {
							(*ctxp)->module.instruments[i].samples[j].data16[k] += (*ctxp)->module.instruments[i].samples[j].data16[k-1];
						}
					}
				}
			}
		}
	}
}

struct tracker_context_segment_page_s
{
  tracker_context_segment_page_t *next;
  uint16_t used;
  tracker_context_segment_t segments[TRACKER_CONTEXT_SEGMENT_PAGE_CAPACITY];
};

int tracker_context_has_registered_segments(const xm_context_t *ctx)
{
  if (!ctx) return 0;
  if (ctx->ctx_size < offsetof(xm_context_t, tracker_segment_pages) + sizeof(ctx->tracker_segment_pages)) return 0;
  if (ctx->tracker_segments_magic != TRACKER_CONTEXT_SEGMENTS_MAGIC) return 0;
  if (ctx->tracker_segments_magic_check != TRACKER_CONTEXT_SEGMENTS_MAGIC_CHECK) return 0;
  if (!ctx->tracker_segment_pages) return 0;

  return ctx->tracker_segment_count > 0;
}

uint16_t tracker_context_get_segment_count(const xm_context_t *ctx)
{
  if (!tracker_context_has_registered_segments(ctx)) return 0;
  return ctx->tracker_segment_count;
}

int tracker_context_get_segment(const xm_context_t *ctx, uint16_t index, tracker_context_segment_t *segment)
{
  uint16_t base = 0;

  if (!segment) return 0;
  memset(segment, 0, sizeof(*segment));
  if (!tracker_context_has_registered_segments(ctx)) return 0;
  if (index >= ctx->tracker_segment_count) return 0;

  for (tracker_context_segment_page_t *page = ctx->tracker_segment_pages; page; page = page->next)
  {
    if (index < base + page->used)
    {
      *segment = page->segments[index - base];
      return 1;
    }

    base += page->used;
  }

  return 0;
}

int tracker_context_register_segment(xm_context_t *ctx, void *addr, size_t size, uint8_t type)
{
  tracker_context_segment_page_t *page;
  tracker_context_segment_page_t *last_page = NULL;

  if (!ctx || !addr || !size) return 0;
  if (ctx->ctx_size < offsetof(xm_context_t, tracker_segment_pages) + sizeof(ctx->tracker_segment_pages)) return 0;

  if (ctx->tracker_segments_magic != TRACKER_CONTEXT_SEGMENTS_MAGIC || ctx->tracker_segments_magic_check != TRACKER_CONTEXT_SEGMENTS_MAGIC_CHECK)
  {
    if (ctx->tracker_segments_magic || ctx->tracker_segments_magic_check || ctx->tracker_segment_count || ctx->tracker_segment_pages) return 0;

    ctx->tracker_segments_magic = TRACKER_CONTEXT_SEGMENTS_MAGIC;
    ctx->tracker_segments_magic_check = TRACKER_CONTEXT_SEGMENTS_MAGIC_CHECK;
  }

  for (page = ctx->tracker_segment_pages; page; page = page->next)
  {
    last_page = page;

    for (uint16_t i = 0; i < page->used; i++)
    {
      if (page->segments[i].addr == addr)
      {
        page->segments[i].size = size;
        page->segments[i].type = type;
        page->segments[i].flags = 0;
        page->segments[i].index = 0xffff;
        return 1;
      }
    }
  }

  if (ctx->tracker_segment_count == UINT16_MAX) return 0;

  if (!last_page || last_page->used >= TRACKER_CONTEXT_SEGMENT_PAGE_CAPACITY)
  {
    page = (tracker_context_segment_page_t*)malloc_spiram(sizeof(tracker_context_segment_page_t));
    if (!page) return 0;
    memset(page, 0, sizeof(*page));

    if (last_page)
      last_page->next = page;
    else
      ctx->tracker_segment_pages = page;

    last_page = page;
  }

  tracker_context_segment_t *seg = last_page->segments + last_page->used;
  seg->addr = addr;
  seg->size = size;
  seg->type = type;
  seg->flags = 0;
  seg->index = 0xffff;

  last_page->used++;
  ctx->tracker_segment_count++;

  return 1;
}

void tracker_context_free_segments(xm_context_t *ctx)
{
  tracker_context_segment_page_t *page;
  tracker_context_segment_page_t *next_page;
  if (!ctx) return;

  if (!tracker_context_has_registered_segments(ctx))
  {
    free(ctx);
    return;
  }

  page = ctx->tracker_segment_pages;

  for (tracker_context_segment_page_t *p = page; p; p = p->next)
  {
    for (uint16_t i = 0; i < p->used; i++)
    {
      void *addr = p->segments[i].addr;
      if (!addr) continue;
      if (addr == ctx) continue;

      free(addr);
      p->segments[i].addr = NULL;
    }
  }

  free(ctx);

  while (page)
  {
    next_page = page->next;
    free(page);
    page = next_page;
  }
}

void xm_free_context(xm_context_t* context)
{
  tracker_context_free_segments(context);
}

void xm_set_max_loop_count(xm_context_t* context, uint8_t loopcnt) {
	context->max_loop_count = loopcnt;
}

uint8_t xm_get_loop_count(xm_context_t* context) {
	return context->loop_count;
}

void xm_seek(xm_context_t* ctx, uint8_t pot, uint8_t row, uint16_t tick) {
	ctx->current_table_index = pot;
	ctx->current_row = row;
	ctx->current_tick = tick;
	ctx->remaining_samples_in_tick = 0;
}

bool xm_mute_channel(xm_context_t* ctx, uint16_t channel, bool mute) {
	bool old = ctx->channels[channel - 1].muted;
	ctx->channels[channel - 1].muted = mute;
	return old;
}

bool xm_mute_instrument(xm_context_t* ctx, uint16_t instr, bool mute) {
	bool old = ctx->module.instruments[instr - 1].muted;
	ctx->module.instruments[instr - 1].muted = mute;
	return old;
}

#if XM_STRINGS
const char* xm_get_module_name(xm_context_t* ctx) {
	return ctx->module.name;
}

const char* xm_get_tracker_name(xm_context_t* ctx) {
	return ctx->module.trackername;
}
#else
const char* xm_get_module_name(xm_context_t* ctx) {
	return NULL;
}

const char* xm_get_tracker_name(xm_context_t* ctx) {
	return NULL;
}
#endif

uint16_t xm_get_number_of_channels(xm_context_t* ctx) {
	return ctx->module.num_channels;
}

uint16_t xm_get_module_length(xm_context_t* ctx) {
	return ctx->module.length;
}

uint16_t xm_get_number_of_patterns(xm_context_t* ctx) {
	return ctx->module.num_patterns;
}

uint16_t xm_get_number_of_rows(xm_context_t* ctx, uint16_t pattern) {
	return ctx->module.patterns[pattern].num_rows;
}

uint16_t xm_get_number_of_instruments(xm_context_t* ctx) {
	return ctx->module.num_instruments;
}

uint16_t xm_get_number_of_samples(xm_context_t* ctx, uint16_t instrument) {
	return ctx->module.instruments[instrument - 1].num_samples;
}

void* xm_get_sample_waveform(xm_context_t* ctx, uint16_t i, uint16_t s, size_t* size, uint8_t* bits) {
	*size = ctx->module.instruments[i - 1].samples[s].length;
	*bits = ctx->module.instruments[i - 1].samples[s].bits;
	return ctx->module.instruments[i - 1].samples[s].data8;
}

void xm_get_playing_speed(xm_context_t* ctx, uint16_t* bpm, uint16_t* tempo) {
	if(bpm) *bpm = ctx->bpm;
	if(tempo) *tempo = ctx->tempo;
}

void xm_get_position(xm_context_t* ctx, uint8_t* pattern_index, uint8_t* pattern, uint8_t* row, uint64_t* samples) {
	if(pattern_index) *pattern_index = ctx->current_table_index;
	if(pattern) *pattern = ctx->module.pattern_table[ctx->current_table_index];
	if(row) *row = ctx->current_row;
	if(samples) *samples = ctx->generated_samples;
}

uint64_t xm_get_latest_trigger_of_instrument(xm_context_t* ctx, uint16_t instr) {
	return ctx->module.instruments[instr - 1].latest_trigger;
}

uint64_t xm_get_latest_trigger_of_sample(xm_context_t* ctx, uint16_t instr, uint16_t sample) {
	return ctx->module.instruments[instr - 1].samples[sample].latest_trigger;
}

uint64_t xm_get_latest_trigger_of_channel(xm_context_t* ctx, uint16_t chn) {
	return ctx->channels[chn - 1].latest_trigger;
}

bool xm_is_channel_active(xm_context_t* ctx, uint16_t chn) {
	xm_channel_context_t* ch = ctx->channels + (chn - 1);
	return ch->instrument != NULL && ch->sample != NULL && ch->sample_position >= 0;
}

float xm_get_frequency_of_channel(xm_context_t* ctx, uint16_t chn) {
	return ctx->channels[chn - 1].frequency;
}

float xm_get_volume_of_channel(xm_context_t* ctx, uint16_t chn) {
	return ctx->channels[chn - 1].actual_volume * ctx->global_volume;
}

float xm_get_panning_of_channel(xm_context_t* ctx, uint16_t chn) {
	return ctx->channels[chn - 1].actual_panning;
}

uint16_t xm_get_instrument_of_channel(xm_context_t* ctx, uint16_t chn) {
	xm_channel_context_t* ch = ctx->channels + (chn - 1);
	if(ch->instrument == NULL) return 0;
	return 1 + (ch->instrument - ctx->module.instruments);
}

// ============================================================================
// XM module loader
// ============================================================================

/* Bounded reader macros.
 * If we attempt to read the buffer out-of-bounds, pretend that the buffer is
 * infinitely padded with zeroes.
 */
#define READ_U8(offset) (((offset) < moddata_length) ? (*(uint8_t*)(moddata + (offset))) : 0)
#define READ_U16(offset) ((uint16_t)READ_U8(offset) | ((uint16_t)READ_U8((offset) + 1) << 8))
#define READ_U32(offset) ((uint32_t)READ_U16(offset) | ((uint32_t)READ_U16((offset) + 2) << 16))
#define READ_MEMCPY(ptr, offset, length) tracker_memcpy_pad(ptr, length, moddata, moddata_length, offset)

int xm_group_sizes_add(xm_context_group_sizes_t *sizes, size_t *field, size_t add)
{
  if (!sizes || !field) return 0;
  if (!tracker_size_add(field, add)) return 0;
  if (!tracker_size_add(&sizes->total_size, add)) return 0;
  return 1;
}

int xm_group_sizes_add_mul(xm_context_group_sizes_t *sizes, size_t *field, size_t a, size_t b)
{
  size_t bytes;

  if (!tracker_size_mul(a, b, &bytes)) return 0;
  return xm_group_sizes_add(sizes, field, bytes);
}

int xm_measure_group_sizes_from_memory(const char *moddata, size_t moddata_length, xm_context_group_sizes_t *sizes)
{
  size_t offset;
  uint16_t num_channels;
  uint16_t num_patterns;
  uint16_t num_instruments;

  if (!moddata || !sizes) return 0;
  memset(sizes, 0, sizeof(*sizes));

  if (!xm_group_sizes_add(sizes, &sizes->context_size, sizeof(xm_context_t))) return 0;

  offset = 60;
  num_channels = READ_U16(offset + 8);
  num_patterns = READ_U16(offset + 10);
  num_instruments = READ_U16(offset + 12);

  if (!xm_group_sizes_add_mul(sizes, &sizes->patterns_size, num_patterns, sizeof(xm_pattern_t))) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->instruments_size, num_instruments, sizeof(xm_instrument_t))) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->runtime_size, num_channels, sizeof(xm_channel_context_t))) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->runtime_size, READ_U16(offset + 4), MAX_NUM_ROWS * sizeof(uint8_t))) return 0;

  offset += READ_U32(offset);

  for (uint16_t i = 0; i < num_patterns; i++)
  {
    uint16_t num_rows = READ_U16(offset + 5);
    uint16_t packed_pattern_size = READ_U16(offset + 7);
    size_t slot_count;

    if (!tracker_size_mul(num_rows, num_channels, &slot_count)) return 0;
    if (!xm_group_sizes_add_mul(sizes, &sizes->patterns_size, slot_count, sizeof(xm_pattern_slot_t))) return 0;
    if (!tracker_size_add(&offset, READ_U32(offset))) return 0;
    if (!tracker_size_add(&offset, packed_pattern_size)) return 0;
  }

  for (uint16_t i = 0; i < num_instruments; i++)
  {
    uint16_t num_samples = READ_U16(offset + 27);
    uint32_t sample_header_size = 0;
    size_t sample_size_aggregate = 0;

    if (!xm_group_sizes_add_mul(sizes, &sizes->instruments_size, num_samples, sizeof(xm_sample_t))) return 0;

    if (num_samples > 0) sample_header_size = READ_U32(offset + 29);
    if (!tracker_size_add(&offset, READ_U32(offset))) return 0;

    for (uint16_t j = 0; j < num_samples; j++)
    {
      uint32_t sample_size = READ_U32(offset);
      if (!tracker_size_add(&sample_size_aggregate, sample_size)) return 0;
      if (!xm_group_sizes_add(sizes, &sizes->samples_size, sample_size)) return 0;
      if (!tracker_size_add(&offset, sample_header_size)) return 0;
    }

    if (!tracker_size_add(&offset, sample_size_aggregate)) return 0;
  }

  return sizes->total_size <= INT_MAX;
}

void *xm_group_cursor_alloc(char **cursor, char *end, size_t size, bool clear)
{
  void *ptr;

  if (!cursor || !*cursor || !end) return NULL;
  if (*cursor > end) return NULL;
  if (size > (size_t)(end - *cursor)) return NULL;

  ptr = *cursor;
  *cursor += size;
  if (clear && size) memset(ptr, 0, size);
  return ptr;
}

void *xm_alloc_and_register_group(xm_context_t *ctx, size_t size, uint8_t type, MFUNC mfunc, bool clear)
{
  void *ptr;

  if (!ctx || !mfunc) return NULL;
  if (!size) return NULL;

  ptr = mfunc(size);
  if (!ptr) return NULL;
  if (clear) memset(ptr, 0, size);

  if (!tracker_context_register_segment(ctx, ptr, size, type))
  {
    free(ptr);
    return NULL;
  }

  return ptr;
}

int xm_allocate_context_groups(xm_context_t **ctxp, xm_context_group_cursor_t *cursor, const xm_context_group_sizes_t *sizes, MFUNC mfunc)
{
  xm_context_t *ctx;
  void *patterns = NULL;
  void *instruments = NULL;
  void *samples = NULL;
  void *runtime = NULL;
  void *format_extra = NULL;

  if (ctxp) *ctxp = NULL;
  if (!ctxp || !cursor || !sizes || !mfunc) return 0;
  memset(cursor, 0, sizeof(*cursor));

  ctx = (xm_context_t*)mfunc(sizeof(xm_context_t));
  if (!ctx) return 0;
  memset(ctx, 0, sizeof(*ctx));
  ctx->ctx_size = sizes->total_size;

  if (!tracker_context_register_segment(ctx, ctx, sizeof(xm_context_t), TRACKER_CONTEXT_SEG_CONTEXT))
  {
    free(ctx);
    return 0;
  }

  *ctxp = ctx;

  patterns = xm_alloc_and_register_group(ctx, sizes->patterns_size, TRACKER_CONTEXT_SEG_PATTERNS, mfunc, true);
  if (sizes->patterns_size && !patterns) goto fail;

  instruments = xm_alloc_and_register_group(ctx, sizes->instruments_size, TRACKER_CONTEXT_SEG_INSTRUMENTS, mfunc, true);
  if (sizes->instruments_size && !instruments) goto fail;

  samples = xm_alloc_and_register_group(ctx, sizes->samples_size, TRACKER_CONTEXT_SEG_SAMPLES, mfunc, false);
  if (sizes->samples_size && !samples) goto fail;

  runtime = xm_alloc_and_register_group(ctx, sizes->runtime_size, TRACKER_CONTEXT_SEG_RUNTIME, mfunc, true);
  if (sizes->runtime_size && !runtime) goto fail;

  format_extra = xm_alloc_and_register_group(ctx, sizes->format_extra_size, TRACKER_CONTEXT_SEG_FORMAT_EXTRA, mfunc, true);
  if (sizes->format_extra_size && !format_extra) goto fail;

  cursor->patterns = (char*)patterns;
  cursor->patterns_end = cursor->patterns ? cursor->patterns + sizes->patterns_size : NULL;
  cursor->instruments = (char*)instruments;
  cursor->instruments_end = cursor->instruments ? cursor->instruments + sizes->instruments_size : NULL;
  cursor->samples = (char*)samples;
  cursor->samples_end = cursor->samples ? cursor->samples + sizes->samples_size : NULL;
  cursor->runtime = (char*)runtime;
  cursor->runtime_end = cursor->runtime ? cursor->runtime + sizes->runtime_size : NULL;
  cursor->format_extra = (char*)format_extra;
  cursor->format_extra_end = cursor->format_extra ? cursor->format_extra + sizes->format_extra_size : NULL;

  return 1;

fail:
  xm_free_context(ctx);
  *ctxp = NULL;
  memset(cursor, 0, sizeof(*cursor));
  return 0;
}

int xm_setup_runtime_group(xm_context_t *ctx, xm_context_group_cursor_t *cursor, uint32_t rate)
{
  if (!ctx || !cursor) return 0;

  ctx->channels = (xm_channel_context_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, ctx->module.num_channels * sizeof(xm_channel_context_t), true);
  if (!ctx->channels) return 0;

  ctx->rate = rate;
  ctx->global_volume = 1.f;
  ctx->amplification = .25f;

#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  for (uint8_t i = 0; i < ctx->module.num_channels; i++)
  {
    xm_channel_context_t *ch = ctx->channels + i;
    ch->ping = true;
    ch->vibrato_waveform = XM_SINE_WAVEFORM;
    ch->vibrato_waveform_retrigger = true;
    ch->tremolo_waveform = XM_SINE_WAVEFORM;
    ch->tremolo_waveform_retrigger = true;
    ch->panbrello_waveform = XM_SINE_WAVEFORM;
    ch->panbrello_waveform_retrigger = true;
    ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
    ch->channel_volume = 1.0f;
    ch->panning = ch->panning_envelope_panning = .5f;
    ch->default_panning = -1.f;
    ch->tracker_format = TRACKER_FORMAT_XM;
    ch->period_note_offset = .0f;
    ch->actual_volume = .0f;
    ch->actual_panning = .5f;
  }

  ctx->row_loop_count = (uint8_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t), true);
  if (!ctx->row_loop_count) return 0;

  return 1;
}

int xm_check_sanity_preload(const char* module, size_t module_length) {
	if(module_length < 60) {
		return 4;
	}

	if(memcmp("Extended Module: ", module, 17) != 0) {
		return 1;
	}

	if(module[37] != 0x1A) {
		return 2;
	}

	if(module[59] != 0x01 || module[58] != 0x04) {
		/* Not XM 1.04 */
		return 3;
	}

	return 0;
}

int xm_check_sanity_postload(xm_context_t* ctx) {
	/* @todo: plenty of stuff to do here… */

	/* Check the POT */
	for(uint8_t i = 0; i < ctx->module.length; ++i) {
		if(ctx->module.pattern_table[i] >= ctx->module.num_patterns) {
			if(i+1 == ctx->module.length && ctx->module.length > 1) {
				/* Cheap fix */
				--ctx->module.length;
				DEBUG("trimming invalid POT at pos %X", i);
			} else {
				DEBUG("module has invalid POT, pos %X references nonexistent pattern %X",
				      i,
				      ctx->module.pattern_table[i]);
				return 1;
			}
		}
	}

	return 0;
}

size_t xm_get_memory_needed_for_context(const char* moddata, size_t moddata_length)
{
  xm_context_group_sizes_t sizes;

  if (!xm_measure_group_sizes_from_memory(moddata, moddata_length, &sizes)) return 0;
  return sizes.total_size;
}

int xm_load_module(xm_context_t* ctx, const char* moddata, size_t moddata_length, xm_context_group_cursor_t *cursor) {
	size_t offset = 0;
	xm_module_t* mod = &(ctx->module);

	ctx->tracker_format = TRACKER_FORMAT_XM;

	/* Read XM header */
#if XM_STRINGS
	READ_MEMCPY(mod->name, offset + 17, MODULE_NAME_LENGTH);
	READ_MEMCPY(mod->trackername, offset + 38, TRACKER_NAME_LENGTH);
#endif
	offset += 60;

	/* Read module header */
	uint32_t header_size = READ_U32(offset);

	mod->length = READ_U16(offset + 4);
	mod->restart_position = READ_U16(offset + 6);
	mod->num_channels = READ_U16(offset + 8);
	mod->num_patterns = READ_U16(offset + 10);
	mod->num_instruments = READ_U16(offset + 12);

	size_t patterns_size = mod->num_patterns * sizeof(xm_pattern_t);
	mod->patterns = patterns_size ? (xm_pattern_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, patterns_size, true) : NULL;
	if(patterns_size && !mod->patterns) return 0;

	size_t instruments_size = mod->num_instruments * sizeof(xm_instrument_t);
	mod->instruments = instruments_size ? (xm_instrument_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, instruments_size, true) : NULL;
	if(instruments_size && !mod->instruments) return 0;

	uint16_t flags = READ_U32(offset + 14);
	mod->frequency_type = (flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;

	ctx->tempo = READ_U16(offset + 16);
	ctx->bpm = READ_U16(offset + 18);

	READ_MEMCPY(mod->pattern_table, offset + 20, PATTERN_ORDER_TABLE_LENGTH);
	offset += header_size;

	/* Read patterns */
	for(uint16_t i = 0; i < mod->num_patterns; ++i) {
		uint16_t packed_patterndata_size = READ_U16(offset + 7);
		xm_pattern_t* pat = mod->patterns + i;

		pat->num_rows = READ_U16(offset + 5);

		size_t slots_size = mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t);
		pat->slots = slots_size ? (xm_pattern_slot_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, slots_size, true) : NULL;
		if(slots_size && !pat->slots) return 0;

		/* Pattern header length */
		offset += READ_U32(offset);

		if(packed_patterndata_size == 0) {
			/* No pattern data is present */
		} else {
			/* This isn't your typical for loop */
			for(uint16_t j = 0, k = 0; j < packed_patterndata_size; ++k) {
				uint8_t note = READ_U8(offset + j);
				xm_pattern_slot_t* slot = pat->slots + k;

				if(note & (1 << 7)) {
					/* MSB is set, this is a compressed packet */
					++j;

					if(note & (1 << 0)) {
						/* Note follows */
						slot->note = READ_U8(offset + j);
						++j;
					} else {
						slot->note = 0;
					}

					if(note & (1 << 1)) {
						/* Instrument follows */
						slot->instrument = READ_U8(offset + j);
						++j;
					} else {
						slot->instrument = 0;
					}

					if(note & (1 << 2)) {
						/* Volume column follows */
						slot->volume_column = READ_U8(offset + j);
						++j;
					} else {
						slot->volume_column = 0;
					}

					if(note & (1 << 3)) {
						/* Effect follows */
						slot->effect_type = READ_U8(offset + j);
						++j;
					} else {
						slot->effect_type = 0;
					}

					if(note & (1 << 4)) {
						/* Effect parameter follows */
						slot->effect_param = READ_U8(offset + j);
						++j;
					} else {
						slot->effect_param = 0;
					}
				} else {
					/* Uncompressed packet */
					slot->note = note;
					slot->instrument = READ_U8(offset + j + 1);
					slot->volume_column = READ_U8(offset + j + 2);
					slot->effect_type = READ_U8(offset + j + 3);
					slot->effect_param = READ_U8(offset + j + 4);
					j += 5;
				}
			}
		}

		offset += packed_patterndata_size;
	}

	/* Read instruments */
	for(uint16_t i = 0; i < ctx->module.num_instruments; ++i) {
		uint32_t sample_header_size = 0;
		xm_instrument_t* instr = mod->instruments + i;

#if XM_STRINGS
		READ_MEMCPY(instr->name, offset + 4, INSTRUMENT_NAME_LENGTH);
#endif
	    instr->num_samples = READ_U16(offset + 27);

		if(instr->num_samples > 0) {
			/* Read extra header properties */
			sample_header_size = READ_U32(offset + 29);
			READ_MEMCPY(instr->sample_of_notes, offset + 33, NUM_NOTES);

			instr->volume_envelope.num_points = READ_U8(offset + 225);
			instr->panning_envelope.num_points = READ_U8(offset + 226);

			for(uint8_t j = 0; j < instr->volume_envelope.num_points; ++j) {
				instr->volume_envelope.points[j].frame = READ_U16(offset + 129 + 4 * j);
				instr->volume_envelope.points[j].value = READ_U16(offset + 129 + 4 * j + 2);
			}

			for(uint8_t j = 0; j < instr->panning_envelope.num_points; ++j) {
				instr->panning_envelope.points[j].frame = READ_U16(offset + 177 + 4 * j);
				instr->panning_envelope.points[j].value = READ_U16(offset + 177 + 4 * j + 2);
			}

			instr->volume_envelope.sustain_point = READ_U8(offset + 227);
			instr->volume_envelope.loop_start_point = READ_U8(offset + 228);
			instr->volume_envelope.loop_end_point = READ_U8(offset + 229);

			instr->panning_envelope.sustain_point = READ_U8(offset + 230);
			instr->panning_envelope.loop_start_point = READ_U8(offset + 231);
			instr->panning_envelope.loop_end_point = READ_U8(offset + 232);

			uint8_t flags = READ_U8(offset + 233);
			instr->volume_envelope.enabled = flags & (1 << 0);
			instr->volume_envelope.sustain_enabled = flags & (1 << 1);
			instr->volume_envelope.loop_enabled = flags & (1 << 2);

			flags = READ_U8(offset + 234);
			instr->panning_envelope.enabled = flags & (1 << 0);
			instr->panning_envelope.sustain_enabled = flags & (1 << 1);
			instr->panning_envelope.loop_enabled = flags & (1 << 2);

			instr->vibrato_type = (xm_waveform_type_t)READ_U8(offset + 235);
			if(instr->vibrato_type == 2) {
				instr->vibrato_type = XM_RAMP_DOWN_WAVEFORM;
			} else if(instr->vibrato_type == 1) {
				instr->vibrato_type = XM_SQUARE_WAVEFORM;
			}
			instr->vibrato_sweep = READ_U8(offset + 236);
			instr->vibrato_depth = READ_U8(offset + 237);
			instr->vibrato_rate = READ_U8(offset + 238);
			instr->volume_fadeout = READ_U16(offset + 239);

			instr->samples = (xm_sample_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, instr->num_samples * sizeof(xm_sample_t), true);
			if(!instr->samples) return 0;
		} else {
			instr->samples = NULL;
		}

		/* Instrument header size */
		offset += READ_U32(offset);

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample header */
			xm_sample_t* sample = instr->samples + j;

			sample->length = READ_U32(offset);
			sample->loop_start = READ_U32(offset + 4);
			sample->loop_length = READ_U32(offset + 8);
			sample->loop_end = sample->loop_start + sample->loop_length;
			sample->volume = (float)READ_U8(offset + 12) / (float)0x40;
			sample->finetune = (int8_t)READ_U8(offset + 13);

			uint8_t flags = READ_U8(offset + 14);
			if((flags & 3) == 0) {
				sample->loop_type = XM_NO_LOOP;
			} else if((flags & 3) == 1) {
				sample->loop_type = XM_FORWARD_LOOP;
			} else {
				sample->loop_type = XM_PING_PONG_LOOP;
			}

			sample->bits = (flags & (1 << 4)) ? 16 : 8;

			sample->panning = (float)READ_U8(offset + 15) / (float)0xFF;
			sample->relative_note = (int8_t)READ_U8(offset + 16);
#if XM_STRINGS
			READ_MEMCPY(sample->name, offset + 18, SAMPLE_NAME_LENGTH);
#endif
			if(sample->length > 0) {
				sample->data8 = (int8_t*)xm_group_cursor_alloc(&cursor->samples, cursor->samples_end, sample->length, false);
				if(!sample->data8) return 0;
			} else {
				sample->data8 = NULL;
			}

			if(sample->bits == 16) {
				sample->loop_start >>= 1;
				sample->loop_length >>= 1;
				sample->loop_end >>= 1;
				sample->length >>= 1;
			}

			offset += sample_header_size;
		}

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample data */
			xm_sample_t* sample = instr->samples + j;
			uint32_t length = sample->length;

			if(sample->bits == 16) {
				int16_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					v = v + (int16_t)READ_U16(offset + (k << 1));
					sample->data16[k] = v;
				}
				offset += sample->length << 1;
			} else {
				int8_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					v = v + (int8_t)READ_U8(offset + k);
					sample->data8[k] = v;
				}
				offset += sample->length;
			}
		}
	}

	return 1;
}

// ============================================================================
// MOD module loader
// ============================================================================

#define MOD_STREAM_READ_OK 0

int mod_read_layout(const void *moddata, size_t moddata_length, mod_layout_t *layout);

uint16_t mod_channels_from_magic(const uint8_t *magic)
{
  if (!magic) return 0;

  if (!memcmp(magic, "M.K.", 4)) return 4;
  if (!memcmp(magic, "M!K!", 4)) return 4;
  if (!memcmp(magic, "M&K!", 4)) return 4;
  if (!memcmp(magic, "N.T.", 4)) return 4;
  if (!memcmp(magic, "FLT4", 4)) return 4;
  if (!memcmp(magic, "EXO4", 4)) return 4;

  if (magic[0] >= '1' && magic[0] <= '9' && magic[1] == 'C' && magic[2] == 'H' && magic[3] == 'N')
    return magic[0] - '0';

  if (magic[0] >= '0' && magic[0] <= '9' && magic[1] >= '0' && magic[1] <= '9')
  {
    uint16_t channels = (uint16_t)((magic[0] - '0') * 10 + (magic[1] - '0'));
    if ((magic[2] == 'C' && magic[3] == 'H') || (magic[2] == 'C' && magic[3] == 'N'))
      return channels;
  }

  return 0;
}

uint16_t mod_get_channel_count(const void *moddata, size_t moddata_length)
{
  mod_layout_t layout;

  if (!mod_read_layout(moddata, moddata_length, &layout)) return 0;
  return layout.channels;
}

int mod_finish_layout(const uint8_t *header, size_t header_length, size_t moddata_length, mod_layout_t *layout)
{
  uint8_t max_pattern = 0;
  uint8_t max_table_pattern = 0;
  uint16_t min_patterns;
  uint16_t table_patterns;
  size_t pattern_end;
  size_t sample_avail;
  size_t order_offset;
  size_t pattern_block_bytes;

  if (!header || !layout) return 0;
  if (!layout->sample_count || layout->sample_count > MOD_SAMPLE_COUNT) return 0;
  if (!layout->header_size || header_length < layout->header_size) return 0;
  if (moddata_length < layout->header_size) return 0;
  if (!layout->channels || layout->channels > MOD_MAX_CHANNELS) return 0;
  if (!layout->song_length || layout->song_length > MOD_ORDER_COUNT) return 0;

  if (layout->restart_position >= layout->song_length) layout->restart_position = 0;

  order_offset = layout->sample_count == MOD_SAMPLE_COUNT ? MOD_ORDER_OFFSET : MOD_15_ORDER_OFFSET;
  for (uint16_t i = 0; i < layout->song_length; i++)
  {
    uint8_t pat = header[order_offset + i];
    if (pat >= MOD_ORDER_COUNT) return 0;
    if (pat > max_pattern) max_pattern = pat;
  }

  for (uint16_t i = 0; i < MOD_ORDER_COUNT; i++)
  {
    uint8_t pat = header[order_offset + i];
    if (pat < MOD_ORDER_COUNT && pat > max_table_pattern) max_table_pattern = pat;
  }

  min_patterns = (uint16_t)max_pattern + 1;
  table_patterns = (uint16_t)max_table_pattern + 1;
  if (table_patterns < min_patterns) table_patterns = min_patterns;
  if (!min_patterns) return 0;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    size_t off = MOD_TITLE_SIZE + (size_t)i * MOD_SAMPLE_HEADER_SIZE;
    uint32_t length = (uint32_t)tracker_rd_be16(header + off + 22) * 2u;
    uint32_t loop_start = (uint32_t)tracker_rd_be16(header + off + 26) * 2u;
    uint32_t loop_length = (uint32_t)tracker_rd_be16(header + off + 28) * 2u;

    layout->sample_length[i] = length;
    layout->sample_loop_start[i] = loop_start;
    layout->sample_loop_length[i] = loop_length;

    if (!tracker_size_add(&layout->sample_bytes, length)) return 0;
  }

  if (!tracker_size_mul(MOD_ROWS_PER_PATTERN, layout->channels, &pattern_block_bytes)) return 0;
  if (!tracker_size_mul(pattern_block_bytes, 4, &pattern_block_bytes)) return 0;

  layout->patterns = min_patterns;
  if (moddata_length >= layout->header_size && moddata_length - layout->header_size >= layout->sample_bytes)
  {
    size_t pattern_area = moddata_length - layout->header_size - layout->sample_bytes;
    size_t file_patterns = pattern_area / pattern_block_bytes;
    size_t pattern_tail = pattern_area % pattern_block_bytes;

    if (pattern_block_bytes && !pattern_tail && file_patterns >= min_patterns && file_patterns <= MOD_ORDER_COUNT)
      layout->patterns = (uint16_t)file_patterns;
    else if (pattern_block_bytes && table_patterns > min_patterns && file_patterns >= table_patterns && table_patterns <= MOD_ORDER_COUNT)
      layout->patterns = table_patterns;
  }

  if (!tracker_size_mul(layout->patterns, pattern_block_bytes, &layout->pattern_bytes)) return 0;

  layout->sample_data_offset = layout->pattern_data_offset;
  if (!tracker_size_add(&layout->sample_data_offset, layout->pattern_bytes)) return 0;
  pattern_end = layout->sample_data_offset;
  if (moddata_length < pattern_end) return 0;

  sample_avail = moddata_length - pattern_end;
  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    uint32_t length = layout->sample_length[i];
    uint32_t file_length = length;

    if ((size_t)file_length > sample_avail) file_length = (uint32_t)sample_avail;
    layout->sample_file_length[i] = file_length;
    sample_avail -= file_length;
  }

  layout->expected_size = layout->sample_data_offset;
  if (!tracker_size_add(&layout->expected_size, layout->sample_bytes)) return 0;

  return 1;
}

int mod_read_layout_from_header(const uint8_t *header, size_t header_length, size_t moddata_length, mod_layout_t *layout)
{
  uint16_t channels;

  if (!header || !layout) return 0;
  if (header_length < MOD_15_HEADER_SIZE) return 0;

  memset(layout, 0, sizeof(*layout));

  if (header_length >= MOD_HEADER_SIZE)
  {
    channels = mod_channels_from_magic(header + MOD_MAGIC_OFFSET);
    if (channels)
    {
      layout->channels = channels;
      layout->sample_count = MOD_SAMPLE_COUNT;
      layout->header_size = MOD_HEADER_SIZE;
      layout->pattern_data_offset = MOD_HEADER_SIZE;
      layout->song_length = header[MOD_SONG_LENGTH_OFFSET];
      layout->restart_position = header[MOD_RESTART_OFFSET];
      if (mod_finish_layout(header, header_length, moddata_length, layout)) return 1;
    }
  }

  memset(layout, 0, sizeof(*layout));
  layout->channels = 4;
  layout->sample_count = MOD_15_SAMPLE_COUNT;
  layout->header_size = MOD_15_HEADER_SIZE;
  layout->pattern_data_offset = MOD_15_HEADER_SIZE;
  layout->song_length = header[MOD_15_SONG_LENGTH_OFFSET];
  layout->restart_position = header[MOD_15_RESTART_OFFSET];
  if (mod_finish_layout(header, header_length, moddata_length, layout)) return 1;

  memset(layout, 0, sizeof(*layout));
  return 0;
}

int mod_read_layout(const void *moddata, size_t moddata_length, mod_layout_t *layout)
{
  const uint8_t *data = (const uint8_t*)moddata;

  if (!data || moddata_length < MOD_15_HEADER_SIZE) return 0;
  return mod_read_layout_from_header(data, moddata_length, moddata_length, layout);
}

int mod_check_sanity_preload(const void *moddata, size_t moddata_length)
{
  mod_layout_t layout;

  return mod_read_layout(moddata, moddata_length, &layout) ? 0 : 1;
}
uint32_t mod_effective_loop_length(const mod_layout_t *layout, uint16_t sample_index, uint32_t *out_loop_start)
{
  uint32_t length;
  uint32_t loop_start;
  uint32_t loop_length;

  if (!layout || sample_index >= layout->sample_count) return 0;

  length = layout->sample_length[sample_index];
  loop_start = layout->sample_loop_start[sample_index];
  loop_length = layout->sample_loop_length[sample_index];

  if (loop_start >= length || loop_length <= 2) return 0;
  if (loop_length > length - loop_start) loop_length = length - loop_start;
  if (loop_length <= 2) return 0;

  if (out_loop_start) *out_loop_start = loop_start;
  return loop_length;
}

int mod_layout_set_efx_backup_mask(mod_layout_t *layout, uint32_t sample_mask)
{
  size_t bytes = 0;
  uint16_t count = 0;
  uint32_t mask = 0;

  if (!layout) return 0;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    uint32_t loop_length;

    if (!(sample_mask & (1u << i))) continue;

    loop_length = mod_effective_loop_length(layout, i, NULL);
    if (!loop_length) continue;

    if (!tracker_size_add(&bytes, loop_length)) return 0;
    count++;
    mask |= 1u << i;
  }

  layout->efx_sample_mask = mask;
  layout->efx_backup_count = count;
  layout->efx_backup_bytes = bytes;
  return 1;
}

int mod_layout_set_efx_backup_all_looped(mod_layout_t *layout)
{
  uint32_t sample_mask = 0;

  if (!layout) return 0;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    if (mod_effective_loop_length(layout, i, NULL)) sample_mask |= 1u << i;
  }

  return mod_layout_set_efx_backup_mask(layout, sample_mask);
}

void mod_scan_efx_apply_slot(const mod_layout_t *layout, uint8_t *current_sample, uint8_t *efx_speed, uint16_t channel, const xm_pattern_slot_t *slot, uint32_t *sample_mask)
{
  uint8_t sample_index;

  if (!layout || !current_sample || !efx_speed || !slot || !sample_mask) return;
  if (channel >= layout->channels || channel >= MOD_MAX_CHANNELS) return;

  if (slot->instrument > 0 && slot->instrument <= layout->sample_count)
    current_sample[channel] = slot->instrument;

  if (slot->effect_type == 0xE && (slot->effect_param & 0xF0) == 0xF0)
    efx_speed[channel] = slot->effect_param & 0x0F;

  if (!efx_speed[channel] || !current_sample[channel]) return;

  sample_index = current_sample[channel] - 1;
  if (sample_index >= layout->sample_count) return;
  if (!mod_effective_loop_length(layout, sample_index, NULL)) return;

  *sample_mask |= 1u << sample_index;
}

uint32_t mod_scan_efx_sample_mask_from_context(const xm_context_t *ctx, const mod_layout_t *layout)
{
  uint8_t current_sample[MOD_MAX_CHANNELS] = {0};
  uint8_t efx_speed[MOD_MAX_CHANNELS] = {0};
  uint32_t sample_mask = 0;

  if (!ctx || !layout || !ctx->module.patterns) return 0;
  if (!layout->channels || layout->channels > MOD_MAX_CHANNELS) return 0;

  for (uint16_t order = 0; order < ctx->module.length; order++)
  {
    uint8_t pat_index = ctx->module.pattern_table[order];
    xm_pattern_t *pat;

    if (pat_index >= ctx->module.num_patterns) continue;
    pat = ctx->module.patterns + pat_index;
    if (!pat->slots) continue;

    for (uint16_t row = 0; row < pat->num_rows; row++)
    {
      for (uint16_t ch = 0; ch < ctx->module.num_channels; ch++)
      {
        xm_pattern_slot_t *slot = pat->slots + (size_t)row * ctx->module.num_channels + ch;
        mod_scan_efx_apply_slot(layout, current_sample, efx_speed, ch, slot, &sample_mask);
      }
    }
  }

  return sample_mask;
}

uint32_t mod_scan_efx_sample_mask_from_data(const void *moddata, const mod_layout_t *layout)
{
  const uint8_t *data = (const uint8_t*)moddata;
  uint8_t current_sample[MOD_MAX_CHANNELS] = {0};
  uint8_t efx_speed[MOD_MAX_CHANNELS] = {0};
  uint32_t sample_mask = 0;
  size_t order_offset;

  if (!data || !layout) return 0;
  if (!layout->channels || layout->channels > MOD_MAX_CHANNELS) return 0;

  order_offset = layout->sample_count == MOD_SAMPLE_COUNT ? MOD_ORDER_OFFSET : MOD_15_ORDER_OFFSET;
  for (uint16_t order = 0; order < layout->song_length; order++)
  {
    uint8_t pat_index = data[order_offset + order];

    if (pat_index >= layout->patterns) continue;

    for (uint16_t row = 0; row < MOD_ROWS_PER_PATTERN; row++)
    {
      for (uint16_t ch = 0; ch < layout->channels; ch++)
      {
        xm_pattern_slot_t slot;
        size_t off = layout->pattern_data_offset + (((size_t)pat_index * MOD_ROWS_PER_PATTERN + row) * layout->channels + ch) * 4;

        memset(&slot, 0, sizeof(slot));
        mod_decode_pattern_slot(&slot, data + off);
        mod_scan_efx_apply_slot(layout, current_sample, efx_speed, ch, &slot, &sample_mask);
      }
    }
  }

  return sample_mask;
}

int mod_get_group_sizes_for_layout(const mod_layout_t *layout, xm_context_group_sizes_t *sizes)
{
  size_t v;

  if (!layout || !sizes) return 0;
  memset(sizes, 0, sizeof(*sizes));

  if (!xm_group_sizes_add(sizes, &sizes->context_size, sizeof(xm_context_t))) return 0;

  if (!xm_group_sizes_add_mul(sizes, &sizes->patterns_size, layout->patterns, sizeof(xm_pattern_t))) return 0;

  if (!tracker_size_mul(layout->patterns, MOD_ROWS_PER_PATTERN, &v)) return 0;
  if (!tracker_size_mul(v, layout->channels, &v)) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->patterns_size, v, sizeof(xm_pattern_slot_t))) return 0;

  if (!xm_group_sizes_add_mul(sizes, &sizes->instruments_size, layout->sample_count, sizeof(xm_instrument_t))) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->instruments_size, layout->sample_count, sizeof(xm_sample_t))) return 0;

  if (!tracker_size_add(&sizes->total_size, layout->sample_bytes)) return 0;

  if (!xm_group_sizes_add_mul(sizes, &sizes->runtime_size, layout->channels, sizeof(xm_channel_context_t))) return 0;
  if (!tracker_size_mul(layout->song_length, MAX_NUM_ROWS, &v)) return 0;
  if (!xm_group_sizes_add_mul(sizes, &sizes->runtime_size, v, sizeof(uint8_t))) return 0;

  if (layout->efx_backup_count)
  {
    if (!xm_group_sizes_add_mul(sizes, &sizes->format_extra_size, layout->efx_backup_count, sizeof(xm_mod_efx_backup_t))) return 0;
    if (!xm_group_sizes_add(sizes, &sizes->format_extra_size, layout->efx_backup_bytes)) return 0;
  }

  return sizes->total_size <= INT_MAX;
}

size_t mod_get_memory_needed_for_layout(const mod_layout_t *layout)
{
  xm_context_group_sizes_t sizes;

  if (!mod_get_group_sizes_for_layout(layout, &sizes)) return 0;
  return sizes.total_size;
}

size_t mod_get_memory_needed_for_context(const void *moddata, size_t moddata_length)
{
  mod_layout_t layout;

  if (!mod_read_layout(moddata, moddata_length, &layout)) return 0;
  if (!mod_layout_set_efx_backup_mask(&layout, mod_scan_efx_sample_mask_from_data(moddata, &layout))) return 0;
  return mod_get_memory_needed_for_layout(&layout);
}

void mod_init_channel(xm_channel_context_t *ch, uint16_t channel)
{
  float panning;

  if (!ch) return;

  panning = ((channel & 3) == 1 || (channel & 3) == 2) ? 0.75f : 0.25f;

  ch->ping = true;
  ch->vibrato_waveform = XM_SINE_WAVEFORM;
  ch->vibrato_waveform_retrigger = true;
  ch->tremolo_waveform = XM_SINE_WAVEFORM;
  ch->tremolo_waveform_retrigger = true;
  ch->panbrello_waveform = XM_SINE_WAVEFORM;
  ch->panbrello_waveform_retrigger = true;

  ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
  ch->channel_volume = 1.0f;
  ch->panning = panning;
  ch->default_panning = panning;
  ch->tracker_format = TRACKER_FORMAT_MOD;
  ch->period_note_offset = .0f;
  ch->panning_envelope_panning = .5f;
  ch->actual_volume = .0f;
  ch->actual_panning = panning;
#if XM_RAMPING
  ch->target_volume = .0f;
  ch->target_panning = panning;
#endif
}

void mod_decode_pattern_slot(xm_pattern_slot_t *slot, const uint8_t *entry)
{
  uint8_t b0;
  uint8_t b1;
  uint8_t b2;
  uint8_t b3;

  if (!slot || !entry) return;

  b0 = entry[0];
  b1 = entry[1];
  b2 = entry[2];
  b3 = entry[3];

  slot->period = (uint16_t)(((uint16_t)(b0 & 0x0F) << 8) | b1);
  slot->note = 0;
  slot->instrument = (uint8_t)((b0 & 0xF0) | (b2 >> 4));
  slot->volume_column = 0;
  slot->effect_type = b2 & 0x0F;
  slot->effect_param = b3;
}

int mod_setup_module_header(xm_context_t *ctx, const uint8_t *header, const mod_layout_t *layout, xm_context_group_cursor_t *cursor)
{
  xm_module_t *mod;

  if (!ctx || !header || !layout || !cursor) return 0;

  mod = &ctx->module;
  ctx->tracker_format = TRACKER_FORMAT_MOD;

#if XM_STRINGS
  tracker_copy_trimmed(mod->name, sizeof(mod->name), header, MOD_TITLE_SIZE);
  strncpy(mod->trackername, "ProTracker MOD", sizeof(mod->trackername) - 1);
#endif

  mod->length = layout->song_length;
  mod->restart_position = layout->restart_position;
  mod->num_channels = layout->channels;
  mod->num_patterns = layout->patterns;
  mod->num_instruments = layout->sample_count;
  mod->frequency_type = XM_AMIGA_FREQUENCIES;

  memcpy(mod->pattern_table, header + (layout->sample_count == MOD_SAMPLE_COUNT ? MOD_ORDER_OFFSET : MOD_15_ORDER_OFFSET), layout->song_length);

  mod->patterns = (xm_pattern_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, mod->num_patterns * sizeof(xm_pattern_t), true);
  if (!mod->patterns) return 0;

  mod->instruments = (xm_instrument_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, mod->num_instruments * sizeof(xm_instrument_t), true);
  if (!mod->instruments) return 0;

  return 1;
}

int mod_setup_sample_metadata(xm_context_t *ctx, const uint8_t *header, const mod_layout_t *layout, uint16_t i, xm_context_group_cursor_t *cursor, MFUNC mfunc)
{
  xm_module_t *mod;
  size_t header_off;
  xm_instrument_t *instr;
  xm_sample_t *sample;
  uint8_t volume;
  uint8_t finetune;
  uint32_t loop_start;
  uint32_t loop_length;

  if (!ctx || !header || !layout || !cursor || !mfunc || i >= layout->sample_count) return 0;

  mod = &ctx->module;
  header_off = MOD_TITLE_SIZE + (size_t)i * MOD_SAMPLE_HEADER_SIZE;
  instr = mod->instruments + i;
  volume = header[header_off + 25];
  finetune = header[header_off + 24] & 0x0F;
  loop_start = layout->sample_loop_start[i];
  loop_length = layout->sample_loop_length[i];

#if XM_STRINGS
  tracker_copy_trimmed(instr->name, sizeof(instr->name), header + header_off, 22);
#endif
  instr->num_samples = 1;
  instr->samples = (xm_sample_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, sizeof(xm_sample_t), true);
  if (!instr->samples) return 0;

  sample = instr->samples;
#if XM_STRINGS
  tracker_copy_trimmed(sample->name, sizeof(sample->name), header + header_off, 22);
#endif
  sample->bits = 8;
  sample->length = layout->sample_length[i];
  sample->volume = (float)(volume > 64 ? 64 : volume) / 64.f;
  sample->finetune = (int8_t)((finetune < 8 ? finetune : finetune - 16) << 4);
  sample->panning = .5f;
  sample->relative_note = 0;
  sample->data8 = sample->length ? (int8_t*)xm_alloc_and_register_group(ctx, sample->length, TRACKER_CONTEXT_SEG_SAMPLES, mfunc, false) : NULL;
  if (sample->length && !sample->data8) return 0;

  if (loop_start >= sample->length || loop_length <= 2)
  {
    sample->loop_start = 0;
    sample->loop_length = 0;
    sample->loop_end = 0;
    sample->loop_type = XM_NO_LOOP;
  }
  else
  {
    if (loop_length > sample->length - loop_start)
      loop_length = sample->length - loop_start;

    if (loop_length <= 2)
    {
      sample->loop_start = 0;
      sample->loop_length = 0;
      sample->loop_end = 0;
      sample->loop_type = XM_NO_LOOP;
    }
    else
    {
      sample->loop_start = loop_start;
      sample->loop_length = loop_length;
      sample->loop_end = loop_start + loop_length;
      sample->loop_type = XM_FORWARD_LOOP;
    }
  }

  return 1;
}

int mod_load_module(xm_context_t *ctx, const void *moddata, const mod_layout_t *layout, xm_context_group_cursor_t *cursor, MFUNC mfunc)
{
  const uint8_t *data = (const uint8_t*)moddata;
  xm_module_t *mod;
  size_t pattern_offset;
  size_t sample_offset;

  if (!ctx || !data || !layout || !cursor || !mfunc) return 0;

  mod = &ctx->module;
  if (!mod_setup_module_header(ctx, data, layout, cursor)) return 0;

  pattern_offset = layout->pattern_data_offset;
  for (uint16_t pat_i = 0; pat_i < mod->num_patterns; pat_i++)
  {
    xm_pattern_t *pat = mod->patterns + pat_i;

    pat->num_rows = MOD_ROWS_PER_PATTERN;
    pat->slots = (xm_pattern_slot_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, MOD_ROWS_PER_PATTERN * mod->num_channels * sizeof(xm_pattern_slot_t), true);
    if (!pat->slots) return 0;

    for (uint16_t row = 0; row < MOD_ROWS_PER_PATTERN; row++)
    {
      for (uint16_t ch = 0; ch < mod->num_channels; ch++)
      {
        size_t off = pattern_offset + ((size_t)row * mod->num_channels + ch) * 4;
        xm_pattern_slot_t *slot = pat->slots + (size_t)row * mod->num_channels + ch;

        mod_decode_pattern_slot(slot, data + off);
      }
    }

    pattern_offset += (size_t)MOD_ROWS_PER_PATTERN * mod->num_channels * 4;
  }

  sample_offset = layout->sample_data_offset;
  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    xm_sample_t *sample;

    if (!mod_setup_sample_metadata(ctx, data, layout, i, cursor, mfunc)) return 0;

    sample = mod->instruments[i].samples;
    if (layout->sample_file_length[i])
      memcpy(sample->data8, data + sample_offset, layout->sample_file_length[i]);
    if (layout->sample_file_length[i] < sample->length)
      memset(sample->data8 + layout->sample_file_length[i], 0, sample->length - layout->sample_file_length[i]);
    sample_offset += layout->sample_file_length[i];
  }

  return 1;
}

int mod_setup_context_runtime(xm_context_t *ctx, xm_context_group_cursor_t *cursor, uint32_t rate)
{
  if (!ctx || !cursor) return 0;

  ctx->channels = (xm_channel_context_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, ctx->module.num_channels * sizeof(xm_channel_context_t), true);
  if (!ctx->channels) return 0;

  ctx->row_loop_count = (uint8_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t), true);
  if (!ctx->row_loop_count) return 0;

  ctx->tempo = MOD_DEFAULT_TEMPO;
  ctx->bpm = MOD_DEFAULT_BPM;
  ctx->global_volume = 1.f;
  ctx->amplification = .25f;
  ctx->rate = rate;
#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  for (uint16_t i = 0; i < ctx->module.num_channels; i++)
    mod_init_channel(ctx->channels + i, i);

  return 1;
}

int mod_setup_efx_backups(xm_context_t *ctx, const mod_layout_t *layout, xm_context_group_cursor_t *cursor)
{
  xm_mod_efx_backup_t *backup;
  int8_t *backup_data;

  if (!ctx || !layout || !cursor) return 0;

  ctx->mod_efx_backup_count = 0;
  ctx->mod_efx_backups = NULL;
  ctx->mod_efx_backup_data = NULL;

  if (!layout->efx_backup_count) return 1;

  backup = (xm_mod_efx_backup_t*)xm_group_cursor_alloc(&cursor->format_extra, cursor->format_extra_end, (size_t)layout->efx_backup_count * sizeof(xm_mod_efx_backup_t), true);
  if (!backup) return 0;

  backup_data = (int8_t*)xm_group_cursor_alloc(&cursor->format_extra, cursor->format_extra_end, layout->efx_backup_bytes, false);
  if (layout->efx_backup_bytes && !backup_data) return 0;

  ctx->mod_efx_backups = backup;
  ctx->mod_efx_backup_data = backup_data;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    xm_instrument_t *instr;
    xm_sample_t *sample;
    uint32_t loop_length;

    if (!(layout->efx_sample_mask & (1u << i))) continue;
    if (i >= ctx->module.num_instruments) continue;

    instr = ctx->module.instruments + i;
    if (!instr->num_samples || !instr->samples) continue;

    sample = instr->samples;
    if (sample->bits != 8 || sample->loop_type == XM_NO_LOOP || !sample->data8) continue;

    loop_length = sample->loop_length;
    if (sample->loop_start >= sample->length || loop_length <= 2) continue;
    if (loop_length > sample->length - sample->loop_start) loop_length = sample->length - sample->loop_start;
    if (loop_length <= 2) continue;

    backup->sample = sample;
    backup->backup8 = backup_data;
    backup->loop_start = sample->loop_start;
    backup->loop_length = loop_length;
    memcpy(backup_data, sample->data8 + sample->loop_start, loop_length);

    backup_data += loop_length;
    backup++;
    ctx->mod_efx_backup_count++;
  }

  return 1;
}

void mod_restore_efx_backups(xm_context_t *ctx)
{
  if (!ctx || !ctx->mod_efx_backup_count || !ctx->mod_efx_backups) return;

  for (uint16_t i = 0; i < ctx->mod_efx_backup_count; i++)
  {
    xm_mod_efx_backup_t *backup = ctx->mod_efx_backups + i;

    if (!backup->sample || !backup->sample->data8 || !backup->backup8 || !backup->loop_length) continue;
    if (backup->loop_start >= backup->sample->length) continue;
    if (backup->loop_length > backup->sample->length - backup->loop_start) continue;

    memcpy(backup->sample->data8 + backup->loop_start, backup->backup8, backup->loop_length);
  }
}

int mod_finish_context(xm_context_t **ctxp, xm_context_t *ctx)
{
  if (XM_DEFENSIVE && xm_check_sanity_postload(ctx))
  {
    xm_free_context(ctx);
    if (ctxp) *ctxp = NULL;
    return -1;
  }

  return (int)ctx->ctx_size;
}

int mod_allocate_format_extra_group(xm_context_t *ctx, xm_context_group_cursor_t *cursor, const xm_context_group_sizes_t *sizes, MFUNC mfunc)
{
  void *format_extra;

  if (!ctx || !cursor || !sizes || !mfunc) return 0;
  if (!sizes->format_extra_size) return 1;

  format_extra = xm_alloc_and_register_group(ctx, sizes->format_extra_size, TRACKER_CONTEXT_SEG_FORMAT_EXTRA, mfunc, true);
  if (!format_extra) return 0;

  cursor->format_extra = (char*)format_extra;
  cursor->format_extra_end = cursor->format_extra + sizes->format_extra_size;
  return 1;
}

int mod_create_context_safe(xm_context_t **ctxp, void *moddata, size_t moddata_length, uint32_t rate, MFUNC mfunc, size_t *out_bytes_needed)
{
  mod_layout_t layout;
  xm_context_group_sizes_t sizes;
  xm_context_group_cursor_t cursor;
  xm_context_t *ctx;

  if (out_bytes_needed) *out_bytes_needed = 0;
  if (ctxp) *ctxp = NULL;
  if (!ctxp || !moddata || !mfunc) return -1;

  if (!mod_read_layout(moddata, moddata_length, &layout)) return -1;
  if (!mod_layout_set_efx_backup_mask(&layout, mod_scan_efx_sample_mask_from_data(moddata, &layout))) return -1;
  if (!mod_get_group_sizes_for_layout(&layout, &sizes)) return -1;

  if (out_bytes_needed) *out_bytes_needed = sizes.total_size;
  if (!sizes.total_size) return -1;
  if (sizes.total_size > INT_MAX) return -2;

  if (!xm_allocate_context_groups(&ctx, &cursor, &sizes, mfunc)) return -2;
  *ctxp = ctx;

  if (!mod_load_module(ctx, moddata, &layout, &cursor, mfunc))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    return -1;
  }

  if (!mod_setup_context_runtime(ctx, &cursor, rate))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    return -1;
  }

  if (!mod_setup_efx_backups(ctx, &layout, &cursor))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    return -1;
  }

  return mod_finish_context(ctxp, ctx);
}

// ============================================================================
// S3M module loader
// ============================================================================

#define S3M_FILE_TYPE_OFFSET 29
#define S3M_ORDERS_OFFSET 32
#define S3M_SAMPLES_OFFSET 34
#define S3M_PATTERNS_OFFSET 36
#define S3M_FLAGS_OFFSET 38
#define S3M_CWTV_OFFSET 40
#define S3M_FORMAT_VERSION_OFFSET 42
#define S3M_GLOBAL_VOLUME_OFFSET 48
#define S3M_SPEED_OFFSET 49
#define S3M_TEMPO_OFFSET 50
#define S3M_MASTER_VOLUME_OFFSET 51
#define S3M_PANNING_TABLE_FLAG_OFFSET 53
#define S3M_CHANNEL_SETTINGS_OFFSET 64
#define S3M_FILE_TYPE_MODULE 0x10
#define S3M_PANNING_TABLE_PRESENT 0xFC
#define S3M_FLAG_FAST_VOLUME_SLIDES 0x40
#define S3M_CWTV_ST3_00 0x1300
#define S3M_CWTV_ST3_20 0x1320
#define S3M_FORMAT_SIGNED 0x01
#define S3M_FORMAT_UNSIGNED 0x02
#define S3M_SAMPLE_TYPE_NONE 0
#define S3M_SAMPLE_TYPE_PCM 1
#define S3M_SAMPLE_TYPE_ADLIB 2
#define S3M_SAMPLE_PACK_NONE 0
#define S3M_ADLIB_REG_COUNT 12
#define S3M_ADLIB_CHANNEL_NONE 0xFF
#define S3M_ADLIB_CHANNEL_FIRST 16
#define S3M_ADLIB_CHANNEL_COUNT 16
#define S3M_SAMPLE_FLAG_LOOP 0x01
#define S3M_SAMPLE_FLAG_STEREO 0x02
#define S3M_SAMPLE_FLAG_16BIT 0x04
#define S3M_PATTERN_NOTE_PRESENT 0x20
#define S3M_PATTERN_VOLUME_PRESENT 0x40
#define S3M_PATTERN_EFFECT_PRESENT 0x80
#define S3M_PATTERN_CHANNEL_MASK 0x1F
#define S3M_NOTE_NONE 0xFF
#define S3M_NOTE_KEY_OFF 0xFE

const char *g_s3m_last_error = "S3M loader was not called";
char g_s3m_last_error_buf[160];

struct s3m_sample_layout_t
{
  uint8_t name[28];
  uint32_t header_offset;
  uint32_t data_offset;
  uint32_t length;
  uint32_t file_length;
  uint32_t right_file_length;
  uint32_t loop_start;
  uint32_t loop_end;
  uint32_t c4speed;
  uint8_t type;
  uint8_t volume;
  uint8_t pack;
  uint8_t flags;
  uint8_t channels;
};

struct s3m_layout_t
{
  uint8_t name[28];
  uint16_t order_count;
  uint16_t sample_count;
  uint16_t pattern_count;
  uint16_t song_length;
  uint16_t channels;
  uint16_t flags;
  uint16_t cwtv;
  uint16_t format_version;
  uint8_t global_volume;
  uint8_t speed;
  uint8_t tempo;
  uint8_t master_volume;
  uint8_t has_panning_table;
  uint8_t signed_samples;
  uint8_t stereo;
  uint8_t channel_map[S3M_MAX_CHANNELS];
  uint8_t orders[S3M_MAX_ORDERS];
  uint8_t pattern_table[S3M_MAX_ORDERS];
  uint8_t channel_settings[S3M_MAX_CHANNELS];
  uint8_t channel_panning[S3M_MAX_CHANNELS];
  uint8_t channel_opl[S3M_MAX_CHANNELS];
  uint16_t sample_para[S3M_MAX_SAMPLES];
  uint16_t pattern_para[S3M_MAX_PATTERNS];
  s3m_sample_layout_t samples[S3M_MAX_SAMPLES];
  s3m_adlib_instrument_t adlib_instruments[S3M_MAX_SAMPLES];
  uint8_t has_adlib_instruments;
  uint8_t has_opl_channels;
  size_t pattern_slots_bytes;
  size_t sample_bytes;
};

typedef int (*s3m_read_at_fn)(void *user, size_t offset, void *dst, size_t size);

struct s3m_read_source_t
{
  void *user;
  size_t size;
  s3m_read_at_fn read_at;
};

const char *s3m_get_last_error()
{
  return g_s3m_last_error;
}

void s3m_set_error(const char *msg)
{
  g_s3m_last_error = msg ? msg : "S3M unknown error";
}

void s3m_set_error_fmt(const char *fmt, ...)
{
  va_list ap;

  if (!fmt)
  {
    s3m_set_error("S3M unknown error");
    return;
  }

  va_start(ap, fmt);
  vsnprintf(g_s3m_last_error_buf, sizeof(g_s3m_last_error_buf), fmt, ap);
  va_end(ap);
  g_s3m_last_error_buf[sizeof(g_s3m_last_error_buf) - 1] = 0;
  g_s3m_last_error = g_s3m_last_error_buf;
}

size_t s3m_para_offset(uint16_t para)
{
  return (size_t)para << 4;
}

float s3m_pan_from_nibble(uint8_t pan)
{
  if (pan > 15) pan = 15;
  return (float)pan / 15.f;
}

void s3m_set_default_panning(s3m_layout_t *layout)
{
  if (!layout) return;

  for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
  {
    uint8_t setting = layout->channel_settings[i];

    if (!layout->stereo || setting == 0xFF)
      layout->channel_panning[i] = 8;
    else
    {
      setting &= 0x7F;
      if (setting < 8)
        layout->channel_panning[i] = 3;
      else if (setting < 16)
        layout->channel_panning[i] = 12;
      else
        layout->channel_panning[i] = 8;
    }
  }
}

void s3m_compact_channel_panning(s3m_layout_t *layout)
{
  if (!layout) return;

  for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
  {
    uint8_t mapped = layout->channel_map[i];

    if (mapped != 0xFF)
      layout->channel_panning[mapped] = layout->channel_panning[i];
  }

  for (uint16_t i = layout->channels; i < S3M_MAX_CHANNELS; i++)
    layout->channel_panning[i] = 8;
}

int s3m_source_read_at(const s3m_read_source_t *source, size_t offset, void *dst, size_t size)
{
  if (!source || !source->read_at || (!dst && size)) return 0;
  if (!size) return 1;
  if (offset > source->size || source->size - offset < size) return 0;
  return source->read_at(source->user, offset, dst, size);
}

int s3m_memory_read_at(void *user, size_t offset, void *dst, size_t size)
{
  const uint8_t *data = (const uint8_t*)user;

  if ((!data && size) || (!dst && size)) return 0;
  if (size) memcpy(dst, data + offset, size);
  return 1;
}

int s3m_read_layout_from_source(const s3m_read_source_t *source, s3m_layout_t *layout)
{
  uint8_t header[S3M_HEADER_SIZE];
  uint8_t sample_header[S3M_SAMPLE_HEADER_SIZE];
  uint8_t table_entry[2];
  size_t tables_offset;
  size_t panning_offset;
  size_t tables_size = 0;
  size_t v;
  uint8_t max_pattern = 0;

  if (!source || !layout) return 0;
  memset(layout, 0, sizeof(*layout));

  if (source->size < S3M_HEADER_SIZE)
  {
    s3m_set_error("S3M header is shorter than 96 bytes");
    return 0;
  }

  if (!s3m_source_read_at(source, 0, header, sizeof(header)))
  {
    s3m_set_error("S3M header read failed");
    return 0;
  }

  if (memcmp(header + S3M_MAGIC_OFFSET, "SCRM", 4) != 0)
  {
    s3m_set_error("S3M missing SCRM signature");
    return 0;
  }

  if (header[S3M_FILE_TYPE_OFFSET] != S3M_FILE_TYPE_MODULE)
  {
    s3m_set_error_fmt("S3M invalid file type: 0x%02X", header[S3M_FILE_TYPE_OFFSET]);
    return 0;
  }

  memcpy(layout->name, header, sizeof(layout->name));
  layout->order_count = tracker_rd_le16(header + S3M_ORDERS_OFFSET);
  layout->sample_count = tracker_rd_le16(header + S3M_SAMPLES_OFFSET);
  layout->pattern_count = tracker_rd_le16(header + S3M_PATTERNS_OFFSET);
  layout->flags = tracker_rd_le16(header + S3M_FLAGS_OFFSET);
  layout->cwtv = tracker_rd_le16(header + S3M_CWTV_OFFSET);
  layout->format_version = tracker_rd_le16(header + S3M_FORMAT_VERSION_OFFSET);
  layout->global_volume = header[S3M_GLOBAL_VOLUME_OFFSET];
  layout->speed = header[S3M_SPEED_OFFSET];
  layout->tempo = header[S3M_TEMPO_OFFSET];
  layout->master_volume = header[S3M_MASTER_VOLUME_OFFSET];
  layout->stereo = (layout->master_volume & 0x80) ? 1 : 0;
  layout->has_panning_table = (header[S3M_PANNING_TABLE_FLAG_OFFSET] == S3M_PANNING_TABLE_PRESENT);
  memcpy(layout->channel_settings, header + S3M_CHANNEL_SETTINGS_OFFSET, S3M_MAX_CHANNELS);
  memset(layout->channel_map, 0xFF, sizeof(layout->channel_map));
  memset(layout->channel_opl, S3M_ADLIB_CHANNEL_NONE, sizeof(layout->channel_opl));

  if (layout->format_version != S3M_FORMAT_SIGNED && layout->format_version != S3M_FORMAT_UNSIGNED)
  {
    s3m_set_error_fmt("S3M unsupported sample format version: 0x%04X", layout->format_version);
    return 0;
  }
  layout->signed_samples = (layout->format_version == S3M_FORMAT_SIGNED);

  if (!layout->order_count || layout->order_count > S3M_MAX_ORDERS)
  {
    s3m_set_error_fmt("S3M unsupported order count: %u", layout->order_count);
    return 0;
  }

  if (layout->sample_count > S3M_MAX_SAMPLES)
  {
    s3m_set_error_fmt("S3M unsupported sample count: %u", layout->sample_count);
    return 0;
  }

  if (!layout->pattern_count || layout->pattern_count > S3M_MAX_PATTERNS)
  {
    s3m_set_error_fmt("S3M unsupported pattern count: %u", layout->pattern_count);
    return 0;
  }

  for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
  {
    uint8_t setting = layout->channel_settings[i];

    if (setting != 0xFF)
    {
      uint8_t mapped = (uint8_t)layout->channels++;

      layout->channel_map[i] = mapped;
      setting &= 0x7F;
      if (setting >= S3M_ADLIB_CHANNEL_FIRST && setting < S3M_ADLIB_CHANNEL_FIRST + S3M_ADLIB_CHANNEL_COUNT)
      {
        layout->channel_opl[mapped] = (uint8_t)(setting - S3M_ADLIB_CHANNEL_FIRST);
        layout->has_opl_channels = 1;
      }
    }
  }

  if (layout->channels < S3M_MIN_CHANNELS || layout->channels > S3M_MAX_CHANNELS)
  {
    s3m_set_error_fmt("S3M unsupported channel count: %u", layout->channels);
    return 0;
  }

  if (!tracker_size_add(&tables_size, layout->order_count)) return 0;
  if (!tracker_size_mul(layout->sample_count, 2, &v)) return 0;
  if (!tracker_size_add(&tables_size, v)) return 0;
  if (!tracker_size_mul(layout->pattern_count, 2, &v)) return 0;
  if (!tracker_size_add(&tables_size, v)) return 0;

  tables_offset = S3M_HEADER_SIZE;
  if (source->size < tables_offset || source->size - tables_offset < tables_size)
  {
    s3m_set_error("S3M truncated order/pointer tables");
    return 0;
  }

  if (!s3m_source_read_at(source, tables_offset, layout->orders, layout->order_count))
  {
    s3m_set_error("S3M order table read failed");
    return 0;
  }
  tables_offset += layout->order_count;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    if (!s3m_source_read_at(source, tables_offset + (size_t)i * 2, table_entry, sizeof(table_entry)))
    {
      s3m_set_error("S3M sample pointer table read failed");
      return 0;
    }
    layout->sample_para[i] = tracker_rd_le16(table_entry);
  }
  tables_offset += (size_t)layout->sample_count * 2;

  for (uint16_t i = 0; i < layout->pattern_count; i++)
  {
    if (!s3m_source_read_at(source, tables_offset + (size_t)i * 2, table_entry, sizeof(table_entry)))
    {
      s3m_set_error("S3M pattern pointer table read failed");
      return 0;
    }
    layout->pattern_para[i] = tracker_rd_le16(table_entry);
  }
  tables_offset += (size_t)layout->pattern_count * 2;

  s3m_set_default_panning(layout);
  panning_offset = tables_offset;
  if (layout->has_panning_table)
  {
    uint8_t panning[S3M_MAX_CHANNELS];

    if (source->size < panning_offset || source->size - panning_offset < S3M_MAX_CHANNELS)
    {
      s3m_set_error("S3M truncated panning table");
      return 0;
    }

    if (!s3m_source_read_at(source, panning_offset, panning, sizeof(panning)))
    {
      s3m_set_error("S3M panning table read failed");
      return 0;
    }

    for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
    {
      uint8_t pan = panning[i];
      if (pan & 0x20) layout->channel_panning[i] = pan & 0x0F;
    }
  }
  s3m_compact_channel_panning(layout);

  for (uint16_t i = 0; i < layout->order_count; i++)
  {
    uint8_t order = layout->orders[i];

    if (order == 0xFF) break;
    if (order == 0xFE) continue;
    if (order >= layout->pattern_count)
    {
      s3m_set_error_fmt("S3M order %u references invalid pattern %u", i, order);
      return 0;
    }

    layout->pattern_table[layout->song_length++] = order;
    if (order > max_pattern) max_pattern = order;
  }

  if (!layout->song_length)
  {
    s3m_set_error("S3M has no playable orders");
    return 0;
  }

  if ((uint16_t)max_pattern + 1 < layout->pattern_count)
    layout->pattern_count = (uint16_t)max_pattern + 1;

  if (!tracker_size_mul(layout->pattern_count, S3M_ROWS_PER_PATTERN, &layout->pattern_slots_bytes)) return 0;
  if (!tracker_size_mul(layout->pattern_slots_bytes, layout->channels, &layout->pattern_slots_bytes)) return 0;
  if (!tracker_size_mul(layout->pattern_slots_bytes, sizeof(xm_pattern_slot_t), &layout->pattern_slots_bytes)) return 0;

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    s3m_sample_layout_t *sample = layout->samples + i;
    size_t header_offset = s3m_para_offset(layout->sample_para[i]);
    size_t data_offset;
    size_t sample_bytes;

    sample->header_offset = (uint32_t)header_offset;
    if (!layout->sample_para[i]) continue;

    if (header_offset > source->size || source->size - header_offset < S3M_SAMPLE_HEADER_SIZE)
    {
      s3m_set_error_fmt("S3M sample %u header outside file", i + 1);
      return 0;
    }

    if (!s3m_source_read_at(source, header_offset, sample_header, sizeof(sample_header)))
    {
      s3m_set_error_fmt("S3M sample %u header read failed", i + 1);
      return 0;
    }

    const uint8_t *h = sample_header;
    memcpy(sample->name, h + 48, sizeof(sample->name));
    sample->type = h[0];
    sample->data_offset = (((uint32_t)h[13] << 16) | tracker_rd_le16(h + 14)) << 4;
    sample->length = tracker_rd_le32(h + 16);
    sample->loop_start = tracker_rd_le32(h + 20);
    sample->loop_end = tracker_rd_le32(h + 24);
    sample->volume = h[28];
    sample->pack = h[30];
    sample->flags = h[31];
    sample->c4speed = tracker_rd_le32(h + 32);
    if (!sample->c4speed) sample->c4speed = S3M_DEFAULT_C4SPEED;

    sample->channels = (sample->flags & S3M_SAMPLE_FLAG_STEREO) ? 2 : 1;

    if (sample->type == S3M_SAMPLE_TYPE_NONE)
    {
      sample->length = 0;
      sample->file_length = 0;
      sample->right_file_length = 0;
      sample->channels = 1;
      continue;
    }

    if (sample->type == S3M_SAMPLE_TYPE_ADLIB)
    {
      s3m_adlib_instrument_t *adlib = layout->adlib_instruments + i;

      if (memcmp(h + 76, "SCRI", 4) != 0)
      {
        s3m_set_error_fmt("S3M AdLib instrument %u missing SCRI signature", i + 1);
        return 0;
      }

      adlib->type = sample->type;
      adlib->volume = sample->volume;
      adlib->c4speed = sample->c4speed;
      memcpy(adlib->regs, h + 16, S3M_ADLIB_REG_COUNT);
      layout->has_adlib_instruments = 1;
      sample->length = 0;
      sample->file_length = 0;
      sample->right_file_length = 0;
      sample->loop_start = 0;
      sample->loop_end = 0;
      sample->flags = 0;
      sample->channels = 1;
      continue;
    }

    if (sample->type != S3M_SAMPLE_TYPE_PCM)
    {
      s3m_set_error_fmt("S3M unsupported sample %u type: %u", i + 1, sample->type);
      return 0;
    }

    if (memcmp(h + 76, "SCRS", 4) != 0)
    {
      s3m_set_error_fmt("S3M sample %u missing SCRS signature", i + 1);
      return 0;
    }

    if (sample->pack != S3M_SAMPLE_PACK_NONE)
    {
      s3m_set_error_fmt("S3M unsupported packed sample %u: pack=%u", i + 1, sample->pack);
      return 0;
    }

    if (sample->length > INT_MAX / 2)
    {
      s3m_set_error_fmt("S3M sample %u is too large", i + 1);
      return 0;
    }

    sample_bytes = sample->length;
    if (sample->flags & S3M_SAMPLE_FLAG_16BIT)
    {
      if (!tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;
    }
    if (sample->channels == 2 && !tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;

    data_offset = sample->data_offset;
    sample->file_length = sample->length;
    sample->right_file_length = 0;
    if (sample_bytes)
    {
      size_t bytes_per_sample = (sample->flags & S3M_SAMPLE_FLAG_16BIT) ? 2 : 1;
      size_t left_bytes = (size_t)sample->length * bytes_per_sample;

      if (data_offset >= source->size)
        sample->file_length = 0;
      else
      {
        size_t available = source->size - data_offset;
        size_t left_available = available < left_bytes ? available : left_bytes;
        sample->file_length = (uint32_t)(left_available / bytes_per_sample);

        if (sample->channels == 2 && available > left_bytes)
        {
          size_t right_available = available - left_bytes;
          if (right_available > left_bytes) right_available = left_bytes;
          sample->right_file_length = (uint32_t)(right_available / bytes_per_sample);
        }
      }
    }

    if (!tracker_size_add(&layout->sample_bytes, sample_bytes)) return 0;
  }

  return 1;
}

uint8_t s3m_note_to_xm(uint8_t note)
{
  uint8_t octave;
  uint8_t semi;
  uint16_t xm_note;

  if (note == S3M_NOTE_NONE) return 0;
  if (note == S3M_NOTE_KEY_OFF) return 97;

  octave = note >> 4;
  semi = note & 0x0F;
  if (semi > 11) return 0;

  xm_note = (uint16_t)octave * 12 + semi + 1;
  if (xm_note > 96) return 0;
  return (uint8_t)xm_note;
}

int s3m_convert_effect(xm_pattern_slot_t *slot, uint8_t command, uint8_t param, uint16_t pattern, uint16_t row, uint16_t channel)
{
  if (!slot) return 0;
  if (!command)
  {
    if (param)
    {
      slot->effect_type = XM_EFFECT_S3M_NONE;
      slot->effect_param = param;
      return 1;
    }

    slot->effect_type = 0;
    slot->effect_param = 0;
    return 1;
  }

  switch (command | 0x40)
  {
    case 'A':
      slot->effect_type = XM_EFFECT_S3M_SPEED;
      slot->effect_param = param;
      return 1;

    case 'B':
      slot->effect_type = XM_EFFECT_S3M_POSITION_JUMP;
      slot->effect_param = param;
      return 1;

    case 'C':
      slot->effect_type = XM_EFFECT_S3M_PATTERN_BREAK;
      slot->effect_param = param;
      return 1;

    case 'D':
      slot->effect_type = XM_EFFECT_S3M_VOLUME_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'E':
      slot->effect_type = XM_EFFECT_S3M_PORTAMENTO_DOWN;
      slot->effect_param = param;
      return 1;

    case 'F':
      slot->effect_type = XM_EFFECT_S3M_PORTAMENTO_UP;
      slot->effect_param = param;
      return 1;

    case 'G':
      slot->effect_type = XM_EFFECT_S3M_TONE_PORTAMENTO;
      slot->effect_param = param;
      return 1;

    case 'H':
      slot->effect_type = XM_EFFECT_S3M_VIBRATO;
      slot->effect_param = param;
      return 1;

    case 'I':
      slot->effect_type = XM_EFFECT_S3M_TREMOR;
      slot->effect_param = param;
      return 1;

    case 'J':
      slot->effect_type = XM_EFFECT_S3M_ARPEGGIO;
      slot->effect_param = param;
      return 1;

    case 'K':
      slot->effect_type = XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'L':
      slot->effect_type = XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'M':
      slot->effect_type = XM_EFFECT_S3M_CHANNEL_VOLUME;
      slot->effect_param = param;
      return 1;

    case 'N':
      slot->effect_type = XM_EFFECT_S3M_CHANNEL_VOLUME_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'O':
      slot->effect_type = XM_EFFECT_S3M_SAMPLE_OFFSET;
      slot->effect_param = param;
      return 1;

    case 'P':
      slot->effect_type = XM_EFFECT_S3M_PANNING_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'Q':
      slot->effect_type = XM_EFFECT_S3M_RETRIG;
      slot->effect_param = param;
      return 1;

    case 'R':
      slot->effect_type = XM_EFFECT_S3M_TREMOLO;
      slot->effect_param = param;
      return 1;

    case 'S':
      slot->effect_type = XM_EFFECT_S3M_EXTENDED;
      slot->effect_param = param;
      return 1;

    case 'T':
      slot->effect_type = XM_EFFECT_S3M_TEMPO;
      slot->effect_param = param;
      return 1;

    case 'U':
      slot->effect_type = XM_EFFECT_S3M_FINE_VIBRATO;
      slot->effect_param = param;
      return 1;

    case 'V':
      slot->effect_type = XM_EFFECT_S3M_GLOBAL_VOLUME;
      slot->effect_param = param;
      return 1;

    case 'W':
      slot->effect_type = XM_EFFECT_S3M_GLOBAL_VOLUME_SLIDE;
      slot->effect_param = param;
      return 1;

    case 'X':
      slot->effect_type = XM_EFFECT_S3M_PANNING;
      slot->effect_param = param;
      return 1;

    case 'Y':
      slot->effect_type = XM_EFFECT_S3M_PANBRELLO;
      slot->effect_param = param;
      return 1;

    case 'Z':
      slot->effect_type = XM_EFFECT_S3M_MIDI_MACRO;
      slot->effect_param = param;
      return 1;

    default:
      break;
  }

  s3m_set_error_fmt("S3M unsupported effect %c%02X at pattern %u row %u channel %u",
    (command >= 1 && command <= 26) ? ('A' + command - 1) : '?',
    param,
    pattern,
    row,
    channel);
  return 0;
}

int s3m_source_read_u8(const s3m_read_source_t *source, size_t *offset, uint8_t *out)
{
  if (!source || !offset || !out) return 0;
  if (!s3m_source_read_at(source, *offset, out, 1)) return 0;
  (*offset)++;
  return 1;
}

int s3m_decode_pattern_from_source(xm_pattern_t *pat, const s3m_read_source_t *source, const s3m_layout_t *layout, uint16_t pattern_index)
{
  size_t base;
  size_t offset;
  size_t soft_end_offset;
  size_t hard_end_offset;
  uint16_t packed_len;
  uint8_t len_buf[2];
  uint16_t row = 0;

  if (!pat || !source || !layout) return 0;
  if (!layout->pattern_para[pattern_index]) return 1;

  base = s3m_para_offset(layout->pattern_para[pattern_index]);
  if (base > source->size || source->size - base < 2)
  {
    s3m_set_error_fmt("S3M pattern %u offset outside file", pattern_index);
    return 0;
  }

  if (!s3m_source_read_at(source, base, len_buf, sizeof(len_buf)))
  {
    s3m_set_error_fmt("S3M pattern %u length read failed", pattern_index);
    return 0;
  }

  packed_len = tracker_rd_le16(len_buf);
  if (packed_len < 2) return 1;

  soft_end_offset = base + packed_len;
  hard_end_offset = base + 2 + packed_len;
  if (soft_end_offset > source->size) soft_end_offset = source->size;
  if (hard_end_offset > source->size) hard_end_offset = source->size;
  for (uint16_t i = 0; i < layout->pattern_count; i++)
  {
    size_t next_offset;

    if (i == pattern_index || !layout->pattern_para[i]) continue;
    next_offset = s3m_para_offset(layout->pattern_para[i]);
    if (next_offset > base && next_offset < hard_end_offset) hard_end_offset = next_offset;
  }
  if (soft_end_offset > hard_end_offset) soft_end_offset = hard_end_offset;
  offset = base + 2;

  while (row < S3M_ROWS_PER_PATTERN)
  {
    uint8_t info;
    uint8_t channel;
    uint8_t note = S3M_NOTE_NONE;
    uint8_t instr = 0;
    uint8_t volume = 0xFF;
    uint8_t command = 0;
    uint8_t param = 0;
    xm_pattern_slot_t *slot = NULL;

    size_t event_offset = offset;

    if (offset >= hard_end_offset) return 1;

    if (!s3m_source_read_u8(source, &offset, &info))
    {
      s3m_set_error_fmt("S3M pattern %u read failed at row %u", pattern_index, row);
      return 0;
    }

    if (info == 0)
    {
      row++;
      continue;
    }

    channel = info & S3M_PATTERN_CHANNEL_MASK;
    if (info & S3M_PATTERN_NOTE_PRESENT)
    {
      if (hard_end_offset - offset < 2)
      {
        if (event_offset >= soft_end_offset) return 1;
        s3m_set_error_fmt("S3M pattern %u truncated note at row %u", pattern_index, row);
        return 0;
      }
      if (!s3m_source_read_u8(source, &offset, &note)) return 0;
      if (!s3m_source_read_u8(source, &offset, &instr)) return 0;
    }

    if (info & S3M_PATTERN_VOLUME_PRESENT)
    {
      if (offset >= hard_end_offset)
      {
        if (event_offset >= soft_end_offset) return 1;
        s3m_set_error_fmt("S3M pattern %u truncated volume at row %u", pattern_index, row);
        return 0;
      }
      if (!s3m_source_read_u8(source, &offset, &volume)) return 0;
    }

    if (info & S3M_PATTERN_EFFECT_PRESENT)
    {
      if (hard_end_offset - offset < 2)
      {
        if (event_offset >= soft_end_offset) return 1;
        s3m_set_error_fmt("S3M pattern %u truncated effect at row %u", pattern_index, row);
        return 0;
      }
      if (!s3m_source_read_u8(source, &offset, &command)) return 0;
      if (!s3m_source_read_u8(source, &offset, &param)) return 0;
    }

    channel = layout->channel_map[channel];
    if (channel == 0xFF) continue;

    slot = pat->slots + (size_t)row * layout->channels + channel;
    slot->note = s3m_note_to_xm(note);
    slot->instrument = instr;
    if (volume != 0xFF)
    {
      if (volume >= 128 && volume <= 192)
        slot->volume_column = volume;
      else
      {
        if (volume > 64) volume = 64;
        slot->volume_column = (uint8_t)(0x10 + volume);
      }
    }

    if (!s3m_convert_effect(slot, command, param, pattern_index, row, channel))
    {
      if (event_offset >= soft_end_offset) return 1;
      return 0;
    }
  }

  return 1;
}

void s3m_apply_post_pattern_compat(xm_context_t *ctx, const s3m_layout_t *layout)
{
  int zxx_count_left = 0;
  int zxx_count_right = 0;
  bool pixplay_panning;

  if (!ctx || !layout) return;

  pixplay_panning = layout->cwtv < S3M_CWTV_ST3_20;
  if (!pixplay_panning) return;

  for (uint16_t pat_i = 0; pat_i < ctx->module.num_patterns; pat_i++)
  {
    xm_pattern_t *pat = ctx->module.patterns + pat_i;
    if (!pat->slots) continue;

    for (uint16_t row = 0; row < pat->num_rows; row++)
    {
      for (uint16_t chn = 0; chn < ctx->module.num_channels; chn++)
      {
        xm_pattern_slot_t *slot = pat->slots + (size_t)row * ctx->module.num_channels + chn;
        if (slot->effect_type != XM_EFFECT_S3M_MIDI_MACRO) continue;

        if (slot->effect_param > 0x0F)
        {
          pixplay_panning = false;
          break;
        }

        if (slot->effect_param < 0x08)
          zxx_count_left++;
        else if (slot->effect_param > 0x08)
          zxx_count_right++;
      }

      if (!pixplay_panning) break;
    }

    if (!pixplay_panning) break;
  }

  if (!pixplay_panning) return;
  if (zxx_count_left + zxx_count_right < ctx->module.num_channels) return;
  if (-zxx_count_left + zxx_count_right >= ctx->module.num_channels) return;

  for (uint16_t pat_i = 0; pat_i < ctx->module.num_patterns; pat_i++)
  {
    xm_pattern_t *pat = ctx->module.patterns + pat_i;
    if (!pat->slots) continue;

    for (uint16_t row = 0; row < pat->num_rows; row++)
    {
      for (uint16_t chn = 0; chn < ctx->module.num_channels; chn++)
      {
        xm_pattern_slot_t *slot = pat->slots + (size_t)row * ctx->module.num_channels + chn;
        if (slot->effect_type != XM_EFFECT_S3M_MIDI_MACRO) continue;

        slot->effect_type = XM_EFFECT_S3M_EXTENDED;
        slot->effect_param |= 0x80;
      }
    }
  }
}

int s3m_load_sample_data_from_source(xm_sample_t *sample, const s3m_read_source_t *source, const s3m_sample_layout_t *sl, bool signed_samples)
{
  uint32_t left_file_length;
  uint32_t right_file_length;
  size_t bytes_per_sample;
  size_t left_file_bytes;
  size_t right_file_bytes;
  size_t right_offset;

  if (!sample || !source || !sl || !sample->data8 || !sample->length) return 1;

  bytes_per_sample = (sample->bits == 16) ? 2 : 1;
  left_file_length = sl->file_length;
  if (left_file_length > sample->length) left_file_length = sample->length;
  right_file_length = sl->right_file_length;
  if (right_file_length > sample->length) right_file_length = sample->length;

  left_file_bytes = (size_t)left_file_length * bytes_per_sample;
  right_file_bytes = (size_t)right_file_length * bytes_per_sample;
  right_offset = (size_t)sample->length * bytes_per_sample;

  if (left_file_bytes && !s3m_source_read_at(source, sl->data_offset, sample->data8, left_file_bytes))
  {
    s3m_set_error("S3M sample data read failed");
    return 0;
  }

  if (sl->channels == 2 && right_file_bytes)
  {
    if (!s3m_source_read_at(source, sl->data_offset + right_offset, sample->data8 + right_offset, right_file_bytes))
    {
      s3m_set_error("S3M stereo sample data read failed");
      return 0;
    }
  }

  if (sample->bits == 16)
  {
    int16_t *left = sample->data16;
    int16_t *right = (int16_t*)(sample->data8 + right_offset);

    for (uint32_t i = 0; i < sample->length; i++)
    {
      int32_t left_val = 0;
      int32_t right_val = 0;

      if (i < left_file_length)
      {
        uint16_t raw = tracker_rd_le16((const uint8_t*)left + (size_t)i * 2);
        if (!signed_samples) raw ^= 0x8000;
        left_val = (int16_t)raw;
      }

      if (sl->channels == 2)
      {
        if (i < right_file_length)
        {
          uint16_t raw = tracker_rd_le16((const uint8_t*)right + (size_t)i * 2);
          if (!signed_samples) raw ^= 0x8000;
          right_val = (int16_t)raw;
        }
        left[i] = (int16_t)((left_val + right_val) / 2);
      }
      else
        left[i] = (int16_t)left_val;
    }
  }
  else
  {
    int8_t *left = sample->data8;
    int8_t *right = sample->data8 + right_offset;

    for (uint32_t i = 0; i < sample->length; i++)
    {
      int16_t left_val = 0;
      int16_t right_val = 0;

      if (i < left_file_length)
        left_val = signed_samples ? left[i] : (int8_t)((uint8_t)left[i] ^ 0x80);

      if (sl->channels == 2)
      {
        if (i < right_file_length)
          right_val = signed_samples ? right[i] : (int8_t)((uint8_t)right[i] ^ 0x80);
        left[i] = (int8_t)((left_val + right_val) / 2);
      }
      else
        left[i] = (int8_t)left_val;
    }
  }

  return 1;
}

int s3m_get_group_sizes_for_layout(const s3m_layout_t *layout, xm_context_group_sizes_t *sizes)
{
  size_t v;

  if (!layout || !sizes) return 0;
  memset(sizes, 0, sizeof(*sizes));

  if (!xm_group_sizes_add(sizes, &sizes->context_size, sizeof(xm_context_t))) return 0;

  if (!tracker_size_mul(layout->pattern_count, sizeof(xm_pattern_t), &v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->patterns_size, v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->patterns_size, layout->pattern_slots_bytes)) return 0;

  if (!tracker_size_mul(layout->sample_count, sizeof(xm_instrument_t), &v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->instruments_size, v)) return 0;
  if (!tracker_size_mul(layout->sample_count, sizeof(xm_sample_t), &v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->instruments_size, v)) return 0;

  if (!tracker_size_add(&sizes->total_size, layout->sample_bytes)) return 0;

  if (layout->has_adlib_instruments)
  {
    if (!tracker_size_mul(layout->sample_count, sizeof(s3m_adlib_instrument_t), &v)) return 0;
    if (!xm_group_sizes_add(sizes, &sizes->format_extra_size, v)) return 0;
  }

  if (layout->has_opl_channels)
  {
    if (!tracker_size_mul(layout->channels, sizeof(uint8_t), &v)) return 0;
    if (!xm_group_sizes_add(sizes, &sizes->format_extra_size, v)) return 0;
  }

  if (!tracker_size_mul(layout->channels, sizeof(xm_channel_context_t), &v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->runtime_size, v)) return 0;
  if (!tracker_size_mul(layout->song_length, MAX_NUM_ROWS, &v)) return 0;
  if (!tracker_size_mul(v, sizeof(uint8_t), &v)) return 0;
  if (!xm_group_sizes_add(sizes, &sizes->runtime_size, v)) return 0;

  return sizes->total_size <= INT_MAX;
}

int s3m_setup_module_header_grouped(xm_context_t *ctx, const s3m_layout_t *layout, xm_context_group_cursor_t *cursor)
{
  xm_module_t *mod;
  size_t patterns_size;
  size_t instruments_size;

  if (!ctx || !layout || !cursor) return 0;

  mod = &ctx->module;
  ctx->tracker_format = TRACKER_FORMAT_S3M;

#if XM_STRINGS
  tracker_copy_trimmed(mod->name, sizeof(mod->name), layout->name, sizeof(layout->name));
  tracker_copy_trimmed(mod->trackername, sizeof(mod->trackername), (const uint8_t*)"Scream Tracker 3", 16);
#endif

  mod->length = layout->song_length;
  mod->restart_position = 0;
  mod->num_channels = layout->channels;
  mod->num_patterns = layout->pattern_count;
  mod->num_instruments = layout->sample_count;
  mod->frequency_type = XM_S3M_FREQUENCIES;
  memcpy(mod->pattern_table, layout->pattern_table, layout->song_length);

  patterns_size = (size_t)mod->num_patterns * sizeof(xm_pattern_t);
  mod->patterns = patterns_size ? (xm_pattern_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, patterns_size, true) : NULL;
  if (patterns_size && !mod->patterns) return 0;

  instruments_size = (size_t)mod->num_instruments * sizeof(xm_instrument_t);
  mod->instruments = instruments_size ? (xm_instrument_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, instruments_size, true) : NULL;
  if (instruments_size && !mod->instruments) return 0;

  if (layout->has_adlib_instruments)
  {
    size_t adlib_size = (size_t)layout->sample_count * sizeof(s3m_adlib_instrument_t);

    ctx->s3m_adlib_instrument_count = layout->sample_count;
    ctx->s3m_adlib_instruments = (s3m_adlib_instrument_t*)xm_group_cursor_alloc(&cursor->format_extra, cursor->format_extra_end, adlib_size, false);
    if (!ctx->s3m_adlib_instruments) return 0;
    memcpy(ctx->s3m_adlib_instruments, layout->adlib_instruments, adlib_size);
  }

  if (layout->has_opl_channels)
  {
    ctx->s3m_channel_opl = (uint8_t*)xm_group_cursor_alloc(&cursor->format_extra, cursor->format_extra_end, layout->channels, false);
    if (!ctx->s3m_channel_opl) return 0;
    memcpy(ctx->s3m_channel_opl, layout->channel_opl, layout->channels);
  }

  return 1;
}

int s3m_setup_sample_metadata_grouped(xm_context_t *ctx, const s3m_layout_t *layout, uint16_t i, xm_context_group_cursor_t *cursor, MFUNC mfunc)
{
  xm_module_t *mod;
  const s3m_sample_layout_t *sl;
  xm_instrument_t *instr;
  xm_sample_t *sample;
  uint32_t loop_start;
  uint32_t loop_end;
  size_t sample_bytes;

  if (!ctx || !layout || !cursor || !mfunc || i >= layout->sample_count) return 0;

  mod = &ctx->module;
  sl = layout->samples + i;
  instr = mod->instruments + i;

#if XM_STRINGS
  if (layout->sample_para[i])
    tracker_copy_trimmed(instr->name, sizeof(instr->name), sl->name, sizeof(sl->name));
#endif

  instr->num_samples = 1;
  instr->samples = (xm_sample_t*)xm_group_cursor_alloc(&cursor->instruments, cursor->instruments_end, sizeof(xm_sample_t), true);
  if (!instr->samples) return 0;

  sample = instr->samples;
#if XM_STRINGS
  if (layout->sample_para[i])
    tracker_copy_trimmed(sample->name, sizeof(sample->name), sl->name, sizeof(sl->name));
#endif

  sample->bits = (sl->flags & S3M_SAMPLE_FLAG_16BIT) ? 16 : 8;
  sample->length = sl->length;
  sample->volume = (float)(sl->volume > 64 ? 64 : sl->volume) / 64.f;
  sample->finetune = 0;
  sample->panning = .5f;
  sample->relative_note = 0;
  sample->c4speed = sl->c4speed;

  sample_bytes = sample->length;
  if (sample->bits == 16 && !tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;
  if (sl->channels == 2 && !tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;
  sample->data8 = sample_bytes ? (int8_t*)xm_alloc_and_register_group(ctx, sample_bytes, TRACKER_CONTEXT_SEG_SAMPLES, mfunc, false) : NULL;
  if (sample_bytes && !sample->data8) return 0;

  loop_start = sl->loop_start;
  loop_end = sl->loop_end;
  if (!(sl->flags & S3M_SAMPLE_FLAG_LOOP) || loop_start >= sample->length || loop_end <= loop_start + 2)
  {
    sample->loop_start = 0;
    sample->loop_length = 0;
    sample->loop_end = 0;
    sample->loop_type = XM_NO_LOOP;
  }
  else
  {
    if (loop_end > sample->length) loop_end = sample->length;
    if (loop_end <= loop_start + 2)
    {
      sample->loop_start = 0;
      sample->loop_length = 0;
      sample->loop_end = 0;
      sample->loop_type = XM_NO_LOOP;
    }
    else
    {
      sample->loop_start = loop_start;
      sample->loop_end = loop_end;
      sample->loop_length = loop_end - loop_start;
      sample->loop_type = XM_FORWARD_LOOP;
    }
  }

  sample->latest_trigger = 0;
  return 1;
}

int s3m_load_module_from_source_grouped(xm_context_t *ctx, const s3m_read_source_t *source, const s3m_layout_t *layout, xm_context_group_cursor_t *cursor, MFUNC mfunc)
{
  xm_module_t *mod;

  if (!ctx || !source || !layout || !cursor || !mfunc) return 0;
  if (!s3m_setup_module_header_grouped(ctx, layout, cursor)) return 0;

  mod = &ctx->module;
  for (uint16_t pat_i = 0; pat_i < mod->num_patterns; pat_i++)
  {
    xm_pattern_t *pat = mod->patterns + pat_i;
    size_t slots_size = (size_t)S3M_ROWS_PER_PATTERN * mod->num_channels * sizeof(xm_pattern_slot_t);

    pat->num_rows = S3M_ROWS_PER_PATTERN;
    pat->slots = slots_size ? (xm_pattern_slot_t*)xm_group_cursor_alloc(&cursor->patterns, cursor->patterns_end, slots_size, true) : NULL;
    if (slots_size && !pat->slots) return 0;
    if (!s3m_decode_pattern_from_source(pat, source, layout, pat_i)) return 0;
  }

  s3m_apply_post_pattern_compat(ctx, layout);

  for (uint16_t i = 0; i < layout->sample_count; i++)
  {
    const s3m_sample_layout_t *sl = layout->samples + i;
    xm_sample_t *sample;

    if (!s3m_setup_sample_metadata_grouped(ctx, layout, i, cursor, mfunc)) return 0;

    sample = mod->instruments[i].samples;
    if (sl->file_length && !s3m_load_sample_data_from_source(sample, source, sl, layout->signed_samples != 0)) return 0;
  }

  return 1;
}

int s3m_setup_context_runtime_grouped(xm_context_t *ctx, const s3m_layout_t *layout, xm_context_group_cursor_t *cursor, uint32_t rate)
{
  if (!ctx || !layout || !cursor) return 0;

  ctx->channels = (xm_channel_context_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, (size_t)ctx->module.num_channels * sizeof(xm_channel_context_t), true);
  if (!ctx->channels) return 0;

  ctx->row_loop_count = (uint8_t*)xm_group_cursor_alloc(&cursor->runtime, cursor->runtime_end, (size_t)ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t), true);
  if (!ctx->row_loop_count) return 0;

  ctx->tempo = layout->speed ? layout->speed : 6;
  ctx->bpm = layout->tempo ? layout->tempo : 125;
  ctx->global_volume = (float)(layout->global_volume > 64 ? 64 : layout->global_volume) / 64.f;
  ctx->s3m_fast_volume_slides =
    (layout->cwtv == S3M_CWTV_ST3_00 ||
     (layout->flags & S3M_FLAG_FAST_VOLUME_SLIDES) != 0);
  ctx->amplification = .25f;
  ctx->rate = rate;
#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  for (uint16_t i = 0; i < ctx->module.num_channels; i++)
  {
    xm_channel_context_t *ch = ctx->channels + i;
    float panning = s3m_pan_from_nibble(layout->channel_panning[i]);

    ch->ping = true;
    ch->vibrato_waveform = XM_SINE_WAVEFORM;
    ch->vibrato_waveform_retrigger = true;
    ch->tremolo_waveform = XM_SINE_WAVEFORM;
    ch->tremolo_waveform_retrigger = true;
    ch->panbrello_waveform = XM_SINE_WAVEFORM;
    ch->panbrello_waveform_retrigger = true;
    ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
    ch->channel_volume = 1.0f;
    ch->panning = panning;
    ch->default_panning = panning;
    ch->tracker_format = TRACKER_FORMAT_S3M;
    ch->period_note_offset = .0f;
    ch->panning_envelope_panning = .5f;
    ch->actual_volume = .0f;
    ch->actual_panning = panning;
#if XM_RAMPING
    ch->target_volume = ch->volume;
    ch->target_panning = panning;
#endif
  }

  return 1;
}

int s3m_finish_context(xm_context_t **ctxp, xm_context_t *ctx)
{
  if (XM_DEFENSIVE && xm_check_sanity_postload(ctx))
  {
    xm_free_context(ctx);
    if (ctxp) *ctxp = NULL;
    s3m_set_error("S3M context failed postload sanity check");
    return -1;
  }

  return (int)ctx->ctx_size;
}

int s3m_create_context_safe_source(xm_context_t **ctxp, const s3m_read_source_t *source, uint32_t rate, MFUNC mfunc, size_t *out_bytes_needed)
{
  s3m_layout_t *layout;
  xm_context_group_sizes_t sizes;
  xm_context_group_cursor_t cursor;
  xm_context_t *ctx;

  if (ctxp) *ctxp = NULL;
  if (out_bytes_needed) *out_bytes_needed = 0;
  s3m_set_error("S3M loader was not called");

  if (!ctxp || !source || !mfunc || !rate)
  {
    s3m_set_error("S3M invalid loader parameter");
    return -1;
  }

  layout = (s3m_layout_t*)mfunc(sizeof(s3m_layout_t));
  if (!layout)
  {
    s3m_set_error("S3M layout allocation failed");
    return -2;
  }

  if (!s3m_read_layout_from_source(source, layout))
  {
    free(layout);
    return -1;
  }

  if (!s3m_get_group_sizes_for_layout(layout, &sizes))
  {
    s3m_set_error("S3M context size calculation failed");
    free(layout);
    return -1;
  }

  if (out_bytes_needed) *out_bytes_needed = sizes.total_size;
  if (!sizes.total_size)
  {
    s3m_set_error("S3M context size calculation failed");
    free(layout);
    return -1;
  }
  if (sizes.total_size > INT_MAX)
  {
    s3m_set_error_fmt("S3M context too large: %u", (unsigned)sizes.total_size);
    free(layout);
    return -2;
  }

  if (!xm_allocate_context_groups(&ctx, &cursor, &sizes, mfunc))
  {
    s3m_set_error_fmt("S3M context allocation failed: need=%u", (unsigned)sizes.total_size);
    free(layout);
    return -2;
  }
  *ctxp = ctx;

  if (!s3m_load_module_from_source_grouped(ctx, source, layout, &cursor, mfunc))
  {
    xm_free_context(ctx);
    *ctxp = NULL;
    free(layout);
    return -1;
  }

  if (!s3m_setup_context_runtime_grouped(ctx, layout, &cursor, rate))
  {
    s3m_set_error("S3M runtime context setup failed");
    xm_free_context(ctx);
    *ctxp = NULL;
    free(layout);
    return -1;
  }

  ctx->ctx_size = sizes.total_size;
  if (out_bytes_needed) *out_bytes_needed = ctx->ctx_size;

  free(layout);
  s3m_set_error("S3M OK");
  return s3m_finish_context(ctxp, ctx);
}

int s3m_create_context_safe(xm_context_t **ctxp, void *s3mdata, size_t s3mdata_length, uint32_t rate, MFUNC mfunc, size_t *out_bytes_needed)
{
  s3m_read_source_t source;

  if (!s3mdata)
  {
    if (ctxp) *ctxp = NULL;
    if (out_bytes_needed) *out_bytes_needed = 0;
    s3m_set_error("S3M invalid loader parameter");
    return -1;
  }

  source.user = s3mdata;
  source.size = s3mdata_length;
  source.read_at = s3m_memory_read_at;
  return s3m_create_context_safe_source(ctxp, &source, rate, mfunc, out_bytes_needed);
}

// ============================================================================
// Common tracker playback engine
// ============================================================================


/* ----- Static functions ----- */

static float xm_waveform(xm_waveform_type_t, uint8_t);
static void xm_autovibrato(xm_context_t*, xm_channel_context_t*);
static void xm_vibrato(xm_context_t*, xm_channel_context_t*, uint8_t, uint16_t);
static void xm_tremolo(xm_context_t*, xm_channel_context_t*, uint8_t, uint16_t);
static void xm_panbrello(xm_channel_context_t*, uint8_t, uint16_t);
static void xm_arpeggio(xm_context_t*, xm_channel_context_t*, uint8_t, uint16_t);
static void xm_tone_portamento(xm_context_t*, xm_channel_context_t*);
static void xm_pitch_slide(xm_context_t*, xm_channel_context_t*, float);
static void xm_panning_slide(xm_channel_context_t*, uint8_t);
static void xm_volume_slide(xm_context_t*, xm_channel_context_t*, uint8_t);
static void xm_invert_loop(xm_channel_context_t*);

static float xm_envelope_lerp(xm_envelope_point_t*, xm_envelope_point_t*, uint16_t);
static void xm_envelope_tick(xm_channel_context_t*, xm_envelope_t*, uint16_t*, float*);
static void xm_envelopes(xm_channel_context_t*);

static float xm_linear_period(float);
static float xm_linear_frequency(float);
static float xm_amiga_period(float);
static float xm_amiga_frequency(float);
static float xm_s3m_period(float);
static float xm_s3m_frequency(xm_channel_context_t*, float, float);
static float xm_period(xm_context_t*, float);
static float xm_frequency(xm_context_t*, xm_channel_context_t*, float, float);
static void xm_update_frequency(xm_context_t*, xm_channel_context_t*);

static void xm_handle_note_and_instrument(xm_context_t*, xm_channel_context_t*, xm_pattern_slot_t*);
static void xm_trigger_note(xm_context_t*, xm_channel_context_t*, unsigned int flags);
static void xm_cut_note(xm_channel_context_t*);
static void xm_key_off(xm_channel_context_t*);

static void xm_post_pattern_change(xm_context_t*);
static void xm_row(xm_context_t*);
static void xm_tick(xm_context_t*);

static float xm_sample_at(xm_sample_t*, size_t);
static float xm_next_of_sample(xm_channel_context_t*);
static void xm_sample_no_tick(xm_context_t*, float*, float*);
size_t xm_pingpong_forward_subrun(float, float, float, size_t);
size_t xm_pingpong_backward_subrun(float, float, float, size_t);
void xm_advance_no_loop(xm_channel_context_t*, size_t);
void xm_advance_forward_loop(xm_channel_context_t*, size_t);
void xm_advance_pingpong_loop(xm_channel_context_t*, size_t);
void xm_advance_gain_ramp_no_loop(xm_channel_context_t*, size_t, float, float);
void xm_advance_gain_ramp_forward_loop(xm_channel_context_t*, size_t, float, float);
void xm_advance_gain_ramp_pingpong_loop(xm_channel_context_t*, size_t, float, float);
void xm_render_no_loop_8(xm_channel_context_t*, float*, size_t);
void xm_render_no_loop_16(xm_channel_context_t*, float*, size_t);
void xm_render_forward_loop_8(xm_channel_context_t*, float*, size_t);
void xm_render_forward_loop_16(xm_channel_context_t*, float*, size_t);
void xm_render_pingpong_loop_8(xm_channel_context_t*, float*, size_t);
void xm_render_pingpong_loop_16(xm_channel_context_t*, float*, size_t);
void xm_render_sample_ramp_no_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_sample_ramp_no_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_sample_ramp_forward_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_sample_ramp_forward_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_sample_ramp_pingpong_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_sample_ramp_pingpong_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_no_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_no_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_forward_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_forward_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_pingpong_loop_8(xm_channel_context_t*, float*, size_t, float, float);
void xm_render_gain_ramp_pingpong_loop_16(xm_channel_context_t*, float*, size_t, float, float);
void xm_sample(xm_context_t*, float*, float*);
void xm_render(xm_context_t*, float*, size_t);

/* ----- Other oddities ----- */

#define XM_TRIGGER_KEEP_VOLUME             (1 << 0)
#define XM_TRIGGER_KEEP_PERIOD             (1 << 1)
#define XM_TRIGGER_KEEP_SAMPLE_POSITION    (1 << 2)

static const uint16_t amiga_frequencies[] =
{
  1712, 1616, 1525, 1440, /* C-2, C#2, D-2, D#2 */
  1357, 1281, 1209, 1141, /* E-2, F-2, F#2, G-2 */
  1077, 1017, 961, 907,   /* G#2, A-2, A#2, B-2 */
  856,                    /* C-3 */
};

#define XM_S3M_C4_PERIOD      1712.f
#define XM_S3M_C4_FREQUENCY   261.625565f
#define XM_S3M_DEFAULT_C4SPEED 8363.f

static const float    multi_retrig_add[] =
{
  0.f,  -1.f,  -2.f, -4.f, /* 0, 1, 2, 3 */
  -8.f, -16.f, 0.f, 0.f,   /* 4, 5, 6, 7 */
  0.f,  1.f,   2.f, 4.f,   /* 8, 9, A, B */
  8.f,  16.f,  0.f, 0.f    /* C, D, E, F */
};

static const float    multi_retrig_multiply[] =
{
  1.f, 1.f, 1.f, 1.f,       /* 0, 1, 2, 3 */
  1.f, 1.f, .6666667f, .5f, /* 4, 5, 6, 7 */
  1.f, 1.f, 1.f, 1.f,       /* 8, 9, A, B */
  1.f, 1.f, 1.5f, 2.f       /* C, D, E, F */
};

#define XM_CLAMP_UP1F(vol, limit)            do {                  \
    if ((vol) > (limit)) (vol) = (limit);    \
} while(0)
#define XM_CLAMP_UP(vol)                     XM_CLAMP_UP1F((vol), 1.f)

#define XM_CLAMP_DOWN1F(vol, limit)          do {                \
    if ((vol) < (limit)) (vol) = (limit);    \
} while(0)
#define XM_CLAMP_DOWN(vol)                   XM_CLAMP_DOWN1F((vol), .0f)

#define XM_CLAMP2F(vol, up, down)            do {                  \
    if ((vol) > (up)) (vol)        = (up);                  \
    else if ((vol) < (down)) (vol) = (down); \
} while(0)
#define XM_CLAMP(vol)                        XM_CLAMP2F((vol), 1.f, .0f)

#define XM_SLIDE_TOWARDS(val, goal, incr)    do {          \
    if ((val) > (goal)) {                                            \
      (val) -= (incr);                                                \
      XM_CLAMP_DOWN1F((val), (goal));                 \
    } else if ((val) < (goal)) {                                     \
      (val) += (incr);                                                \
      XM_CLAMP_UP1F((val), (goal));                   \
    }                                                                                       \
} while(0)

#define XM_LERP(u, v, t)                     ((u) + (t) * ((v) - (u)))
#define XM_INVERSE_LERP(u, v, lerp)          (((lerp) - (u)) / ((v) - (u)))

#define HAS_TONE_PORTAMENTO(s)               ((s)->effect_type == 3 \
                                              || (s)->effect_type == 5 \
                                              || (s)->effect_type == XM_EFFECT_S3M_TONE_PORTAMENTO \
                                              || (s)->effect_type == XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE \
                                              || ((s)->volume_column >> 4) == 0xF)
#define HAS_ARPEGGIO(s)                      (((s)->effect_type == 0 \
                                               || (s)->effect_type == XM_EFFECT_S3M_ARPEGGIO) \
                                              && (s)->effect_param != 0)
#define HAS_VIBRATO(s)                       ((s)->effect_type == 4 \
                                              || (s)->effect_type == 6 \
                                              || (s)->effect_type == XM_EFFECT_S3M_VIBRATO \
                                              || (s)->effect_type == XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE \
                                              || (s)->effect_type == XM_EFFECT_S3M_FINE_VIBRATO \
                                              || ((s)->volume_column >> 4) == 0xB)
#define NOTE_IS_VALID(n)                     ((n) > 0 && (n) < 97)
#define SLOT_HAS_NOTE(s)                     ((s)->period != 0 || NOTE_IS_VALID((s)->note))
#define CHANNEL_IS_MOD(ch)                   ((ch)->tracker_format == TRACKER_FORMAT_MOD)
#define CHANNEL_IS_S3M(ch)                   ((ch)->tracker_format == TRACKER_FORMAT_S3M)
#define CHANNEL_HAS_DEFAULT_PANNING(ch)      ((ch)->default_panning >= .0f)
#define CHANNEL_USES_PERIOD_EFFECTS(ch)      (CHANNEL_IS_MOD(ch) || CHANNEL_IS_S3M(ch))

/* ----- Function definitions ----- */

static const uint8_t mod_efx_table[16] = {
  0, 5, 6, 7, 8, 10, 11, 13,
  16, 19, 22, 26, 32, 43, 64, 128
};

static float xm_waveform(xm_waveform_type_t waveform, uint8_t step)
{
  static unsigned int next_rand = 24492;

  step %= 0x40;

  switch (waveform)
  {
    case XM_SINE_WAVEFORM:
      /* Why not use a table? For saving space, and because there's very very little actual performance gain. */
      return -sinf(2.f * 3.141592f * (float)step / (float)0x40);

    case XM_RAMP_DOWN_WAVEFORM:
      /* Ramp down: 1.0f when step = 0; -1.0f when step = 0x40 */
      return (float)(0x20 - step) / 0x20;

    case XM_SQUARE_WAVEFORM:
      /* Square with a 50% duty */
      return (step >= 0x20) ? 1.f : -1.f;

    case XM_RANDOM_WAVEFORM:
      /* Use the POSIX.1-2001 example, just to be deterministic across different machines */
      next_rand = next_rand * 1103515245 + 12345;
      return (float)((next_rand >> 16) & 0x7FFF) / (float)0x4000 - 1.f;

    case XM_RAMP_UP_WAVEFORM:
      /* Ramp up: -1.f when step = 0; 1.f when step = 0x40 */
      return (float)(step - 0x20) / 0x20;

    default:
      break;
  }

  return .0f;
}

static void xm_autovibrato(xm_context_t* ctx, xm_channel_context_t* ch)
{
  if (ch->instrument == NULL || ch->instrument->vibrato_depth == 0)
    return;

  xm_instrument_t* instr = ch->instrument;
  float          sweep   = 1.f;

  if (ch->autovibrato_ticks < instr->vibrato_sweep)
  {
    /* No idea if this is correct, but it sounds close enough… */
    sweep = XM_LERP(0.f, 1.f, (float)ch->autovibrato_ticks / (float)instr->vibrato_sweep);
  }

  unsigned int step = ((ch->autovibrato_ticks++) * instr->vibrato_rate) >> 2;
  ch->autovibrato_note_offset = .25f * xm_waveform(instr->vibrato_type, step) * (float)instr->vibrato_depth / (float)0xF * sweep;
  xm_update_frequency(ctx, ch);
}

static void xm_vibrato(xm_context_t* ctx, xm_channel_context_t* ch, uint8_t param, uint16_t pos)
{
  unsigned int step = pos * (param >> 4);

  if (CHANNEL_USES_PERIOD_EFFECTS(ch))
  {
    ch->vibrato_note_offset = .0f;
    ch->vibrato_period_offset = -xm_waveform(ch->vibrato_waveform, step) * (float)((param & 0x0F) << 1);
  }
  else
  {
    ch->vibrato_period_offset = .0f;
    ch->vibrato_note_offset = 2.f * xm_waveform(ch->vibrato_waveform, step) * (float)(param & 0x0F) / (float)0xF;
  }

  xm_update_frequency(ctx, ch);
}

static void xm_tremolo(xm_context_t* ctx, xm_channel_context_t* ch, uint8_t param, uint16_t pos)
{
  unsigned int step = pos * (param >> 4);

  /* Not so sure about this, it sounds correct by ear compared with MilkyTracker, but it could come from other bugs */
  if (CHANNEL_USES_PERIOD_EFFECTS(ch))
    ch->tremolo_volume = -1.f * xm_waveform(ch->tremolo_waveform, step) * (float)(param & 0x0F) / (float)0x10;
  else
    ch->tremolo_volume = -1.f * xm_waveform(ch->tremolo_waveform, step) * (float)(param & 0x0F) / (float)0xF;
}

static void xm_panbrello(xm_channel_context_t* ch, uint8_t param, uint16_t pos)
{
  unsigned int step = pos * (param >> 4);

  ch->panbrello_panning_offset = xm_waveform(ch->panbrello_waveform, step) * (float)(param & 0x0F) / 64.f;
}

static void xm_arpeggio(xm_context_t* ctx, xm_channel_context_t* ch, uint8_t param, uint16_t tick)
{
  switch (tick % 3)
  {
    case 0:
      ch->arp_in_progress = false;
      ch->arp_note_offset = 0;
      break;

    case 1:
      ch->arp_in_progress = true;
      if (ch->current != NULL && ch->current->period != 0)
        ch->arp_note_offset = param >> 4;
      else
        ch->arp_note_offset = param & 0x0F;
      break;

    case 2:
      ch->arp_in_progress = true;
      if (ch->current != NULL && ch->current->period != 0)
        ch->arp_note_offset = param & 0x0F;
      else
        ch->arp_note_offset = param >> 4;
      break;
  }

  xm_update_frequency(ctx, ch);
}

static void xm_tone_portamento(xm_context_t* ctx, xm_channel_context_t* ch)
{
  /* 3xx called without a note, wait until we get an actual target note. */
  if (ch->tone_portamento_target_period == 0.f)
    return;

  if (ch->period != ch->tone_portamento_target_period)
  {
    float slide_scale = (ctx->module.frequency_type == XM_LINEAR_FREQUENCIES) ? 4.f : 1.f;

    XM_SLIDE_TOWARDS(ch->period,
                     ch->tone_portamento_target_period,
                     slide_scale * ch->tone_portamento_param
                     );
    xm_update_frequency(ctx, ch);
  }
}

static void xm_pitch_slide(xm_context_t* ctx, xm_channel_context_t* ch, float period_offset)
{
  if (ctx->module.frequency_type == XM_LINEAR_FREQUENCIES)
    period_offset *= 4.f;

  ch->period += period_offset;
  XM_CLAMP_DOWN(ch->period);
  /* XXX: upper bound of period ? */

  xm_update_frequency(ctx, ch);
}

static void xm_panning_slide(xm_channel_context_t* ch, uint8_t rawval)
{
  float f;

  if ((rawval & 0xF0) && (rawval & 0x0F))  /* Illegal state */
    return;

  if (rawval & 0xF0)
  {
    /* Slide right */
    f = (float)(rawval >> 4) / (float)0xFF;
    ch->panning += f;
    XM_CLAMP_UP(ch->panning);
  }
  else
  {
    /* Slide left */
    f = (float)(rawval & 0x0F) / (float)0xFF;
    ch->panning -= f;
    XM_CLAMP_DOWN(ch->panning);
  }
}

static void xm_volume_slide(xm_context_t* ctx, xm_channel_context_t* ch, uint8_t rawval)
{
  float f;

  if (CHANNEL_IS_S3M(ch))
  {
    if ((rawval & 0x0F) == 0x0F && (rawval & 0xF0))
    {
      if (ctx->current_tick == 0)
      {
        f = (float)(rawval >> 4) / (float)0x40;
        ch->volume += f;
        XM_CLAMP_UP(ch->volume);
      }
      return;
    }

    if ((rawval & 0xF0) == 0xF0 && (rawval & 0x0F))
    {
      if (ctx->current_tick == 0)
      {
        f = (float)(rawval & 0x0F) / (float)0x40;
        ch->volume -= f;
        XM_CLAMP_DOWN(ch->volume);
      }
      return;
    }

    if (ctx->current_tick == 0 && !ctx->s3m_fast_volume_slides) return;

    if (rawval & 0x0F)
    {
      f = (float)(rawval & 0x0F) / (float)0x40;
      ch->volume -= f;
      XM_CLAMP_DOWN(ch->volume);
      return;
    }

    f = (float)(rawval >> 4) / (float)0x40;
    ch->volume += f;
    XM_CLAMP_UP(ch->volume);
    return;
  }

  if ((rawval & 0xF0) && (rawval & 0x0F))
  {
    /* Illegal state */
    return;
  }

  if (rawval & 0xF0)
  {
    /* Slide up */
    f = (float)(rawval >> 4) / (float)0x40;
    ch->volume += f;
    XM_CLAMP_UP(ch->volume);
  }
  else
  {
    /* Slide down */
    f = (float)(rawval & 0x0F) / (float)0x40;
    ch->volume -= f;
    XM_CLAMP_DOWN(ch->volume);
  }
}

static void xm_invert_loop(xm_channel_context_t* ch)
{
  uint32_t loop_length;
  uint32_t pos;

  if (!ch || !CHANNEL_IS_MOD(ch) || !ch->invert_loop_speed) return;
  if (!ch->sample || ch->sample->bits != 8 || !ch->sample->data8) return;
  if (ch->sample->loop_type == XM_NO_LOOP) return;
  if (ch->sample->loop_start >= ch->sample->length || ch->sample->loop_length <= 2) return;

  loop_length = ch->sample->loop_length;
  if (loop_length > ch->sample->length - ch->sample->loop_start)
    loop_length = ch->sample->length - ch->sample->loop_start;
  if (loop_length <= 2) return;

  ch->invert_loop_delay += mod_efx_table[ch->invert_loop_speed & 0x0F];
  if (ch->invert_loop_delay < 128) return;

  ch->invert_loop_delay = 0;
  ch->invert_loop_offset++;
  if (ch->invert_loop_offset >= loop_length) ch->invert_loop_offset = 0;

  pos = ch->sample->loop_start + ch->invert_loop_offset;
  ch->sample->data8[pos] = ~ch->sample->data8[pos];
}

static float xm_envelope_lerp(xm_envelope_point_t* a, xm_envelope_point_t* b, uint16_t pos)
{
  /* Linear interpolation between two envelope points */
  if (pos <= a->frame)
    return a->value;
  else if (pos >= b->frame)
    return b->value;
  else
  {
    float p = (float)(pos - a->frame) / (float)(b->frame - a->frame);
    return a->value * (1 - p) + b->value * p;
  }
}

static void xm_post_pattern_change(xm_context_t* ctx)
{
  /* Loop if necessary */
  if (ctx->module.length == 0)
  {
    ctx->current_table_index = 0;
    ctx->current_row = 0;
    return;
  }

  if (ctx->current_table_index >= ctx->module.length)
  {
    if (ctx->module.restart_position < ctx->module.length)
      ctx->current_table_index = ctx->module.restart_position;
    else
      ctx->current_table_index = 0;
  }
}

static float xm_linear_period(float note)
{
  return 7680.f - note * 64.f;
}

static float xm_linear_frequency(float period)
{
  return 8363.f * powf(2.f, (4608.f - period) / 768.f);
}

static float xm_amiga_period(float note)
{
  unsigned int intnote = note;
  uint8_t      a = intnote % 12;
  int8_t       octave = note / 12.f - 2;
  uint16_t     p1 = amiga_frequencies[a], p2 = amiga_frequencies[a + 1];

  if (octave > 0)
  {
    p1 >>= octave;
    p2 >>= octave;
  }
  else if (octave < 0)
  {
    p1 <<= (-octave);
    p2 <<= (-octave);
  }

  return XM_LERP(p1, p2, note - intnote);
}

static float xm_amiga_frequency(float period)
{
  if (period == .0f)
    return .0f;

  /* This is the PAL value. No reason to choose this one over the NTSC value. */
  return 7093789.2f / (period * 2.f);
}

static float xm_s3m_period(float note)
{
  return XM_S3M_C4_PERIOD * powf(2.f, (48.f - note) / 12.f);
}

static float xm_s3m_frequency(xm_channel_context_t* ch, float period, float note_offset)
{
  float c4speed = XM_S3M_DEFAULT_C4SPEED;

  if (period <= .0f) return .0f;
  if (ch != NULL && ch->sample != NULL && ch->sample->c4speed != 0)
    c4speed = (float)ch->sample->c4speed;

  return c4speed * XM_S3M_C4_PERIOD * powf(2.f, note_offset / 12.f) / period;
}

static float xm_period(xm_context_t* ctx, float note)
{
  switch (ctx->module.frequency_type)
  {
    case XM_LINEAR_FREQUENCIES:
      return xm_linear_period(note);

    case XM_AMIGA_FREQUENCIES:
      return xm_amiga_period(note);

    case XM_S3M_FREQUENCIES:
      return xm_s3m_period(note);
  }

  return .0f;
}

static float xm_frequency(xm_context_t* ctx, xm_channel_context_t* ch, float period, float note_offset)
{
  uint8_t  a;
  int8_t   octave;
  float    note;
  uint16_t p1, p2;

  switch (ctx->module.frequency_type)
  {
    case XM_LINEAR_FREQUENCIES:
      return xm_linear_frequency(period - 64.f * note_offset);

    case XM_AMIGA_FREQUENCIES:
      if (note_offset == 0)
      {
        /* A chance to escape from insanity */
        return xm_amiga_frequency(period);
      }

      /* FIXME: this is very crappy at best */
      a = octave = 0;

      /* Find the octave of the current period */
      if (period > amiga_frequencies[0])
      {
        --octave;
        while (period > (amiga_frequencies[0] << (-octave)))
          --octave;
      }
      else if (period < amiga_frequencies[12])
      {
        ++octave;
        while (period < (amiga_frequencies[12] >> octave))
          ++octave;
      }

      /* Find the smallest note closest to the current period */
      for (uint8_t i = 0; i < 12; ++i)
      {
        p1 = amiga_frequencies[i], p2 = amiga_frequencies[i + 1];

        if (octave > 0)
        {
          p1 >>= octave;
          p2 >>= octave;
        }
        else if (octave < 0)
        {
          p1 <<= (-octave);
          p2 <<= (-octave);
        }

        if (p2 <= period && period <= p1)
        {
          a = i;
          break;
        }
      }

      if (XM_DEBUG && (p1 < period || p2 > period))
      {
        DEBUG("%i <= %f <= %i should hold but doesn't, this is a bug", p2, period, p1);
      }

      note = 12.f * (octave + 2) + a + XM_INVERSE_LERP(p1, p2, period);

      return xm_amiga_frequency(xm_amiga_period(note + note_offset));

    case XM_S3M_FREQUENCIES:
      return xm_s3m_frequency(ch, period, note_offset);
  }

  return .0f;
}

static void xm_update_frequency(xm_context_t* ctx, xm_channel_context_t* ch)
{
  float note_offset = ch->period_note_offset;
  float period = ch->period;

  if (ch->arp_note_offset > 0)
    note_offset += ch->arp_note_offset;
  else
  {
    note_offset += ch->vibrato_note_offset + ch->autovibrato_note_offset;
    period += ch->vibrato_period_offset;
    XM_CLAMP_DOWN(period);
  }

  if (CHANNEL_IS_S3M(ch) && ch->s3m_glissando_control && ch->tone_portamento_target_period != .0f && period > .0f)
  {
    float note = 48.f - 12.f * log2f(period / XM_S3M_C4_PERIOD);
    period = xm_s3m_period(floorf(note + .5f));
  }

  ch->frequency = xm_frequency(ctx, ch, period, note_offset);
  ch->step = ch->frequency / ctx->rate;
}


int s3m_context_uses_adlib(const xm_context_t *ctx)
{
  return ctx && ctx->tracker_format == TRACKER_FORMAT_S3M &&
         ctx->s3m_adlib_instruments && ctx->s3m_channel_opl;
}

int s3m_adlib_channel_index(const xm_context_t *ctx, const xm_channel_context_t *ch)
{
  if (!ctx || !ch || !ctx->channels) return -1;
  ptrdiff_t index = ch - ctx->channels;
  if (index < 0 || index >= ctx->module.num_channels) return -1;
  return (int)index;
}

uint8_t s3m_adlib_opl_channel(const xm_context_t *ctx, const xm_channel_context_t *ch)
{
  int index = s3m_adlib_channel_index(ctx, ch);
  if (index < 0 || !ctx->s3m_channel_opl) return S3M_ADLIB_CHANNEL_NONE;
  return ctx->s3m_channel_opl[index];
}

const s3m_adlib_instrument_t *s3m_adlib_current_instrument(const xm_context_t *ctx, const xm_channel_context_t *ch, uint16_t *out_index)
{
  if (out_index) *out_index = 0;
  if (!s3m_context_uses_adlib(ctx) || !ch || !ch->instrument) return NULL;

  ptrdiff_t instrument_index = ch->instrument - ctx->module.instruments;
  if (instrument_index < 0 || instrument_index >= ctx->module.num_instruments) return NULL;
  if ((uint16_t)instrument_index >= ctx->s3m_adlib_instrument_count) return NULL;

  const s3m_adlib_instrument_t *adlib = ctx->s3m_adlib_instruments + instrument_index;
  if (adlib->type != S3M_SAMPLE_TYPE_ADLIB) return NULL;

  if (out_index) *out_index = (uint16_t)instrument_index;
  return adlib;
}

uint8_t s3m_adlib_operator_offset(uint8_t opl_channel, int carrier)
{
  if (opl_channel >= 9) return 0;

  if (carrier)
  {
    switch (opl_channel)
    {
      case 0: return 3;
      case 1: return 4;
      case 2: return 5;
      case 3: return 11;
      case 4: return 12;
      case 5: return 13;
      case 6: return 19;
      case 7: return 20;
      case 8: return 21;
    }
  }

  switch (opl_channel)
  {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    case 3: return 8;
    case 4: return 9;
    case 5: return 10;
    case 6: return 16;
    case 7: return 17;
    case 8: return 18;
  }

  return 0;
}

uint8_t s3m_adlib_pan_bits(const xm_channel_context_t *ch)
{
  if (!ch) return 0x30;
  if (ch->panning < .25f) return 0x10;
  if (ch->panning > .75f) return 0x20;
  return 0x30;
}

uint8_t s3m_adlib_scaled_total_level(uint8_t base, float volume)
{
  if (volume < .0f) volume = .0f;
  if (volume > 1.f) volume = 1.f;

  uint8_t level = base & 0x3F;
  uint8_t attenuation = (uint8_t)((1.f - volume) * 63.f + .5f);
  uint16_t scaled = (uint16_t)level + attenuation;
  if (scaled > 63) scaled = 63;
  return (base & 0xC0) | (uint8_t)scaled;
}

void s3m_adlib_frequency_to_opl(float frequency, uint16_t *out_fnum, uint8_t *out_block)
{
  uint16_t fnum = 0;
  uint8_t block = 0;

  if (frequency > .0f)
  {
    for (block = 0; block < 7; block++)
    {
      float f = frequency * ldexpf(1.f, 20 - block) / 49716.f;
      if (f <= 1023.f)
      {
        fnum = (uint16_t)(f + .5f);
        break;
      }
    }

    if (block == 7)
    {
      float f = frequency * ldexpf(1.f, 13) / 49716.f;
      fnum = f > 1023.f ? 1023 : (uint16_t)(f + .5f);
    }
  }

  if (out_fnum) *out_fnum = fnum;
  if (out_block) *out_block = block;
}

float s3m_adlib_channel_frequency(xm_channel_context_t *ch)
{
  if (!ch || ch->period <= .0f) return .0f;

  float period = ch->period;
  float note_offset = ch->period_note_offset;

  if (ch->arp_note_offset > 0)
    note_offset += ch->arp_note_offset;
  else
  {
    note_offset += ch->vibrato_note_offset + ch->autovibrato_note_offset;
    period += ch->vibrato_period_offset;
    XM_CLAMP_DOWN(period);
  }

  if (ch->s3m_glissando_control && ch->tone_portamento_target_period != .0f && period > .0f)
  {
    float note = 48.f - 12.f * log2f(period / XM_S3M_C4_PERIOD);
    period = xm_s3m_period(floorf(note + .5f));
  }

  if (period <= .0f) return .0f;
  return XM_S3M_C4_FREQUENCY * XM_S3M_C4_PERIOD * powf(2.f, note_offset / 12.f) / period;
}

float s3m_adlib_channel_volume(const xm_context_t *ctx, const xm_channel_context_t *ch)
{
  if (!ctx || !ch) return .0f;

#if XM_RAMPING
  float volume = ch->target_volume;
#else
  float volume = ch->actual_volume;
#endif

  if (ch->muted || (ch->instrument && ch->instrument->muted)) volume = .0f;
  volume *= ctx->global_volume;
  XM_CLAMP(volume);
  return volume;
}

void s3m_adlib_write_key_off(uint8_t opl_channel)
{
  if (opl_channel >= 9) return;
  uint8_t reg = (uint8_t)(0xB0 + opl_channel);
  opl_write_fm_reg(reg, opl_read_fm_reg(reg) & 0xDF);
}

void s3m_adlib_write_frequency(uint8_t opl_channel, float frequency, int key_on)
{
  if (opl_channel >= 9) return;

  uint16_t fnum;
  uint8_t block;
  s3m_adlib_frequency_to_opl(frequency, &fnum, &block);

  OPL_FM_REG_WRITE writes[2];
  writes[0].reg = (uint8_t)(0xA0 + opl_channel);
  writes[0].value = (uint8_t)(fnum & 0xFF);
  writes[1].reg = (uint8_t)(0xB0 + opl_channel);
  writes[1].value = (uint8_t)(((fnum >> 8) & 0x03) | ((block & 0x07) << 2) | (key_on ? 0x20 : 0x00));
  opl_write_fm_regs(writes, 2);
}

void s3m_adlib_write_instrument(const xm_context_t *ctx, const xm_channel_context_t *ch, uint8_t opl_channel, const s3m_adlib_instrument_t *adlib)
{
  if (!ctx || !ch || !adlib || opl_channel >= 9) return;

  uint8_t mod_op = s3m_adlib_operator_offset(opl_channel, 0);
  uint8_t car_op = s3m_adlib_operator_offset(opl_channel, 1);
  float volume = s3m_adlib_channel_volume(ctx, ch);

  OPL_FM_REG_WRITE writes[11];
  writes[0].reg = (uint8_t)(0x20 + mod_op);
  writes[0].value = adlib->regs[0];
  writes[1].reg = (uint8_t)(0x20 + car_op);
  writes[1].value = adlib->regs[1];
  writes[2].reg = (uint8_t)(0x40 + mod_op);
  writes[2].value = (adlib->regs[10] & 0x01) ? s3m_adlib_scaled_total_level(adlib->regs[2], volume) : adlib->regs[2];
  writes[3].reg = (uint8_t)(0x40 + car_op);
  writes[3].value = s3m_adlib_scaled_total_level(adlib->regs[3], volume);
  writes[4].reg = (uint8_t)(0x60 + mod_op);
  writes[4].value = adlib->regs[4];
  writes[5].reg = (uint8_t)(0x60 + car_op);
  writes[5].value = adlib->regs[5];
  writes[6].reg = (uint8_t)(0x80 + mod_op);
  writes[6].value = adlib->regs[6];
  writes[7].reg = (uint8_t)(0x80 + car_op);
  writes[7].value = adlib->regs[7];
  writes[8].reg = (uint8_t)(0xE0 + mod_op);
  writes[8].value = adlib->regs[8] & 0x07;
  writes[9].reg = (uint8_t)(0xE0 + car_op);
  writes[9].value = adlib->regs[9] & 0x07;
  writes[10].reg = (uint8_t)(0xC0 + opl_channel);
  writes[10].value = (adlib->regs[10] & 0x0F) | s3m_adlib_pan_bits(ch);
  opl_write_fm_regs(writes, 11);
}

void s3m_adlib_write_volume(const xm_context_t *ctx, const xm_channel_context_t *ch, uint8_t opl_channel, const s3m_adlib_instrument_t *adlib)
{
  if (!ctx || !ch || !adlib || opl_channel >= 9) return;

  uint8_t mod_op = s3m_adlib_operator_offset(opl_channel, 0);
  uint8_t car_op = s3m_adlib_operator_offset(opl_channel, 1);
  float volume = s3m_adlib_channel_volume(ctx, ch);

  OPL_FM_REG_WRITE writes[2];
  size_t count = 0;

  if (adlib->regs[10] & 0x01)
  {
    writes[count].reg = (uint8_t)(0x40 + mod_op);
    writes[count].value = s3m_adlib_scaled_total_level(adlib->regs[2], volume);
    count++;
  }

  writes[count].reg = (uint8_t)(0x40 + car_op);
  writes[count].value = s3m_adlib_scaled_total_level(adlib->regs[3], volume);
  count++;

  opl_write_fm_regs(writes, count);
}

void s3m_adlib_sync_channel(xm_context_t *ctx, xm_channel_context_t *ch)
{
  if (!s3m_context_uses_adlib(ctx) || !ch) return;

  uint8_t opl_channel = s3m_adlib_opl_channel(ctx, ch);
  if (opl_channel == S3M_ADLIB_CHANNEL_NONE) return;

  if (opl_channel >= 9)
  {
    ch->s3m_opl_key_on = 0;
    return;
  }

  uint16_t instrument_index;
  const s3m_adlib_instrument_t *adlib = s3m_adlib_current_instrument(ctx, ch, &instrument_index);
  if (!adlib || ch->sample_position < 0 || ch->s3m_note_cut)
  {
    if (ch->s3m_opl_key_on)
    {
      s3m_adlib_write_key_off(opl_channel);
      ch->s3m_opl_key_on = 0;
    }
    return;
  }

  if (!opl_is_enabled()) return;

  int new_trigger = ch->s3m_opl_trigger_id != ch->s3m_opl_synced_trigger_id;
  int new_instrument = ch->s3m_opl_instrument != (uint8_t)(instrument_index + 1);

  if (!ch->s3m_opl_key_on || new_trigger || new_instrument)
  {
    s3m_adlib_write_key_off(opl_channel);
    if (!ch->s3m_opl_key_on || new_instrument)
      s3m_adlib_write_instrument(ctx, ch, opl_channel, adlib);
    else
      s3m_adlib_write_volume(ctx, ch, opl_channel, adlib);
    ch->s3m_opl_key_on = 1;
    ch->s3m_opl_instrument = (uint8_t)(instrument_index + 1);
    ch->s3m_opl_synced_trigger_id = ch->s3m_opl_trigger_id;
  }
  else
  {
    s3m_adlib_write_volume(ctx, ch, opl_channel, adlib);
    opl_write_fm_reg((uint8_t)(0xC0 + opl_channel), (adlib->regs[10] & 0x0F) | s3m_adlib_pan_bits(ch));
  }

  s3m_adlib_write_frequency(opl_channel, s3m_adlib_channel_frequency(ch), 1);
}

void s3m_adlib_all_notes_off(xm_context_t *ctx)
{
  if (!s3m_context_uses_adlib(ctx)) return;

  for (uint16_t i = 0; i < ctx->module.num_channels; i++)
  {
    uint8_t opl_channel = ctx->s3m_channel_opl[i];
    if (opl_channel < 9)
      s3m_adlib_write_key_off(opl_channel);

    ctx->channels[i].s3m_opl_key_on = 0;
    ctx->channels[i].s3m_opl_instrument = 0;
    ctx->channels[i].s3m_opl_trigger_id = 0;
    ctx->channels[i].s3m_opl_synced_trigger_id = 0;
  }
}

void s3m_adlib_force_opl2_fm_mode()
{
  OPL_FM_REG_WRITE writes[3];
  writes[0].reg = 0x0105;
  writes[0].value = 0x01;
  writes[1].reg = 0x0104;
  writes[1].value = 0x00;
  writes[2].reg = 0x0105;
  writes[2].value = 0x00;
  opl_write_fm_regs(writes, 3);
}

esp_err_t s3m_adlib_prepare_opl(xm_context_t *ctx)
{
  if (!s3m_context_uses_adlib(ctx)) return ESP_OK;

  OPL_INFO info;
  esp_err_t err = opl_get_info(&info);
  if (err != ESP_OK) return err;

  int use_opl2_melody_fast_path = 0;

  if (!info.enabled)
  {
    err = opl_enable(OPL_MODE_YMF262, 0);
    if (err != ESP_OK) return err;
    use_opl2_melody_fast_path = 1;
  }
  else if (info.mode == OPL_MODE_YMF262)
    use_opl2_melody_fast_path = 1;

  if (use_opl2_melody_fast_path)
    s3m_adlib_force_opl2_fm_mode();

  opl_reset();

  if (use_opl2_melody_fast_path)
  {
    err = opl_set_fm_render_mode(OPL_FM_RENDER_OPL2_MELODY);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}

static void xm_handle_note_and_instrument(xm_context_t* ctx, xm_channel_context_t* ch, xm_pattern_slot_t* s)
{
  xm_pattern_slot_t s3m_memory_slot;

  if (CHANNEL_IS_S3M(ch))
  {
    bool s3m_uses_shared_memory = false;

    switch (s->effect_type)
    {
      case XM_EFFECT_S3M_VOLUME_SLIDE:
      case XM_EFFECT_S3M_PORTAMENTO_DOWN:
      case XM_EFFECT_S3M_PORTAMENTO_UP:
      case XM_EFFECT_S3M_TREMOR:
      case XM_EFFECT_S3M_ARPEGGIO:
      case XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE:
      case XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE:
      case XM_EFFECT_S3M_RETRIG:
      case XM_EFFECT_S3M_TREMOLO:
      case XM_EFFECT_S3M_EXTENDED:
        s3m_uses_shared_memory = true;
        break;

      default:
        break;
    }

    if (s->effect_type == XM_EFFECT_S3M_NONE)
    {
      if (s->effect_param)
        ch->s3m_effect_memory = s->effect_param;
    }
    else if (s3m_uses_shared_memory)
    {
      if (s->effect_param)
        ch->s3m_effect_memory = s->effect_param;
      else if (ch->s3m_effect_memory)
      {
        s3m_memory_slot = *s;
        s3m_memory_slot.effect_param = ch->s3m_effect_memory;
        s = &s3m_memory_slot;
      }
    }
  }

  if (s->instrument > 0)
  {
    if (CHANNEL_IS_MOD(ch) && NOTE_IS_VALID(s->note)) ch->invert_loop_offset = 0;

    if (HAS_TONE_PORTAMENTO(ch->current) && ch->instrument != NULL && ch->sample != NULL)
    {
      if (CHANNEL_USES_PERIOD_EFFECTS(ch))
      {
        if (s->instrument > ctx->module.num_instruments)
        {
          xm_cut_note(ch);
          ch->instrument = NULL;
          ch->sample     = NULL;
        }
        else
        {
          /* MOD tone-portamento row with instrument: update instrument params, but do not retrigger. */
          ch->instrument = ctx->module.instruments + (s->instrument - 1);
          if (ch->instrument->num_samples > 0 && ch->instrument->samples != NULL)
            ch->volume = ch->instrument->samples[0].volume;
        }
      }
      else
      {
        /* Tone portamento in effect, unclear stuff happens */
        xm_trigger_note(ctx, ch, XM_TRIGGER_KEEP_PERIOD | XM_TRIGGER_KEEP_SAMPLE_POSITION);
      }
    }
    else if (s->instrument > ctx->module.num_instruments)
    {
      /* Invalid instrument, Cut current note */
      xm_cut_note(ch);
      ch->instrument = NULL;
      ch->sample     = NULL;
    }
    else
    {
      ch->instrument = ctx->module.instruments + (s->instrument - 1);
      if (s->note == 0 && s->period == 0 && ch->sample != NULL)
      {
        if (CHANNEL_USES_PERIOD_EFFECTS(ch))
        {
          /* MOD/S3M instrument-only row: update instrument parameters, but do not retrigger. */
          if (ch->instrument->num_samples > 0 && ch->instrument->samples != NULL)
            ch->volume = ch->instrument->samples[0].volume;
        }
        else
        {
          /* Ghost instrument, trigger note */
          /* Sample position is kept, but envelopes are reset */
          xm_trigger_note(ctx, ch, XM_TRIGGER_KEEP_SAMPLE_POSITION);
        }
      }
    }
  }

  if (SLOT_HAS_NOTE(s))
  {
    /* Yes, the real XM note number is s->note -1. Try finding THAT in any of the specs! :-) */

    xm_instrument_t* instr = ch->instrument;

    if (HAS_TONE_PORTAMENTO(ch->current) && instr != NULL && ch->sample != NULL)
    {
      /* Tone portamento in effect */
      if (s->period != 0)
      {
        ch->period_note_offset = ch->sample->finetune / 128.f;
        ch->tone_portamento_target_period = s->period;
      }
      else
      {
        ch->period_note_offset = .0f;
        ch->note = s->note + ch->sample->relative_note + ch->sample->finetune / 128.f - 1.f;
        ch->tone_portamento_target_period = xm_period(ctx, ch->note);
      }
    }
    else if (instr == NULL || ch->instrument->num_samples == 0)  /* Bad instrument */
      xm_cut_note(ch);
    else
    {
      uint8_t sample_index = 0;
      if (s->period == 0)
        sample_index = instr->sample_of_notes[s->note - 1];

      if (sample_index < instr->num_samples)
      {
#if XM_RAMPING
        for (unsigned int z = 0; z < XM_SAMPLE_RAMPING_POINTS; ++z)
          ch->end_of_previous_sample[z] = xm_next_of_sample(ch);

        ch->frame_count = 0;
#endif

        ch->sample = instr->samples + sample_index;
        if (s->period != 0)
        {
          ch->orig_note = ch->note = .0f;
          ch->period_note_offset = ch->sample->finetune / 128.f;
        }
        else
        {
          ch->orig_note = ch->note = s->note + ch->sample->relative_note
                                     + ch->sample->finetune / 128.f - 1.f;
          ch->period_note_offset = .0f;
        }

        if (s->instrument > 0)
          xm_trigger_note(ctx, ch, 0);
        else  /* Ghost note: keep old volume */
          xm_trigger_note(ctx, ch, XM_TRIGGER_KEEP_VOLUME);
      }
      else  /* Bad sample */
        xm_cut_note(ch);
    }
  }
  else if (s->note == 97)  /* Key Off */
    xm_key_off(ch);

  if (CHANNEL_IS_S3M(ch) && s->volume_column >= 128 && s->volume_column <= 192)
  {
    ch->surround = false;
    ch->panning = (float)(s->volume_column - 128) / 64.f;
  }
  else switch (s->volume_column >> 4)
  {
    case 0x5:
      if (s->volume_column > 0x50)
        break;

    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4: /* Set volume */
      ch->volume = (float)(s->volume_column - 0x10) / (float)0x40;
      break;

    case 0x8: /* Fine volume slide down */
      xm_volume_slide(ctx, ch, s->volume_column & 0x0F);
      break;

    case 0x9: /* Fine volume slide up */
      xm_volume_slide(ctx, ch, s->volume_column << 4);
      break;

    case 0xA: /* Set vibrato speed */
      ch->vibrato_param = (ch->vibrato_param & 0x0F) | ((s->volume_column & 0x0F) << 4);
      break;

    case 0xC: /* Set panning */
      ch->panning = (float)(
        ((s->volume_column & 0x0F) << 4) | (s->volume_column & 0x0F)) / (float)0xFF;
      break;

    case 0xF: /* Tone portamento */
      if (s->volume_column & 0x0F)
        ch->tone_portamento_param = ((s->volume_column & 0x0F) << 4) | (s->volume_column & 0x0F);
      break;

    default:
      break;
  }

  switch (s->effect_type)
  {
    case 0: /* 0xy: XM/MOD arpeggio */
    case XM_EFFECT_S3M_ARPEGGIO: /* S3M Jxy: Arpeggio */
      if (CHANNEL_IS_S3M(ch) && s->effect_param > 0)
        ch->arpeggio_param = s->effect_param;
      break;

    case 1: /* 1xx: Portamento up */
    case XM_EFFECT_S3M_PORTAMENTO_UP: /* S3M Fxx: Portamento up */
      if (s->effect_param > 0)
        ch->portamento_up_param = s->effect_param;
      break;

    case 2: /* 2xx: Portamento down */
    case XM_EFFECT_S3M_PORTAMENTO_DOWN: /* S3M Exx: Portamento down */
      if (s->effect_param > 0)
        ch->portamento_down_param = s->effect_param;
      break;

    case 3: /* 3xx: Tone portamento */
    case XM_EFFECT_S3M_TONE_PORTAMENTO: /* S3M Gxx: Tone portamento */
      if (s->effect_param > 0)
        ch->tone_portamento_param = s->effect_param;
      break;

    case 4: /* 4xy: Vibrato */
    case XM_EFFECT_S3M_VIBRATO: /* S3M Hxy: Vibrato */
      if (s->effect_param & 0x0F) /* Set vibrato depth */
        ch->vibrato_param = (ch->vibrato_param & 0xF0) | (s->effect_param & 0x0F);

      if (s->effect_param >> 4)  /* Set vibrato speed */
        ch->vibrato_param = (s->effect_param & 0xF0) | (ch->vibrato_param & 0x0F);
      break;

    case 5: /* 5xy: Tone portamento + Volume slide */
    case XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE: /* S3M Lxy: Tone portamento + volume slide */
      if (s->effect_param > 0)
        ch->volume_slide_param = s->effect_param;
      break;

    case 6: /* 6xy: Vibrato + Volume slide */
    case XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE: /* S3M Kxy: Vibrato + volume slide */
      if (s->effect_param > 0)
        ch->volume_slide_param = s->effect_param;
      break;

    case 7: /* 7xy: Tremolo */
    case XM_EFFECT_S3M_TREMOLO: /* S3M Rxy: Tremolo */
      if (s->effect_param & 0x0F)  /* Set tremolo depth */
        ch->tremolo_param = (ch->tremolo_param & 0xF0) | (s->effect_param & 0x0F);

      if (s->effect_param >> 4)  /* Set tremolo speed */
        ch->tremolo_param = (s->effect_param & 0xF0) | (ch->tremolo_param & 0x0F);
      break;

    case 8: /* 8xx: Set panning */
      ch->panning = (float)s->effect_param / (float)0xFF;
      break;

    case XM_EFFECT_S3M_PANNING: /* S3M Xxx: Set panning */
      if (s->effect_param <= 0x80)
      {
        ch->surround = false;
        ch->panning = (float)s->effect_param / 128.f;
      }
      else if (s->effect_param == 0xA4)
      {
        ch->surround = true;
        ch->panning = .5f;
      }
      break;

    case 9: /* 9xx: Sample offset */
    case XM_EFFECT_S3M_SAMPLE_OFFSET: /* S3M Oxx: Sample offset */
      if (ch->sample != NULL && SLOT_HAS_NOTE(s))
      {
        uint8_t sample_offset_param = s->effect_param;
        uint32_t final_offset;

        if (sample_offset_param)
          ch->sample_offset_param = sample_offset_param;
        else
          sample_offset_param = ch->sample_offset_param;

        final_offset = sample_offset_param << (ch->sample->bits == 16 ? 7 : 8);
        if (CHANNEL_IS_S3M(ch))
          final_offset += (uint32_t)ch->s3m_high_sample_offset_param << (ch->sample->bits == 16 ? 15 : 16);

        if (final_offset >= ch->sample->length ||
            (CHANNEL_IS_S3M(ch) && s->period != 0 && ch->sample->loop_type != XM_NO_LOOP && final_offset >= ch->sample->loop_end))
        {
          if (CHANNEL_IS_S3M(ch) && s->period != 0 && ch->sample->loop_type != XM_NO_LOOP)
            ch->sample_position = ch->sample->loop_start;
          else
            ch->sample_position = -1;
          break;
        }

        ch->sample_position = final_offset;
      }
      break;

    case 0xA: /* Axy: Volume slide */
    case XM_EFFECT_S3M_VOLUME_SLIDE: /* S3M Dxy: Volume slide */
      if (s->effect_param > 0)
        ch->volume_slide_param = s->effect_param;
      break;

    case 0xB: /* Bxx: Position jump */
      if (s->effect_param < ctx->module.length)
      {
        ctx->position_jump = true;
        ctx->pattern_break = false;
        ctx->jump_dest     = s->effect_param;
        ctx->jump_row      = 0;
      }
      break;

    case XM_EFFECT_S3M_POSITION_JUMP: /* S3M Bxx: handled once per row in xm_row() */
      break;

    case 0xC: /* Cxx: Set volume */
      ch->volume = (float)((s->effect_param > 0x40)
                                                         ? 0x40 : s->effect_param) / (float)0x40;
      break;

    case 0xD: /* Dxx: Pattern break */
      /* Jump after playing this line */
      ctx->pattern_break = true;
      ctx->jump_row      = (s->effect_param >> 4) * 10 + (s->effect_param & 0x0F);
      break;

    case XM_EFFECT_S3M_PATTERN_BREAK: /* S3M Cxx: handled once per row in xm_row() */
      break;

    case 0xE: /* EXy: Extended command */
      switch (s->effect_param >> 4)
      {
        case 1: /* E1y: Fine portamento up */
          if ((s->effect_param & 0x0F) == 0 && CHANNEL_IS_MOD(ch))
            break;

          if (s->effect_param & 0x0F)
            ch->fine_portamento_up_param = s->effect_param & 0x0F;

          xm_pitch_slide(ctx, ch, -ch->fine_portamento_up_param);
          break;

        case 2: /* E2y: Fine portamento down */
          if ((s->effect_param & 0x0F) == 0 && CHANNEL_IS_MOD(ch))
            break;

          if (s->effect_param & 0x0F)
            ch->fine_portamento_down_param = s->effect_param & 0x0F;

          xm_pitch_slide(ctx, ch, ch->fine_portamento_down_param);
          break;

        case 4: /* E4y: Set vibrato control */
          ch->vibrato_waveform = (xm_waveform_type_t)(s->effect_param & 3);
          ch->vibrato_waveform_retrigger = !((s->effect_param >> 2) & 1);
          break;

        case 5: /* E5y: Set finetune */
          if (ch->sample != NULL)
          {
            uint8_t finetune = s->effect_param & 0x0F;
            int8_t signed_finetune = finetune < 8 ? finetune : finetune - 16;

            if (ch->current->period != 0)
            {
              ch->period_note_offset = (float)(signed_finetune * 16) / 128.f;
              xm_update_frequency(ctx, ch);
            }
            else if (NOTE_IS_VALID(ch->current->note))
            {
              ch->note = ch->current->note + ch->sample->relative_note + (float)(((s->effect_param & 0x0F) - 8) << 4) / 128.f - 1.f;
              ch->period = xm_period(ctx, ch->note);
              xm_update_frequency(ctx, ch);
            }
          }
          break;

        case 6: /* E6y: Pattern loop */
          if (s->effect_param & 0x0F)
          {
            if ((s->effect_param & 0x0F) == ch->pattern_loop_count)
            {
              /* Loop is over */
              ch->pattern_loop_count = 0;
              break;
            }

            /* Jump to the beginning of the loop */
            ch->pattern_loop_count++;
            ctx->position_jump = true;
            ctx->jump_row      = ch->pattern_loop_origin;
            ctx->jump_dest     = ctx->current_table_index;
          }
          else
          {
            /* Set loop start point */
            ch->pattern_loop_origin = ctx->current_row;
            /* Replicate FT2 E60 bug */
            ctx->jump_row = ch->pattern_loop_origin;
          }
          break;

        case 7: /* E7y: Set tremolo control */
          ch->tremolo_waveform           = (xm_waveform_type_t)(s->effect_param & 3);
          ch->tremolo_waveform_retrigger = !((s->effect_param >> 2) & 1);
          break;

        case 8: /* E8y: Set 4-bit panning */
          if (CHANNEL_IS_MOD(ch))
            ch->panning = (float)(s->effect_param & 0x0F) / (float)0x0F;
          break;

        case 0xA: /* EAy: Fine volume slide up */
          if ((s->effect_param & 0x0F) == 0 && CHANNEL_IS_MOD(ch))
            break;

          if (s->effect_param & 0x0F)
            ch->fine_volume_slide_param = s->effect_param & 0x0F;

          xm_volume_slide(ctx, ch, ch->fine_volume_slide_param << 4);
          break;

        case 0xB: /* EBy: Fine volume slide down */
          if ((s->effect_param & 0x0F) == 0 && CHANNEL_IS_MOD(ch))
            break;

          if (s->effect_param & 0x0F)
            ch->fine_volume_slide_param = s->effect_param & 0x0F;

          xm_volume_slide(ctx, ch, ch->fine_volume_slide_param);
          break;

        case 0xD: /* EDy: Note delay */
          /* XXX: figure this out better. EDx triggers
           * the note even when there no note and no
           * instrument. But ED0 acts like a ghost
           * note, EDx (x ≠ 0) does not. */
          if (s->note == 0 && s->instrument == 0)
          {
            unsigned int flags = XM_TRIGGER_KEEP_VOLUME;

            if (ch->current->effect_param & 0x0F)
            {
              ch->note = ch->orig_note;
              xm_trigger_note(ctx, ch, flags);
            }
            else
            {
              xm_trigger_note(
                ctx, ch,
                flags
                | XM_TRIGGER_KEEP_PERIOD
                | XM_TRIGGER_KEEP_SAMPLE_POSITION
                );
            }
          }
          break;

        case 0xE: /* EEy: Pattern delay */
          ctx->extra_ticks = (ch->current->effect_param & 0x0F) * ctx->tempo;
          break;

        case 0xF: /* EFy: Invert loop (MOD only) */
          if (CHANNEL_IS_MOD(ch))
            ch->invert_loop_speed = ch->current->effect_param & 0x0F;
          break;

        default:
          break;
      }
      break;

    case XM_EFFECT_S3M_EXTENDED: /* S3M Sxy: Extended command */
      switch (s->effect_param >> 4)
      {
        case 0x1: /* S1x: Glissando control */
          ch->s3m_glissando_control = (s->effect_param & 0x0F) != 0;
          break;

        case 0x2: /* S2x: Set finetune */
          if (ch->sample != NULL)
          {
            uint8_t finetune = s->effect_param & 0x0F;
            int8_t signed_finetune = finetune < 8 ? finetune : finetune - 16;

            ch->period_note_offset = (float)signed_finetune / 8.f;
            xm_update_frequency(ctx, ch);
          }
          break;

        case 0x3: /* S3x: Set vibrato control */
          ch->vibrato_waveform = (xm_waveform_type_t)(s->effect_param & 3);
          ch->vibrato_waveform_retrigger = !((s->effect_param >> 2) & 1);
          break;

        case 0x4: /* S4x: Set tremolo control */
          ch->tremolo_waveform = (xm_waveform_type_t)(s->effect_param & 3);
          ch->tremolo_waveform_retrigger = !((s->effect_param >> 2) & 1);
          break;

        case 0x5: /* S5x: Set panbrello control */
          ch->panbrello_waveform = (xm_waveform_type_t)(s->effect_param & 3);
          ch->panbrello_waveform_retrigger = !((s->effect_param >> 2) & 1);
          break;

        case 0x6: /* S6x: Fine pattern delay, handled once per row in xm_row() */
          break;

        case 0x8: /* S8x: Set 4-bit panning */
          ch->surround = false;
          ch->panning = (float)(s->effect_param & 0x0F) / (float)0x0F;
          break;

        case 0x9: /* S9x: Sound control, only surround on/off is represented in the current mixer */
          if ((s->effect_param & 0x0F) == 0)
            ch->surround = false;
          else if ((s->effect_param & 0x0F) == 1)
          {
            ch->surround = true;
            ch->panning = .5f;
          }
          break;

        case 0xA: /* SAx: High sample offset */
          ch->s3m_high_sample_offset_param = s->effect_param & 0x0F;
          break;

        case 0xB: /* SBx: Pattern loop */
          if (s->effect_param & 0x0F)
          {
            if ((s->effect_param & 0x0F) == ch->pattern_loop_count)
            {
              ch->pattern_loop_count = 0;
              break;
            }

            ch->pattern_loop_count++;
            ctx->position_jump = true;
            ctx->jump_row      = ch->pattern_loop_origin;
            ctx->jump_dest     = ctx->current_table_index;
          }
          else
          {
            ch->pattern_loop_origin = ctx->current_row;
            ctx->jump_row = ch->pattern_loop_origin;
          }
          break;

        case 0xD: /* SDx: Note delay, delayed note is triggered from xm_tick() */
          break;

        case 0xE: /* SEx: Pattern delay, handled once per row in xm_row() */
          break;

        default:
          break;
      }
      break;

    case 0xF: /* Fxx: Set tempo/BPM */
      if (s->effect_param > 0)
      {
        if (s->effect_param <= 0x1F)
          ctx->tempo = s->effect_param;
        else
          ctx->bpm = s->effect_param;
      }
      break;

    case 16: /* Gxx: Set global volume */
    case XM_EFFECT_S3M_GLOBAL_VOLUME: /* S3M Vxx: Set global volume */
      ctx->global_volume = (float)((s->effect_param > 0x40)
                                                                         ? 0x40 : s->effect_param) / (float)0x40;
      break;

    case 17: /* Hxy: Global volume slide */
    case XM_EFFECT_S3M_GLOBAL_VOLUME_SLIDE: /* S3M Wxy: Global volume slide */
      if (s->effect_param > 0)
        ch->global_volume_slide_param = s->effect_param;
      break;

    case 21: /* Lxx: Set envelope position */
      ch->volume_envelope_frame_count  = s->effect_param;
      ch->panning_envelope_frame_count = s->effect_param;
      break;

    case 25: /* Pxy: Panning slide */
    case XM_EFFECT_S3M_PANNING_SLIDE: /* S3M Pxy: Panning slide */
      if (s->effect_param > 0)
        ch->panning_slide_param = s->effect_param;
      break;

    case XM_EFFECT_S3M_PANBRELLO: /* S3M Yxy: Panbrello */
      if (s->effect_param & 0x0F)
        ch->panbrello_param = (ch->panbrello_param & 0xF0) | (s->effect_param & 0x0F);
      if (s->effect_param >> 4)
        ch->panbrello_param = (s->effect_param & 0xF0) | (ch->panbrello_param & 0x0F);
      break;

    case 27: /* Rxy/Qxy: Multi retrig note */
    case XM_EFFECT_S3M_RETRIG: /* S3M Qxy: Retrigger */
      if (s->effect_param > 0)
      {
        if (CHANNEL_IS_S3M(ch))
          ch->multi_retrig_param = s->effect_param;
        else if ((s->effect_param >> 4) == 0)  /* Keep previous x value */
          ch->multi_retrig_param = (ch->multi_retrig_param & 0xF0) | (s->effect_param & 0x0F);
        else
          ch->multi_retrig_param = s->effect_param;
      }
      break;

    case 29: /* Txy: Tremor */
    case XM_EFFECT_S3M_TREMOR: /* S3M Ixy: Tremor */
      if (s->effect_param > 0)  /* Tremor x and y params do not appear to be separately kept in memory, unlike Rxy */
        ch->tremor_param = s->effect_param;
      break;

    case XM_EFFECT_S3M_NONE: /* S3M no-op with parameter: only updates shared memory */
      break;

    case XM_EFFECT_S3M_SPEED: /* S3M Axx: handled once per row in xm_row() */
      break;

    case XM_EFFECT_S3M_TEMPO: /* S3M Txx: handled once per row in xm_row() */
      break;

    case XM_EFFECT_S3M_FINE_VIBRATO: /* S3M Uxy: Fine vibrato */
      if (s->effect_param & 0x0F)
        ch->vibrato_param = (ch->vibrato_param & 0xF0) | (s->effect_param & 0x0F);
      if (s->effect_param >> 4)
        ch->vibrato_param = (s->effect_param & 0xF0) | (ch->vibrato_param & 0x0F);
      break;

    case XM_EFFECT_S3M_CHANNEL_VOLUME: /* S3M Mxx: Set channel volume */
      if (s->effect_param <= 0x40)
        ch->channel_volume = (float)s->effect_param / (float)0x40;
      break;

    case XM_EFFECT_S3M_CHANNEL_VOLUME_SLIDE: /* S3M Nxy: Channel volume slide */
      if (s->effect_param > 0)
        ch->channel_volume_slide_param = s->effect_param;
      break;

    case 33: /* Xxy: Extra stuff */
      switch (s->effect_param >> 4)
      {
        case 1: /* X1y: Extra fine portamento up */
          if (s->effect_param & 0x0F)
            ch->extra_fine_portamento_up_param = s->effect_param & 0x0F;

          xm_pitch_slide(ctx, ch, -1.0f * ch->extra_fine_portamento_up_param);
          break;

        case 2: /* X2y: Extra fine portamento down */
          if (s->effect_param & 0x0F)
            ch->extra_fine_portamento_down_param = s->effect_param & 0x0F;

          xm_pitch_slide(ctx, ch, ch->extra_fine_portamento_down_param);
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }
}

static void xm_trigger_note(xm_context_t* ctx, xm_channel_context_t* ch, unsigned int flags)
{
  if (!(flags & XM_TRIGGER_KEEP_SAMPLE_POSITION))
  {
    ch->sample_position = 0.f;
    ch->ping            = true;
  }

  if (ch->sample != NULL)
  {
    if (!(flags & XM_TRIGGER_KEEP_VOLUME))
      ch->volume = ch->sample->volume;

    if (CHANNEL_HAS_DEFAULT_PANNING(ch))
      ch->panning = ch->default_panning;
    else
      ch->panning = ch->sample->panning;
  }

  ch->sustained                   = true;
  ch->fadeout_volume              = ch->volume_envelope_volume = 1.0f;
  ch->panning_envelope_panning    = .5f;
  ch->volume_envelope_frame_count = ch->panning_envelope_frame_count = 0;
  ch->vibrato_note_offset         = 0.f;
  ch->vibrato_period_offset       = .0f;
  ch->tremolo_volume              = 0.f;
  ch->panbrello_panning_offset    = .0f;
  ch->panbrello_in_progress       = false;
  ch->s3m_note_cut              = false;
  ch->tremor_on                   = false;

  ch->autovibrato_ticks = 0;

  if (ch->vibrato_waveform_retrigger)
    ch->vibrato_ticks = 0;             /* XXX: should the waveform itself also be reset to sine? */

  if (ch->tremolo_waveform_retrigger)
    ch->tremolo_ticks = 0;

  if (ch->panbrello_waveform_retrigger)
    ch->panbrello_ticks = 0;

  if (!(flags & XM_TRIGGER_KEEP_PERIOD))
  {
    if (ch->current != NULL && ch->current->period != 0)
      ch->period = ch->current->period;
    else
      ch->period = xm_period(ctx, ch->note);
    xm_update_frequency(ctx, ch);
  }

  ch->s3m_opl_trigger_id++;
  ch->latest_trigger = ctx->generated_samples;
  if (ch->instrument != NULL)
    ch->instrument->latest_trigger = ctx->generated_samples;

  if (ch->sample != NULL)
    ch->sample->latest_trigger = ctx->generated_samples;
}

static void xm_cut_note(xm_channel_context_t* ch)
{
  /* NB: this is not the same as Key Off */
  ch->volume = .0f;
  if (CHANNEL_IS_S3M(ch))
  {
    ch->sample_position = -1.f;
    ch->s3m_note_cut = true;
  }
}

static void xm_key_off(xm_channel_context_t* ch)
{
  /* Key Off */
  ch->sustained = false;

  /* If no volume envelope is used, also cut the note */
  if (ch->instrument == NULL || !ch->instrument->volume_envelope.enabled)
    xm_cut_note(ch);
}

static void xm_row(xm_context_t* ctx)
{
  if (ctx->module.length == 0 || ctx->module.num_channels == 0 ||
      ctx->module.num_patterns == 0 || ctx->module.patterns == NULL ||
      ctx->channels == NULL || ctx->row_loop_count == NULL)
    return;

  if (ctx->position_jump)
  {
    ctx->current_table_index = ctx->jump_dest;
    ctx->current_row         = ctx->jump_row;
    ctx->position_jump       = false;
    ctx->pattern_break       = false;
    ctx->jump_row            = 0;
    xm_post_pattern_change(ctx);
  }
  else if (ctx->pattern_break)
  {
    ctx->current_table_index++;
    ctx->current_row   = ctx->jump_row;
    ctx->pattern_break = false;
    ctx->jump_row      = 0;
    xm_post_pattern_change(ctx);
  }

  if (ctx->current_table_index >= ctx->module.length)
    xm_post_pattern_change(ctx);

  if (ctx->current_table_index >= ctx->module.length)
    return;

  uint8_t pattern_index = ctx->module.pattern_table[ctx->current_table_index];

  if (pattern_index >= ctx->module.num_patterns)
  {
    ctx->current_table_index = 0;
    ctx->current_row = 0;
    return;
  }

  xm_pattern_t* cur = ctx->module.patterns + pattern_index;

  if (cur->slots == NULL || cur->num_rows == 0)
  {
    ctx->current_table_index++;
    ctx->current_row = 0;
    xm_post_pattern_change(ctx);
    return;
  }

  if (ctx->current_row >= cur->num_rows)
    ctx->current_row = 0;

  if (ctx->tracker_format == TRACKER_FORMAT_S3M)
  {
    bool s3m_position_jump = false;
    bool s3m_pattern_break = false;
    bool s3m_pattern_delay = false;
    uint8_t s3m_jump_dest = 0;
    uint8_t s3m_jump_row = 0;
    uint8_t s3m_pattern_delay_param = 0;
    uint8_t s3m_fine_pattern_delay_ticks = 0;

    ctx->s3m_tempo_slide_param = 0;

    for (uint8_t i = 0; i < ctx->module.num_channels; ++i)
    {
      xm_pattern_slot_t* s = cur->slots + ctx->current_row * ctx->module.num_channels + i;

      switch (s->effect_type)
      {
        case XM_EFFECT_S3M_SPEED: /* S3M Axx: Set speed */
          if (s->effect_param > 0)
            ctx->tempo = s->effect_param;
          break;

        case XM_EFFECT_S3M_POSITION_JUMP: /* S3M Bxx: Position jump */
          if (s->effect_param < ctx->module.length)
          {
            s3m_position_jump = true;
            s3m_jump_dest = s->effect_param;
          }
          break;

        case XM_EFFECT_S3M_PATTERN_BREAK: /* S3M Cxx: Pattern break */
        {
          uint8_t row = (s->effect_param >> 4) * 10 + (s->effect_param & 0x0F);
          if (row < 64)
          {
            s3m_pattern_break = true;
            s3m_jump_row = row;
          }
          break;
        }

        case XM_EFFECT_S3M_TEMPO: /* S3M Txx: Set tempo / tempo slide */
          if (s->effect_param >= 0x20)
            ctx->bpm = s->effect_param;
          else if (s->effect_param > 0)
            ctx->s3m_tempo_slide_param = s->effect_param;
          break;

        case XM_EFFECT_S3M_EXTENDED: /* S3M Sxy: Extended command */
        {
          uint8_t param = s->effect_param;
          if (!param)
          {
            xm_channel_context_t* ch = ctx->channels + i;
            param = ch->s3m_effect_memory;
          }

          switch (param >> 4)
          {
            case 0x6: /* S6x: Fine pattern delay */
              s3m_fine_pattern_delay_ticks += param & 0x0F;
              break;

            case 0xE: /* SEx: Pattern delay */
              if ((param & 0x0F) && !s3m_pattern_delay)
              {
                s3m_pattern_delay_param = param & 0x0F;
                s3m_pattern_delay = true;
              }
              break;

            default:
              break;
          }
          break;
        }

        default:
          break;
      }
    }

    if (s3m_pattern_delay)
      ctx->extra_ticks = s3m_pattern_delay_param * ctx->tempo;
    ctx->extra_ticks += s3m_fine_pattern_delay_ticks;

    if (s3m_position_jump)
    {
      ctx->position_jump = true;
      ctx->jump_dest = s3m_jump_dest;
      ctx->jump_row = s3m_pattern_break ? s3m_jump_row : 0;
      ctx->pattern_break = false;
    }
    else if (s3m_pattern_break)
    {
      ctx->pattern_break = true;
      ctx->jump_row = s3m_jump_row;
    }
  }

  bool in_a_loop = false;

  /* Read notes… */
  for (uint8_t i = 0; i < ctx->module.num_channels; ++i)
  {
    xm_pattern_slot_t   * s  = cur->slots + ctx->current_row * ctx->module.num_channels + i;
    xm_channel_context_t* ch = ctx->channels + i;

    ch->current = s;

    uint8_t effect_param = s->effect_param;
    if (s->effect_type == XM_EFFECT_S3M_EXTENDED)
    {
      if (effect_param)
        ch->s3m_effect_memory = effect_param;
      else
        effect_param = ch->s3m_effect_memory;
    }

    if (!((s->effect_type == 0xE && (s->effect_param >> 4) == 0xD) ||
          (s->effect_type == XM_EFFECT_S3M_EXTENDED && (effect_param >> 4) == 0xD)))
      xm_handle_note_and_instrument(ctx, ch, s);
    else if (CHANNEL_IS_S3M(ch))
    {
      uint8_t delay = effect_param & 0x0F;
      ch->note_delay_param = (delay > 0 && delay < ctx->tempo) ? delay : 0xFF;
    }
    else
      ch->note_delay_param = effect_param & 0x0F;

    if (!in_a_loop && ch->pattern_loop_count > 0)
      in_a_loop = true;
  }

  if (!in_a_loop)
  {
    /* No E6y loop is in effect (or we are in the first pass) */
    ctx->loop_count = (ctx->row_loop_count[MAX_NUM_ROWS * ctx->current_table_index + ctx->current_row]++);
  }

  ctx->current_row++;       /* Since this is an uint8, this line can increment from 255 to 0, in which case it is still necessary to go the next pattern. */

  if (!ctx->position_jump && !ctx->pattern_break &&
      (ctx->current_row >= cur->num_rows || ctx->current_row == 0))
  {
    ctx->current_table_index++;
    ctx->current_row = ctx->jump_row;             /* This will be 0 most of the time, except when E60 is used */
    ctx->jump_row    = 0;
    xm_post_pattern_change(ctx);
  }
}

static void xm_envelope_tick(xm_channel_context_t* ch, xm_envelope_t* env, uint16_t* counter, float* outval)
{
  if (env->num_points < 2)
  {
    /* Don't really know what to do… */
    if (env->num_points == 1)
    {
      /* XXX I am pulling this out of my ass */
      *outval = (float)env->points[0].value / (float)0x40;

      if (*outval > 1)
        *outval = 1;
    }

    return;
  }
  else
  {
    uint8_t j;

    if (env->loop_enabled)
    {
      uint16_t loop_start  = env->points[env->loop_start_point].frame;
      uint16_t loop_end    = env->points[env->loop_end_point].frame;
      uint16_t loop_length = loop_end - loop_start;

      if (*counter >= loop_end)
        *counter -= loop_length;
    }

    for (j = 0; j < (env->num_points - 2); ++j)
    {
      if (env->points[j].frame <= *counter && env->points[j + 1].frame >= *counter)
        break;
    }

    *outval = xm_envelope_lerp(env->points + j, env->points + j + 1, *counter) / (float)0x40;

    /* Make sure it is safe to increment frame count */
    if (!ch->sustained || !env->sustain_enabled || *counter != env->points[env->sustain_point].frame)
      (*counter)++;
  }
}

static void xm_envelopes(xm_channel_context_t* ch)
{
  if (ch->instrument != NULL)
  {
    if (ch->instrument->volume_envelope.enabled)
    {
      if (!ch->sustained)
      {
        ch->fadeout_volume -= (float)ch->instrument->volume_fadeout / 65536.f;
        XM_CLAMP_DOWN(ch->fadeout_volume);
      }

      xm_envelope_tick(ch,
                       &(ch->instrument->volume_envelope),
                       &(ch->volume_envelope_frame_count),
                       &(ch->volume_envelope_volume));
    }

    if (ch->instrument->panning_envelope.enabled)
    {
      xm_envelope_tick(ch,
                       &(ch->instrument->panning_envelope),
                       &(ch->panning_envelope_frame_count),
                       &(ch->panning_envelope_panning));
    }
  }
}

static void xm_tick(xm_context_t* ctx)
{
  if (ctx->current_tick == 0)
    xm_row(ctx);

  for (uint8_t i = 0; i < ctx->module.num_channels; ++i)
  {
    xm_channel_context_t* ch = ctx->channels + i;

    xm_envelopes(ch);
    xm_autovibrato(ctx, ch);

    if (ch->arp_in_progress && !HAS_ARPEGGIO(ch->current))
    {
      ch->arp_in_progress = false;
      ch->arp_note_offset = 0;
      xm_update_frequency(ctx, ch);
    }

    if (ch->vibrato_in_progress && !HAS_VIBRATO(ch->current))
    {
      ch->vibrato_in_progress = false;
      ch->vibrato_note_offset = 0.f;
      ch->vibrato_period_offset = .0f;
      xm_update_frequency(ctx, ch);
    }

    if (ch->panbrello_in_progress && ch->current->effect_type != XM_EFFECT_S3M_PANBRELLO)
    {
      ch->panbrello_in_progress = false;
      ch->panbrello_panning_offset = .0f;
    }

    if (!(CHANNEL_IS_S3M(ch) && ch->current->volume_column >= 128 && ch->current->volume_column <= 192))
    {
      switch (ch->current->volume_column >> 4)
      {
      case 0x6: /* Volume slide down */
        if (ctx->current_tick == 0)
          break;

        xm_volume_slide(ctx, ch, ch->current->volume_column & 0x0F);
        break;

      case 0x7: /* Volume slide up */
        if (ctx->current_tick == 0)
          break;

        xm_volume_slide(ctx, ch, ch->current->volume_column << 4);
        break;

      case 0xB: /* Vibrato */
        if (ctx->current_tick == 0)
          break;

        ch->vibrato_in_progress = false;
        xm_vibrato(ctx, ch, ch->vibrato_param, ch->vibrato_ticks++);
        break;

      case 0xD: /* Panning slide left */
        if (ctx->current_tick == 0)
          break;

        xm_panning_slide(ch, ch->current->volume_column & 0x0F);
        break;

      case 0xE: /* Panning slide right */
        if (ctx->current_tick == 0)
          break;

        xm_panning_slide(ch, ch->current->volume_column << 4);
        break;

      case 0xF: /* Tone portamento */
        if (ctx->current_tick == 0)
          break;

        xm_tone_portamento(ctx, ch);
        break;

        default:
          break;
      }
    }

    switch (ch->current->effect_type)
    {
      case 0: /* 0xy: XM/MOD arpeggio or no-op */
      case XM_EFFECT_S3M_ARPEGGIO: /* S3M Jxy: Arpeggio */
      {
        uint8_t param = ch->current->effect_param;
        if (ch->current->effect_type == 0 && CHANNEL_IS_S3M(ch))
          break;
        if (ch->current->effect_type == XM_EFFECT_S3M_ARPEGGIO && param == 0)
          param = ch->arpeggio_param;

        if (param > 0)
        {
          if (CHANNEL_USES_PERIOD_EFFECTS(ch))
          {
            xm_arpeggio(ctx, ch, param, ctx->current_tick);
            break;
          }

          char arp_offset = ctx->tempo % 3;

          switch (arp_offset)
          {
            case 2: /* 0 -> x -> 0 -> y -> x -> … */
              if (ctx->current_tick == 1)
              {
                ch->arp_in_progress = true;
                ch->arp_note_offset = param >> 4;
                xm_update_frequency(ctx, ch);
                break;
              }

            /* No break here, this is intended */
            case 1: /* 0 -> 0 -> y -> x -> … */
              if (ctx->current_tick == 0)
              {
                ch->arp_in_progress = false;
                ch->arp_note_offset = 0;
                xm_update_frequency(ctx, ch);
                break;
              }

            /* No break here, this is intended */
            case 0: /* 0 -> y -> x -> … */
              xm_arpeggio(ctx, ch, param, ctx->current_tick - arp_offset);

            default:
              break;
          }
        }
        break;
      }

      case 1: /* 1xx: Portamento up */
      case XM_EFFECT_S3M_PORTAMENTO_UP: /* S3M Fxx: Portamento up */
        if (CHANNEL_IS_S3M(ch))
        {
          uint8_t param = ch->portamento_up_param;
          if (!param) break;

          if (ctx->current_tick == 0)
          {
            if ((param & 0xF0) == 0xE0)
              xm_pitch_slide(ctx, ch, -0.25f * (float)(param & 0x0F));
            else if ((param & 0xF0) == 0xF0)
              xm_pitch_slide(ctx, ch, -(float)(param & 0x0F));
          }
          else if ((param & 0xF0) < 0xE0)
            xm_pitch_slide(ctx, ch, -param);
          break;
        }

        if (ctx->current_tick == 0 || (CHANNEL_IS_MOD(ch) && ch->current->effect_param == 0))
          break;

        xm_pitch_slide(ctx, ch, -ch->portamento_up_param);
        break;

      case 2: /* 2xx: Portamento down */
      case XM_EFFECT_S3M_PORTAMENTO_DOWN: /* S3M Exx: Portamento down */
        if (CHANNEL_IS_S3M(ch))
        {
          uint8_t param = ch->portamento_down_param;
          if (!param) break;

          if (ctx->current_tick == 0)
          {
            if ((param & 0xF0) == 0xE0)
              xm_pitch_slide(ctx, ch, 0.25f * (float)(param & 0x0F));
            else if ((param & 0xF0) == 0xF0)
              xm_pitch_slide(ctx, ch, (float)(param & 0x0F));
          }
          else if ((param & 0xF0) < 0xE0)
            xm_pitch_slide(ctx, ch, param);
          break;
        }

        if (ctx->current_tick == 0 || (CHANNEL_IS_MOD(ch) && ch->current->effect_param == 0))
          break;

        xm_pitch_slide(ctx, ch, ch->portamento_down_param);
        break;

      case 3: /* 3xx: Tone portamento */
      case XM_EFFECT_S3M_TONE_PORTAMENTO: /* S3M Gxx: Tone portamento */
        if (ctx->current_tick == 0)
          break;

        xm_tone_portamento(ctx, ch);
        break;

      case 4: /* 4xy: Vibrato */
      case XM_EFFECT_S3M_VIBRATO: /* S3M Hxy: Vibrato */
        if (ctx->current_tick == 0)
          break;

        ch->vibrato_in_progress = true;
        xm_vibrato(ctx, ch, ch->vibrato_param, ch->vibrato_ticks++);
        break;

      case 5: /* 5xy: Tone portamento + Volume slide */
      case XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE: /* S3M Lxy: Tone portamento + volume slide */
        if (ctx->current_tick == 0)
          break;

        xm_tone_portamento(ctx, ch);
        if (ch->current->effect_param != 0 || !CHANNEL_IS_MOD(ch))
          xm_volume_slide(ctx, ch, ch->volume_slide_param);
        break;

      case 6: /* 6xy: Vibrato + Volume slide */
      case XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE: /* S3M Kxy: Vibrato + volume slide */
        if (ctx->current_tick == 0)
          break;

        ch->vibrato_in_progress = true;
        xm_vibrato(ctx, ch, ch->vibrato_param, ch->vibrato_ticks++);
        if (ch->current->effect_param != 0 || !CHANNEL_IS_MOD(ch))
          xm_volume_slide(ctx, ch, ch->volume_slide_param);
        break;

      case 7: /* 7xy: Tremolo */
      case XM_EFFECT_S3M_TREMOLO: /* S3M Rxy: Tremolo */
        if (ctx->current_tick == 0)
          break;

        xm_tremolo(ctx, ch, ch->tremolo_param, ch->tremolo_ticks++);
        break;

      case 0xA: /* Axy: Volume slide */
      case XM_EFFECT_S3M_VOLUME_SLIDE: /* S3M Dxy: Volume slide */
        if (!CHANNEL_IS_S3M(ch) && ctx->current_tick == 0)
          break;
        if (ch->current->effect_param == 0 && CHANNEL_IS_MOD(ch))
          break;

        xm_volume_slide(ctx, ch, ch->volume_slide_param);
        break;

      case 0xE: /* EXy: Extended command */
        switch (ch->current->effect_param >> 4)
        {
          case 0x9: /* E9y: Retrigger note */
            if (ctx->current_tick != 0 && ch->current->effect_param & 0x0F)
            {
              if (!(ctx->current_tick % (ch->current->effect_param & 0x0F)))
              {
                xm_trigger_note(ctx, ch, 0);
                xm_envelopes(ch);
              }
            }
            break;

          case 0xC: /* ECy: Note cut */
            if ((ch->current->effect_param & 0x0F) == ctx->current_tick)
              xm_cut_note(ch);
            break;

          case 0xD: /* EDy: Note delay */
            if (ch->note_delay_param == ctx->current_tick)
            {
              xm_handle_note_and_instrument(ctx, ch, ch->current);
              xm_envelopes(ch);
            }
            break;

          default:
            break;
        }
        break;

      case XM_EFFECT_S3M_EXTENDED: /* S3M Sxy: Extended command */
      {
        uint8_t param = ch->current->effect_param;
        if (!param)
          param = ch->s3m_effect_memory;

        switch (param >> 4)
        {
          case 0xC: /* SCx: Note cut */
          {
            uint8_t tick = param & 0x0F;
            if (tick > 0 && tick < ctx->tempo && tick == ctx->current_tick)
              xm_cut_note(ch);
            break;
          }

          case 0xD: /* SDx: Note delay */
            if (ch->note_delay_param == ctx->current_tick)
            {
              xm_handle_note_and_instrument(ctx, ch, ch->current);
              xm_envelopes(ch);
            }
            break;

          default:
            break;
        }
        break;
      }

      case 17: /* Hxy: Global volume slide */
      case XM_EFFECT_S3M_GLOBAL_VOLUME_SLIDE: /* S3M Wxy: Global volume slide */
      {
        uint8_t param = ch->global_volume_slide_param;
        if (CHANNEL_IS_S3M(ch))
        {
          if ((param & 0x0F) == 0x0F && (param & 0xF0))
          {
            if (ctx->current_tick == 0)
            {
              float f = (float)(param >> 4) / (float)0x40;
              ctx->global_volume += f;
              XM_CLAMP_UP(ctx->global_volume);
            }
            break;
          }

          if ((param & 0xF0) == 0xF0 && (param & 0x0F))
          {
            if (ctx->current_tick == 0)
            {
              float f = (float)(param & 0x0F) / (float)0x40;
              ctx->global_volume -= f;
              XM_CLAMP_DOWN(ctx->global_volume);
            }
            break;
          }

          if (ctx->current_tick == 0 && !ctx->s3m_fast_volume_slides)
            break;

          if (param & 0x0F)
          {
            float f = (float)(param & 0x0F) / (float)0x40;
            ctx->global_volume -= f;
            XM_CLAMP_DOWN(ctx->global_volume);
          }
          else
          {
            float f = (float)(param >> 4) / (float)0x40;
            ctx->global_volume += f;
            XM_CLAMP_UP(ctx->global_volume);
          }
          break;
        }

        if (ctx->current_tick == 0)
          break;

        if ((param & 0xF0) && (param & 0x0F))  /* Illegal state */
          break;

        if (param & 0xF0)
        {
          /* Global slide up */
          float f = (float)(param >> 4) / (float)0x40;
          ctx->global_volume += f;
          XM_CLAMP_UP(ctx->global_volume);
        }
        else
        {
          /* Global slide down */
          float f = (float)(param & 0x0F) / (float)0x40;
          ctx->global_volume -= f;
          XM_CLAMP_DOWN(ctx->global_volume);
        }
        break;
      }

      case 20: /* Kxx: Key off */
        /* Most documentations will tell you the parameter has no
         * use. Don't be fooled. */
        if (ctx->current_tick == ch->current->effect_param)
          xm_key_off(ch);
        break;

      case 25: /* Pxy: Panning slide */
        if (ctx->current_tick == 0)
          break;

        xm_panning_slide(ch, ch->panning_slide_param);
        break;

      case XM_EFFECT_S3M_PANNING_SLIDE: /* S3M Pxy: Panning slide */
      {
        uint8_t param = ch->panning_slide_param;

        if (!param)
          break;

        if ((param & 0x0F) == 0x0F && (param & 0xF0))
        {
          if (ctx->current_tick == 0)
          {
            ch->panning -= (float)(param >> 4) / 64.f;
            XM_CLAMP_DOWN(ch->panning);
          }
          break;
        }

        if ((param & 0xF0) == 0xF0 && (param & 0x0F))
        {
          if (ctx->current_tick == 0)
          {
            ch->panning += (float)(param & 0x0F) / 64.f;
            XM_CLAMP_UP(ch->panning);
          }
          break;
        }

        if (ctx->current_tick == 0 || ((param & 0xF0) && (param & 0x0F)))
          break;

        if (param & 0x0F)
        {
          ch->panning += (float)(param & 0x0F) / 64.f;
          XM_CLAMP_UP(ch->panning);
        }
        else
        {
          ch->panning -= (float)(param >> 4) / 64.f;
          XM_CLAMP_DOWN(ch->panning);
        }
        break;
      }

      case XM_EFFECT_S3M_PANBRELLO: /* S3M Yxy: Panbrello */
        if (ctx->current_tick == 0)
          break;

        ch->panbrello_in_progress = true;
        xm_panbrello(ch, ch->panbrello_param, ch->panbrello_ticks++);
        break;

      case 27: /* Rxy: Multi retrig note */
      case XM_EFFECT_S3M_RETRIG: /* S3M Qxy: Retrigger */
      {
        uint8_t retrig_ticks;

        if (ctx->current_tick == 0)
          break;

        if (CHANNEL_IS_S3M(ch) && ch->s3m_note_cut)
          break;

        retrig_ticks = ch->multi_retrig_param & 0x0F;
        if (!retrig_ticks)
        {
          if (!CHANNEL_IS_S3M(ch))
            break;
          retrig_ticks = 1;
        }

        if ((ctx->current_tick % retrig_ticks) == 0)
        {
          float v = ch->volume * multi_retrig_multiply[ch->multi_retrig_param >> 4] + multi_retrig_add[ch->multi_retrig_param >> 4];
          XM_CLAMP(v);
          xm_trigger_note(ctx, ch, 0);
          ch->volume = v;
        }
        break;
      }

      case 29: /* Txy: Tremor */
      case XM_EFFECT_S3M_TREMOR: /* S3M Ixy: Tremor */
        if (ctx->current_tick == 0)
          break;

        if (CHANNEL_IS_S3M(ch))
        {
          uint8_t tremor_on_ticks = ch->tremor_param >> 4;
          uint8_t tremor_off_ticks = ch->tremor_param & 0x0F;
          uint8_t tremor_cycle = tremor_on_ticks + tremor_off_ticks;

          if (tremor_cycle)
            ch->tremor_on = ((ctx->current_tick - 1) % tremor_cycle) >= tremor_on_ticks;
        }
        else
        {
          ch->tremor_on = (
            (ctx->current_tick - 1) % ((ch->tremor_param >> 4) + (ch->tremor_param & 0x0F) + 2)
            >
            (ch->tremor_param >> 4)
            );
        }
        break;

      case XM_EFFECT_S3M_FINE_VIBRATO: /* S3M Uxy: Fine vibrato */
        if (ctx->current_tick == 0)
          break;

        ch->vibrato_in_progress = true;
        ch->vibrato_note_offset = .0f;
        ch->vibrato_period_offset = -xm_waveform(ch->vibrato_waveform, ch->vibrato_ticks++ * (ch->vibrato_param >> 4)) * (float)(ch->vibrato_param & 0x0F) / 2.f;
        xm_update_frequency(ctx, ch);
        break;

      case XM_EFFECT_S3M_CHANNEL_VOLUME_SLIDE: /* S3M Nxy: Channel volume slide */
      {
        uint8_t param = ch->channel_volume_slide_param;
        if (!param)
          break;

        if ((param & 0x0F) == 0x0F && (param & 0xF0))
        {
          if (ctx->current_tick == 0)
          {
            ch->channel_volume += (float)(param >> 4) / (float)0x40;
            XM_CLAMP_UP(ch->channel_volume);
          }
          break;
        }

        if ((param & 0xF0) == 0xF0 && (param & 0x0F))
        {
          if (ctx->current_tick == 0)
          {
            ch->channel_volume -= (float)(param & 0x0F) / (float)0x40;
            XM_CLAMP_DOWN(ch->channel_volume);
          }
          break;
        }

        if (ctx->current_tick == 0 && !ctx->s3m_fast_volume_slides)
          break;

        if (param & 0x0F)
        {
          ch->channel_volume -= (float)(param & 0x0F) / (float)0x40;
          XM_CLAMP_DOWN(ch->channel_volume);
        }
        else
        {
          ch->channel_volume += (float)(param >> 4) / (float)0x40;
          XM_CLAMP_UP(ch->channel_volume);
        }
        break;
      }

      default:
        break;
    }

    if (CHANNEL_IS_MOD(ch) && ch->invert_loop_speed &&
        (ctx->current_tick != 0 ||
         (ch->current->effect_type == 0xE && (ch->current->effect_param >> 4) == 0xF)))
      xm_invert_loop(ch);

    float panning, volume;

    panning = ch->panning + ch->panbrello_panning_offset;
    XM_CLAMP(panning);
    panning += (ch->panning_envelope_panning - .5f) * (.5f - fabsf(panning - .5f)) * 2.0f;
    XM_CLAMP(panning);

    if (ch->tremor_on)
      volume = .0f;
    else
    {
      volume = ch->volume + ch->tremolo_volume;
      XM_CLAMP(volume);
      volume *= ch->fadeout_volume * ch->volume_envelope_volume * ch->channel_volume;
    }

#if XM_RAMPING
    ch->target_panning = panning;
    ch->target_volume  = volume;
#else
    ch->actual_panning = panning;
    ch->actual_volume  = volume;
#endif

    s3m_adlib_sync_channel(ctx, ch);
  }

  if (ctx->tracker_format == TRACKER_FORMAT_S3M && ctx->current_tick > 0)
  {
    uint8_t param = ctx->s3m_tempo_slide_param;
    if ((param & 0xF0) == 0x00 && (param & 0x0F))
    {
      if (ctx->bpm > (param & 0x0F))
        ctx->bpm -= param & 0x0F;
      else
        ctx->bpm = 1;
    }
    else if ((param & 0xF0) == 0x10)
      ctx->bpm += param & 0x0F;
  }

  ctx->current_tick++;
  if (ctx->current_tick >= ctx->tempo + ctx->extra_ticks)
  {
    ctx->current_tick = 0;
    ctx->extra_ticks  = 0;
  }

  /* FT2 manual says number of ticks / second = BPM * 0.4 */
  ctx->remaining_samples_in_tick += (float)ctx->rate / ((float)ctx->bpm * 0.4f);
}

static float IRAM_ATTR xm_sample_at(xm_sample_t* sample, size_t k)
{
  return sample->bits == 8 ? (sample->data8[k] / 128.f) : (sample->data16[k] / 32768.f);
}

static float IRAM_ATTR xm_next_of_sample(xm_channel_context_t* ch)
{
  if (ch->instrument == NULL || ch->sample == NULL || ch->sample_position < 0)
  {
#if XM_RAMPING
    if (ch->frame_count < XM_SAMPLE_RAMPING_POINTS)
    {
      return XM_LERP(ch->end_of_previous_sample[ch->frame_count], .0f,
                     (float)ch->frame_count / (float)XM_SAMPLE_RAMPING_POINTS);
    }
#endif

    return .0f;
  }

  if (ch->sample->length == 0)
    return .0f;

  float    u, v, t;
  uint32_t a, b;
  a = (uint32_t)ch->sample_position;       /* This cast is fine, sample_position will not go above integer ranges */

  if (XM_LINEAR_INTERPOLATION)
  {
    b = a + 1;
    t = ch->sample_position - a; /* Cheaper than fmodf(., 1.f) */
  }

  u = xm_sample_at(ch->sample, a);

  switch (ch->sample->loop_type)
  {
    case XM_NO_LOOP:
      if (XM_LINEAR_INTERPOLATION)
        v = (b < ch->sample->length) ? xm_sample_at(ch->sample, b) : .0f;

      ch->sample_position += ch->step;

      if (ch->sample_position >= ch->sample->length)
        ch->sample_position = -1;

      break;

    case XM_FORWARD_LOOP:
      if (XM_LINEAR_INTERPOLATION)
      {
        v = xm_sample_at(ch->sample, (b == ch->sample->loop_end) ? ch->sample->loop_start : b);
      }

      ch->sample_position += ch->step;

      while (ch->sample_position >= ch->sample->loop_end)
        ch->sample_position -= ch->sample->loop_length;

      break;

    case XM_PING_PONG_LOOP:
      if (ch->ping)
        ch->sample_position += ch->step;
      else
        ch->sample_position -= ch->step;

      /* XXX: this may not work for very tight ping-pong loops (ie switches direction more than once per sample) */
      if (ch->ping)
      {
        if (XM_LINEAR_INTERPOLATION)
          v = xm_sample_at(ch->sample, (b >= ch->sample->loop_end) ? a : b);

        if (ch->sample_position >= ch->sample->loop_end)
        {
          ch->ping            = false;
          ch->sample_position = (ch->sample->loop_end << 1) - ch->sample_position;
        }

        /* sanity checking */
        if (ch->sample_position >= ch->sample->length)
        {
          ch->ping             = false;
          ch->sample_position -= ch->sample->length - 1;
        }
      }
      else
      {
        if (XM_LINEAR_INTERPOLATION)
        {
          v = u;
          u = xm_sample_at(ch->sample, (b == 1 || b - 2 <= ch->sample->loop_start) ? a : (b - 2));
        }

        if (ch->sample_position <= ch->sample->loop_start)
        {
          ch->ping            = true;
          ch->sample_position = (ch->sample->loop_start << 1) - ch->sample_position;
        }

        /* sanity checking */
        if (ch->sample_position <= .0f)
        {
          ch->ping            = true;
          ch->sample_position = .0f;
        }
      }
      break;

    default:
      v = .0f;
      break;
  }

  float endval = (XM_LINEAR_INTERPOLATION ? XM_LERP(u, v, t) : u);

#if XM_RAMPING
  if (ch->frame_count < XM_SAMPLE_RAMPING_POINTS)
  {
    /* Smoothly transition between old and new sample. */
    return XM_LERP(ch->end_of_previous_sample[ch->frame_count], endval,
                   (float)ch->frame_count / (float)XM_SAMPLE_RAMPING_POINTS);
  }
#endif

  return endval;
}
size_t IRAM_ATTR xm_pingpong_forward_subrun(float pos, float step, float limit, size_t count)
{
  if (count == 0) return 0;
  if (step <= 0.f) return count;

  size_t run = 0;
  while (run < count)
  {
    run++;
    pos += step;
    if (pos >= limit)
      break;
  }

  return run;
}

size_t IRAM_ATTR xm_pingpong_backward_subrun(float pos, float step, float limit, size_t count)
{
  if (count == 0) return 0;
  if (step <= 0.f) return count;

  size_t run = 0;
  while (run < count)
  {
    run++;
    pos -= step;
    if (pos <= limit)
      break;
  }

  return run;
}

void IRAM_ATTR xm_advance_no_loop(xm_channel_context_t* ch, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;

  for (size_t i = 0; i < count; i++)
  {
    pos += step;
    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_advance_forward_loop(xm_channel_context_t* ch, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;

  for (size_t i = 0; i < count; i++)
  {
    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_advance_pingpong_loop(xm_channel_context_t* ch, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  size_t done = 0;

  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
        pos += step;

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
        pos -= step;

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
}
void IRAM_ATTR xm_advance_gain_ramp_no_loop(xm_channel_context_t* ch, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    pos += step;
    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_advance_gain_ramp_forward_loop(xm_channel_context_t* ch, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_advance_gain_ramp_pingpong_loop(xm_channel_context_t* ch, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;
  size_t done = 0;

  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        pos += step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        pos -= step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_no_loop_8(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = (b < length) ? data[b] / 128.f : .0f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_no_loop_16(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = (b < length) ? data[b] / 32768.f : .0f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_forward_loop_8(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = data[(b == loop_end) ? loop_start : b] / 128.f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_forward_loop_16(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = data[(b == loop_end) ? loop_start : b] / 32768.f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;
  }

  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_pingpong_loop_8(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 128.f;
        float v = data[(b >= loop_end) ? a : b] / 128.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 128.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 128.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_pingpong_loop_16(xm_channel_context_t* ch, float* out, size_t count)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  const float actual_volume = ch->actual_volume;
  const float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 32768.f;
        float v = data[(b >= loop_end) ? a : b] / 32768.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 32768.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 32768.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
}

void IRAM_ATTR xm_render_sample_ramp_no_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = (b < length) ? data[b] / 128.f : .0f;
    float fval = XM_LERP(u, v, t);
    fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                   (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_sample_ramp_no_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = (b < length) ? data[b] / 32768.f : .0f;
    float fval = XM_LERP(u, v, t);
    fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                   (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_sample_ramp_forward_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = data[(b == loop_end) ? loop_start : b] / 128.f;
    float fval = XM_LERP(u, v, t);
    fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                   (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_sample_ramp_forward_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = data[(b == loop_end) ? loop_start : b] / 32768.f;
    float fval = XM_LERP(u, v, t);
    fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                   (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_sample_ramp_pingpong_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 128.f;
        float v = data[(b >= loop_end) ? a : b] / 128.f;
        float fval = XM_LERP(u, v, t);
        fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                       (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 128.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 128.f;
        float fval = XM_LERP(u, v, t);
        fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                       (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_sample_ramp_pingpong_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 32768.f;
        float v = data[(b >= loop_end) ? a : b] / 32768.f;
        float fval = XM_LERP(u, v, t);
        fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                       (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 32768.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 32768.f;
        float fval = XM_LERP(u, v, t);
        fval = XM_LERP(ch->end_of_previous_sample[frame_count], fval,
                       (float)frame_count / (float)XM_SAMPLE_RAMPING_POINTS);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_no_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = (b < length) ? data[b] / 128.f : .0f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_no_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = (b < length) ? data[b] / 32768.f : .0f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);

    if (pos >= length)
    {
      pos = -1;
      break;
    }
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_forward_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 128.f;
    float v = data[(b == loop_end) ? loop_start : b] / 128.f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_forward_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  const uint32_t loop_length = sample->loop_length;
  float pos = ch->sample_position;
  const float step = ch->step;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  for (size_t i = 0; i < count; i++)
  {
    uint32_t a = (uint32_t)pos;
    uint32_t b = a + 1;
    float t = pos - a;
    float u = data[a] / 32768.f;
    float v = data[(b == loop_end) ? loop_start : b] / 32768.f;
    float fval = XM_LERP(u, v, t);

    out[0] += fval * actual_volume * (1.f - actual_panning);
    out[1] += fval * actual_volume * actual_panning * surround_sign;
    out += 2;

    pos += step;
    while (pos >= loop_end)
      pos -= loop_length;

    frame_count++;
    XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
    XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
  }

  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_pingpong_loop_8(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int8_t* data = sample->data8;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 128.f;
        float v = data[(b >= loop_end) ? a : b] / 128.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 128.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 128.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}

void IRAM_ATTR xm_render_gain_ramp_pingpong_loop_16(xm_channel_context_t* ch, float* out, size_t count, float volume_ramp, float panning_ramp)
{
  xm_sample_t* sample = ch->sample;
  const int16_t* data = sample->data16;
  const uint32_t length = sample->length;
  const uint32_t loop_start = sample->loop_start;
  const uint32_t loop_end = sample->loop_end;
  float pos = ch->sample_position;
  const float step = ch->step;
  bool ping = ch->ping;
  float actual_volume = ch->actual_volume;
  float actual_panning = ch->actual_panning;
  const float surround_sign = ch->surround ? -1.f : 1.f;
  const float target_volume = ch->target_volume;
  const float target_panning = ch->target_panning;
  unsigned long frame_count = ch->frame_count;

  size_t done = 0;
  while (done < count)
  {
    if (ping)
    {
      size_t run = xm_pingpong_forward_subrun(pos, step, loop_end, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float u = data[a] / 32768.f;
        float v = data[(b >= loop_end) ? a : b] / 32768.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos += step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos >= loop_end)
      {
        ping = false;
        pos  = (loop_end << 1) - pos;
      }

      if (pos >= length)
      {
        ping = false;
        pos -= length - 1;
      }

      done += run;
    }
    else
    {
      size_t run = xm_pingpong_backward_subrun(pos, step, loop_start, count - done);
      for (size_t i = 0; i < run; i++)
      {
        uint32_t a = (uint32_t)pos;
        uint32_t b = a + 1;
        float t = pos - a;
        float v = data[a] / 32768.f;
        float u = data[(b == 1 || b - 2 <= loop_start) ? a : (b - 2)] / 32768.f;
        float fval = XM_LERP(u, v, t);

        out[0] += fval * actual_volume * (1.f - actual_panning);
        out[1] += fval * actual_volume * actual_panning * surround_sign;
        out += 2;
        pos -= step;
        frame_count++;
        XM_SLIDE_TOWARDS(actual_volume, target_volume, volume_ramp);
        XM_SLIDE_TOWARDS(actual_panning, target_panning, panning_ramp);
      }

      if (pos <= loop_start)
      {
        ping = true;
        pos  = (loop_start << 1) - pos;
      }

      if (pos <= .0f)
      {
        ping = true;
        pos  = .0f;
      }

      done += run;
    }
  }

  ch->ping = ping;
  ch->sample_position = pos;
  ch->frame_count = frame_count;
  ch->actual_volume = actual_volume;
  ch->actual_panning = actual_panning;
}
static void IRAM_ATTR xm_sample_no_tick(xm_context_t* ctx, float* left, float* right)
{
  *left  = 0.f;
  *right = 0.f;

  if (ctx->max_loop_count > 0 && ctx->loop_count >= ctx->max_loop_count)
    return;

  for (uint8_t i = 0; i < ctx->module.num_channels; ++i)
  {
    xm_channel_context_t* ch = ctx->channels + i;

    if (ch->instrument == NULL || ch->sample == NULL || ch->sample_position < 0)
      continue;

    const float fval = xm_next_of_sample(ch);

    if (!ch->muted && !ch->instrument->muted)
    {
      *left  += fval * ch->actual_volume * (1.f - ch->actual_panning);
      *right += fval * ch->actual_volume * ch->actual_panning * (ch->surround ? -1.f : 1.f);
    }

#if XM_RAMPING
    ch->frame_count++;
    XM_SLIDE_TOWARDS(ch->actual_volume, ch->target_volume, ctx->volume_ramp);
    XM_SLIDE_TOWARDS(ch->actual_panning, ch->target_panning, ctx->panning_ramp);
#endif
  }

  const float fgvol = ctx->global_volume * ctx->amplification;
  *left  *= fgvol;
  *right *= fgvol;
}

void IRAM_ATTR xm_sample(xm_context_t* ctx, float* left, float* right)
{
  if (ctx->remaining_samples_in_tick <= 0)
    xm_tick(ctx);

  ctx->remaining_samples_in_tick--;
  xm_sample_no_tick(ctx, left, right);
}

void IRAM_ATTR xm_render(xm_context_t* ctx, float* out, size_t samples)
{
  size_t offset = 0;

  while (offset < samples)
  {
    if (ctx->remaining_samples_in_tick <= 0)
      xm_tick(ctx);

    size_t run = (size_t)ctx->remaining_samples_in_tick;
    if ((float)run < ctx->remaining_samples_in_tick)
      run++;
    if (run == 0)
      run = 1;

    const size_t left = samples - offset;
    if (run > left)
      run = left;

    float* clear_dst = out + 2 * offset;
    for (size_t i = 0; i < run; i++)
    {
      clear_dst[0] = 0.f;
      clear_dst[1] = 0.f;
      clear_dst += 2;
    }

    if (!(ctx->max_loop_count > 0 && ctx->loop_count >= ctx->max_loop_count))
    {
      for (uint8_t i = 0; i < ctx->module.num_channels; ++i)
      {
        xm_channel_context_t* ch = ctx->channels + i;

        if (ch->instrument == NULL || ch->sample == NULL || ch->sample_position < 0)
          continue;

        xm_sample_t* sample = ch->sample;
        if (sample->length == 0)
        {
#if XM_RAMPING
          for (size_t j = 0; j < run; j++)
          {
            ch->frame_count++;
            XM_SLIDE_TOWARDS(ch->actual_volume, ch->target_volume, ctx->volume_ramp);
            XM_SLIDE_TOWARDS(ch->actual_panning, ch->target_panning, ctx->panning_ramp);
          }
#endif
          continue;
        }

        const xm_loop_type_t loop_type = sample->loop_type;
        const uint8_t bits = sample->bits;
        const bool muted = ch->muted || ch->instrument->muted;
        float* dst = out + 2 * offset;

#if XM_RAMPING
        size_t done = 0;

        if (!muted && ch->frame_count < XM_SAMPLE_RAMPING_POINTS)
        {
          size_t ramp_run = XM_SAMPLE_RAMPING_POINTS - ch->frame_count;
          if (ramp_run > run)
            ramp_run = run;

          if (bits == 8)
          {
            switch (loop_type)
            {
              case XM_NO_LOOP:
                xm_render_sample_ramp_no_loop_8(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              case XM_FORWARD_LOOP:
                xm_render_sample_ramp_forward_loop_8(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              case XM_PING_PONG_LOOP:
                xm_render_sample_ramp_pingpong_loop_8(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              default:
                break;
            }
          }
          else
          {
            switch (loop_type)
            {
              case XM_NO_LOOP:
                xm_render_sample_ramp_no_loop_16(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              case XM_FORWARD_LOOP:
                xm_render_sample_ramp_forward_loop_16(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              case XM_PING_PONG_LOOP:
                xm_render_sample_ramp_pingpong_loop_16(ch, dst, ramp_run, ctx->volume_ramp, ctx->panning_ramp);
                break;

              default:
                break;
            }
          }

          done += ramp_run;
          dst += 2 * ramp_run;
          if (ch->sample_position < 0)
            continue;
        }

        size_t rest = run - done;
        if (rest > 0)
        {
          bool gain_ramp = ch->actual_volume != ch->target_volume
                           || ch->actual_panning != ch->target_panning;

          if (muted)
          {
            if (gain_ramp)
            {
              switch (loop_type)
              {
                case XM_NO_LOOP:
                  xm_advance_gain_ramp_no_loop(ch, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_FORWARD_LOOP:
                  xm_advance_gain_ramp_forward_loop(ch, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_PING_PONG_LOOP:
                  xm_advance_gain_ramp_pingpong_loop(ch, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                default:
                  break;
              }
            }
            else
            {
              switch (loop_type)
              {
                case XM_NO_LOOP:
                  xm_advance_no_loop(ch, rest);
                  break;

                case XM_FORWARD_LOOP:
                  xm_advance_forward_loop(ch, rest);
                  break;

                case XM_PING_PONG_LOOP:
                  xm_advance_pingpong_loop(ch, rest);
                  break;

                default:
                  break;
              }

              ch->frame_count += rest;
            }
          }
          else if (gain_ramp)
          {
            if (bits == 8)
            {
              switch (loop_type)
              {
                case XM_NO_LOOP:
                  xm_render_gain_ramp_no_loop_8(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_FORWARD_LOOP:
                  xm_render_gain_ramp_forward_loop_8(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_PING_PONG_LOOP:
                  xm_render_gain_ramp_pingpong_loop_8(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                default:
                  break;
              }
            }
            else
            {
              switch (loop_type)
              {
                case XM_NO_LOOP:
                  xm_render_gain_ramp_no_loop_16(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_FORWARD_LOOP:
                  xm_render_gain_ramp_forward_loop_16(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                case XM_PING_PONG_LOOP:
                  xm_render_gain_ramp_pingpong_loop_16(ch, dst, rest, ctx->volume_ramp, ctx->panning_ramp);
                  break;

                default:
                  break;
              }
            }
          }
          else if (bits == 8)
          {
            switch (loop_type)
            {
              case XM_NO_LOOP:
                xm_render_no_loop_8(ch, dst, rest);
                break;

              case XM_FORWARD_LOOP:
                xm_render_forward_loop_8(ch, dst, rest);
                break;

              case XM_PING_PONG_LOOP:
                xm_render_pingpong_loop_8(ch, dst, rest);
                break;

              default:
                break;
            }

            ch->frame_count += rest;
          }
          else
          {
            switch (loop_type)
            {
              case XM_NO_LOOP:
                xm_render_no_loop_16(ch, dst, rest);
                break;

              case XM_FORWARD_LOOP:
                xm_render_forward_loop_16(ch, dst, rest);
                break;

              case XM_PING_PONG_LOOP:
                xm_render_pingpong_loop_16(ch, dst, rest);
                break;

              default:
                break;
            }

            ch->frame_count += rest;
          }
        }
#else
        if (muted)
        {
          switch (loop_type)
          {
            case XM_NO_LOOP:
              xm_advance_no_loop(ch, run);
              break;

            case XM_FORWARD_LOOP:
              xm_advance_forward_loop(ch, run);
              break;

            case XM_PING_PONG_LOOP:
              xm_advance_pingpong_loop(ch, run);
              break;

            default:
              break;
          }
        }
        else if (bits == 8)
        {
          switch (loop_type)
          {
            case XM_NO_LOOP:
              xm_render_no_loop_8(ch, dst, run);
              break;

            case XM_FORWARD_LOOP:
              xm_render_forward_loop_8(ch, dst, run);
              break;

            case XM_PING_PONG_LOOP:
              xm_render_pingpong_loop_8(ch, dst, run);
              break;

            default:
              break;
          }
        }
        else
        {
          switch (loop_type)
          {
            case XM_NO_LOOP:
              xm_render_no_loop_16(ch, dst, run);
              break;

            case XM_FORWARD_LOOP:
              xm_render_forward_loop_16(ch, dst, run);
              break;

            case XM_PING_PONG_LOOP:
              xm_render_pingpong_loop_16(ch, dst, run);
              break;

            default:
              break;
          }
        }
#endif
      }

      const float fgvol = ctx->global_volume * ctx->amplification;
      float* mix = out + 2 * offset;
      for (size_t i = 0; i < run; i++)
      {
        mix[0] *= fgvol;
        mix[1] *= fgvol;
        mix += 2;
      }
    }

    ctx->remaining_samples_in_tick -= (float)run;
    offset += run;
  }
}

// ============================================================================
// ESP32 tracker task / CLI / stream integration
// ============================================================================




using namespace stats;

QueueHandle_t xm_queue;
QueueHandle_t i2s_queue;
QueueHandle_t player_queue;
QueueHandle_t player_ack_queue;

// Single-slot acknowledgement queue is the stop/play barrier: xm_task may report READY
// only after player_task has switched state and no longer uses the old XM context.
#define MIX_VOLUME_DEFAULT 50
#define MIX_VOLUME_BASE 50

int master_volume = 50000;
int mix_volume = MIX_VOLUME_DEFAULT;
int curr_xm_handle = -1;

#define BCLK_IO        GPIO_NUM_15      // I2S bit clock io number
#define WS_IO          GPIO_NUM_16      // I2S word select io number
#define DOUT_IO        GPIO_NUM_17      // I2S data out io number

#define XM_SAMPLE_RATE        44100
#define XM_FRAME_MS           10
#define XM_SAMPLES_PER_BUFFER (XM_SAMPLE_RATE * XM_FRAME_MS / 1000)
#define XM_BUF_SIZE           (XM_SAMPLES_PER_BUFFER * sizeof(i16) * 2)
#define XM_BUF_NUM            3
#define XM_INFO_PATH_MAX      128

#define XM_STREAM_PATH_MAX 256
#define XM_STREAM_BUFFER_SIZE 512
#define TRACKER_FILE_STREAM_CHUNK_SIZE (64 * 1024)
#define XMZ_INFLATE_DICT_SIZE (32 * 1024)
#define XM_STREAM_ARENA_RESERVE (32 * 1024)

#ifndef TINFL_FLAG_HAS_MORE_INPUT
#define TINFL_FLAG_HAS_MORE_INPUT 0
#endif

EXT_RAM_BSS_ATTR i16 xm_buf[XM_BUF_NUM][XM_BUF_SIZE];
float xm_mix_buf[XM_SAMPLES_PER_BUFFER * 2];  // SRAM for speed
i2s_chan_handle_t tx_chan;
const char XM_TAG[] = "xm";

enum
{
  PLAYER_PLAY,
  PLAYER_STOP
};

typedef struct
{
  u8 valid;
  u16 version;
  u32 header_size;
  u16 song_length;
  u16 restart_position;
  u16 num_channels;
  u16 num_patterns;
  u16 num_instruments;
  u16 flags;
  u16 tempo;
  u16 bpm;
  u32 file_size;
  char path[XM_INFO_PATH_MAX];
  char module_name[21];
  char tracker_name[21];
} XM_INFO;

EXT_RAM_BSS_ATTR XM_INFO xm_info[OBJ_HANDLES_MAX] = {};

typedef enum
{
  TRACK_FILE_FORMAT_XM = 0,
  TRACK_FILE_FORMAT_XMZ,
  TRACK_FILE_FORMAT_MOD,
  TRACK_FILE_FORMAT_S3M
} tracker_file_format_t;

typedef struct XmStreamReader XmStreamReader;
typedef esp_err_t (*XmStreamReadFn)(XmStreamReader *reader, void *dst, size_t size, size_t *out_size);
typedef void (*XmStreamCloseFn)(XmStreamReader *reader);

struct XmStreamReader
{
  FILE *fp;
  size_t file_size;
  size_t pos;
  bool sd_mounted;
  esp_err_t err;
  char opened_path[XM_STREAM_PATH_MAX];
  uint8_t header[60];
  uint32_t module_header_size;
  uint16_t module_flags;
  XmStreamReadFn read;
  XmStreamCloseFn close;
};

typedef struct
{
  XmStreamReader reader;
  u8 *in_buf;
  u8 *dict_buf;
  size_t in_size;
  size_t in_pos;
  size_t dict_pos;
  size_t unpacked_pos;
  size_t unpacked_size;
  tinfl_status status;
  bool done;
} XmzInflateStream;

// Host-stream parser is an inline FSM. It consumes only bytes already present in
// the current DMA chunk and returns NEED_MORE instead of blocking receiver_task.
enum
{
  TRACK_HOST_PARSE_IDLE,
  TRACK_HOST_PARSE_READ_HEADER,
  TRACK_HOST_PARSE_ALLOC_ARENA,
  TRACK_HOST_PARSE_APPLY_XM_HEADER,
  TRACK_HOST_PARSE_READ_MODULE_HEADER,
  TRACK_HOST_PARSE_APPLY_MODULE_HEADER,
  TRACK_HOST_PARSE_READ_PATTERN_HEADER,
  TRACK_HOST_PARSE_LOAD_PATTERN_DATA,
  TRACK_HOST_PARSE_NEXT_PATTERN,
  TRACK_HOST_PARSE_READ_INSTRUMENT_HEADER,
  TRACK_HOST_PARSE_APPLY_INSTRUMENT_HEADER,
  TRACK_HOST_PARSE_READ_SAMPLE_HEADER,
  TRACK_HOST_PARSE_LOAD_SAMPLE_DATA,
  TRACK_HOST_PARSE_NEXT_INSTRUMENT,
  TRACK_HOST_PARSE_ALLOC_RUNTIME,
  TRACK_HOST_PARSE_POSTLOAD,
  TRACK_HOST_PARSE_SHRINK,
  TRACK_HOST_PARSE_DRAIN_TRAILING,
  TRACK_HOST_PARSE_DONE,
  TRACK_HOST_PARSE_MOD_READ_HEADER,
  TRACK_HOST_PARSE_MOD_ALLOC_CONTEXT,
  TRACK_HOST_PARSE_MOD_LOAD_PATTERN_DATA,
  TRACK_HOST_PARSE_MOD_LOAD_SAMPLE_DATA,
  TRACK_HOST_PARSE_MOD_ALLOC_RUNTIME,
  TRACK_HOST_PARSE_MOD_DONE,
  TRACK_HOST_PARSE_S3M_READ_HEADER,
  TRACK_HOST_PARSE_S3M_READ_ORDERS,
  TRACK_HOST_PARSE_S3M_READ_SAMPLE_PTRS,
  TRACK_HOST_PARSE_S3M_READ_PATTERN_PTRS,
  TRACK_HOST_PARSE_S3M_READ_PANNING,
  TRACK_HOST_PARSE_S3M_READ_SAMPLE_HEADER,
  TRACK_HOST_PARSE_S3M_ALLOC_CONTEXT,
  TRACK_HOST_PARSE_S3M_LOAD_PATTERN_DATA,
  TRACK_HOST_PARSE_S3M_LOAD_SAMPLE_DATA,
  TRACK_HOST_PARSE_S3M_ALLOC_RUNTIME,
  TRACK_HOST_PARSE_S3M_DONE,
  TRACK_HOST_PARSE_ERROR
};

enum
{
  XM_HOST_PATTERN_READ_NOTE,
  XM_HOST_PATTERN_READ_COMPRESSED_FIELD,
  XM_HOST_PATTERN_READ_UNCOMPRESSED_FIELD
};

enum
{
  TRACK_HOST_STEP_ERROR = -1,
  TRACK_HOST_STEP_OK = 0,
  TRACK_HOST_STEP_NEED_MORE = 1,
  TRACK_HOST_STEP_NO_MORE = 2
};

enum
{
  TRACK_HOST_BUILD_NEED_MORE = -5
};

enum
{
  TRACK_HOST_STREAM_FORMAT_XM,
  TRACK_HOST_STREAM_FORMAT_MOD,
  TRACK_HOST_STREAM_FORMAT_S3M
};

typedef struct
{
  // All continuation state that previously lived on the xm-stream task stack.
  int stage;
  xm_context_t *ctx;
  char *arena;
  size_t arena_size;
  char *mempool;
  char *mempool_end;
  xm_context_group_sizes_t group_sizes;
  xm_context_group_cursor_t cursor;
  size_t used_size;
  size_t instrument_sample_bytes;
  char *instrument_sample_data;
  char *instrument_sample_data_pos;
  char *instrument_sample_data_end;
  size_t trailing_size;
  size_t trailing_done;
  // Generic progress counter for resumable header/record reads.
  size_t read_done;

  xm_module_t *mod;
  xm_pattern_t *pat;
  xm_instrument_t *instr;
  xm_sample_t *sample;

  uint32_t module_header_size;
  uint32_t pattern_header_size;
  uint32_t instrument_header_size;
  uint32_t sample_header_size;
  size_t packed_pattern_size;
  size_t packed_pattern_soft_size;
  uint16_t pattern_i;
  uint16_t instrument_i;
  uint16_t sample_i;
  uint16_t sample_data_i;
  size_t pattern_packed_pos;
  size_t pattern_event_pos;
  uint32_t pattern_slot_pos;
  // Pattern packets are bitmask-compressed. These fields preserve a partially
  // decoded slot when a DMA chunk ends between optional fields.
  size_t pattern_read_done;
  uint8_t pattern_state;
  uint8_t pattern_note;
  uint8_t pattern_field_idx;
  uint8_t pattern_field_value;
  // XM sample data is delta-encoded. 16-bit samples may split low/high bytes
  // across DMA chunks, so the partial low byte is stored here.
  size_t sample_pos;
  int8_t sample_prev8;
  int16_t sample_prev16;
  uint8_t sample_data_started;
  uint8_t sample_16_have_low;
  uint8_t sample_16_low;

  uint8_t *mod_header;
  mod_layout_t mod_layout;
  size_t mod_header_length;
  size_t mod_preload_pos;
  size_t mod_preload_size;
  uint8_t mod_entry[4];

  s3m_layout_t *s3m_layout;
  size_t s3m_table_offset;
  size_t s3m_sample_bytes;
  uint16_t s3m_pattern_row;
  uint8_t s3m_info;
  uint8_t s3m_note;
  uint8_t s3m_instr;
  uint8_t s3m_volume;
  uint8_t s3m_command;
  uint8_t s3m_param;

  uint8_t header[60];
  uint8_t module_header[276];
  uint8_t pattern_header[9];
  uint8_t instrument_header[243];
  uint8_t sample_header[40];
  uint8_t s3m_header[S3M_HEADER_SIZE];
  uint8_t s3m_sample_header[S3M_SAMPLE_HEADER_SIZE];
  uint8_t s3m_panning[S3M_MAX_CHANNELS];
} XmHostStreamParser;

typedef struct
{
  bool active;
  bool chunk_active;
  bool waiting_rx;
  bool need_chunk;
  size_t total_size;
  size_t pos;
  size_t requested_size;
  size_t logical_size;
  const u8 *chunk_data;
  size_t chunk_size;
  size_t chunk_pos;
  int handle;
  int format;
  esp_err_t err;
  XmHostStreamParser parser;
} XmHostStreamState;

typedef struct
{
  size_t offset;
  size_t rx_size;
  size_t logical_size;
} TrackerStreamChunkRequest;

typedef struct
{
  xm_context_t *ctx;
  size_t used_size;
  int obj_type;
  int build_rc;
} TrackerStreamPushResult;

EXT_RAM_BSS_ATTR XmHostStreamState g_xm_host_stream = {};

// -------------------- System prototypes --------------------
void initialize_xm();

// -------------------- Slave task prototypes --------------------
void i2s_task(void *arg);
void player_task(void *arg);
void xm_task(void *arg);

// -------------------- Slave helper prototypes --------------------
void xm_host_stream_clear_state();
void xm_host_stream_release_chunk();
void xm_host_stream_abort_current();
int xm_host_stream_take(void *dst, size_t size, size_t *done);
int xm_host_mod_stream_take(void *dst, size_t size, size_t *done);
int xm_host_stream_skip_take(size_t size, size_t *done);
void xm_host_stream_free_parser_buffers();
int xm_host_stream_read_record_take(uint8_t *dst, size_t dst_size, uint32_t rec_size, size_t *done);
void *xm_host_stream_alloc_block(size_t size, uint8_t type, bool clear);
void *xm_host_mempool_alloc(char **mempool, char *mempool_end, size_t size, bool clear);
int xm_host_load_pattern_stream(xm_pattern_t *pat, uint16_t packed_size);
int xm_host_load_sample_data_stream(xm_sample_t *sample);
void xm_host_stream_set_info(int handle, xm_context_t *ctx, size_t file_size);
int xm_host_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size);
int xm_host_mod_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size);
int xm_host_s3m_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size);
void xm_host_mod_stream_set_info(int handle, xm_context_t *ctx, size_t file_size);
int tracker_stream_format_is_valid(int format);
int tracker_stream_obj_type(int format);
int tracker_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size);
void tracker_stream_free_pending_context();
esp_err_t tracker_stream_begin(size_t module_size, int format);
esp_err_t tracker_stream_request_chunk(size_t rx_size, TrackerStreamChunkRequest *request);
int tracker_stream_push_chunk(const u8 *data, size_t size, TrackerStreamPushResult *result);
void tracker_stream_finish_success();
int tracker_file_stream_format(tracker_file_format_t format);
void tracker_stream_set_context_info(int handle, int obj_type, const char *path, size_t file_size, xm_context_t *ctx);
int tracker_stream_load_file_to_handle(const char *path, size_t size, tracker_file_format_t format, int *out_handle, bool quiet);
esp_err_t xm_host_stream_prepare_command_format(size_t module_size, size_t rx_size, int format);
esp_err_t xm_host_stream_prepare_command(size_t module_size, size_t rx_size);
esp_err_t mod_host_stream_prepare_command(size_t module_size, size_t rx_size);
esp_err_t s3m_host_stream_prepare_command(size_t module_size, size_t rx_size);
void xm_host_stream_process_rx_data(const u8 *data, size_t size);

// -------------------- XM helper prototypes --------------------
void xm_reset_render_stats();
void xm_reset_context_state(xm_context_t *ctx, int handle);
void xm_stop_player_sync(int handle);
void *xm_malloc(size_t size);
u16 xm_rd_le16(const u8 *p);
u32 xm_rd_le32(const u8 *p);
void xm_copy_trimmed(char *dst, size_t dst_size, const u8 *src, size_t src_size);
void xm_clear_info(int handle);
int xm_parse_info(int handle, const char *path, const u8 *data, size_t size);
const char *xm_obj_state_str(u8 st);
const char *xm_obj_type_str(u8 type);
int xm_find_playing_handle();
int xm_wait_for_state(int handle, u8 state, int timeout_ms);
int xm_wait_for_status_ready(int timeout_ms);
int xm_wait_for_init(int handle, int timeout_ms);
int xm_stop_current_playback(bool quiet = false);
int xm_parse_handle_arg(const char *s, int *out_handle);
int xm_delete_all_modules(bool quiet);
int xm_init_handle(int handle, bool quiet);

// -------------------- XM stream parser prototypes --------------------
esp_err_t xm_stream_file_read(XmStreamReader *reader, void *dst, size_t size, size_t *out_size);
void xm_stream_file_close(XmStreamReader *reader);
esp_err_t xm_stream_reader_open(XmStreamReader *reader, const char *path, size_t file_size);
esp_err_t xm_stream_reader_read_exact(XmStreamReader *reader, void *dst, size_t size);
esp_err_t xm_stream_reader_seek(XmStreamReader *reader, size_t offset);
void xm_set_mod_context_info(int handle, const char *path, size_t file_size, xm_context_t *ctx);
void xm_set_s3m_context_info(int handle, const char *path, size_t file_size, xm_context_t *ctx);

// -------------------- Console helper prototypes --------------------
bool xm_path_is_sd_abs(const char *path);
bool xm_copy_path_with_ext_case(const char *path, char *out, size_t out_size, bool upper);
esp_err_t xm_stat_file_size(const char *path, size_t *out_size);
esp_err_t xm_stat_file_size_any_ext_case(const char *path, size_t *out_size);
FILE *xm_fopen_any_ext_case(const char *path, const char *mode, char *opened_path, size_t opened_path_size);
esp_err_t xm_get_file_size(const char *path, size_t *out_size);
esp_err_t xm_read_file_data(const char *path, void *dst, size_t size);
const char *xm_file_format_str(tracker_file_format_t format);
void xmz_inflate_stream_close(XmzInflateStream *stream);
int xmz_inflate_stream_open(XmzInflateStream *stream, const char *path, size_t packed_size, size_t *out_unpacked_size, tracker_file_format_t format, bool quiet);
int xmz_inflate_stream_read_input(XmzInflateStream *stream, tracker_file_format_t format, bool quiet);
int xmz_inflate_stream_fill(XmzInflateStream *stream, u8 *dst, size_t size, tracker_file_format_t format, bool quiet);
int xmz_inflate_stream_finish(XmzInflateStream *stream, tracker_file_format_t format, bool quiet);

int xm_load_compressed_file_to_handle(const char *path, size_t size, tracker_file_format_t format, int *out_handle, bool quiet);
int xm_load_file_to_handle(const char *path, int *out_handle, bool quiet);
int xm_load_file(const char *path);
int xm_load_play_file(const char *path, bool quiet);

// -------------------- Console prototypes --------------------
int xm_list_cmd();
int xm_info_cmd(int argc, char **argv);
int xm_play_cmd(int handle, bool quiet);
int xm_stop_cmd(bool quiet);
int xm_reset_cmd(int handle, bool quiet);
int xm_del_cmd(int handle);
int xm_vol_cmd(int argc, char **argv);
int xm_cmd(int argc, char **argv);
void xm_console_register_system_commands();

// -------------------- System --------------------

void initialize_xm()
{
  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));
  log_sram_used(__FILE_NAME__ ": i2s_new_channel");

  i2s_std_config_t tx_std_cfg =
  {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(XM_SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg =
    {
      .bclk         = BCLK_IO,
      .ws           = WS_IO,
      .dout         = DOUT_IO,
      .invert_flags =
      {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv   = false,
      },
    },
  };
  log_sram_used(__FILE_NAME__ ": tx_std_cfg");

  tx_std_cfg.slot_cfg.bit_shift = true;   // Phillips format support

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
  log_sram_used(__FILE_NAME__ ": i2s_channel_init_std_mode");
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
  log_sram_used(__FILE_NAME__ ": i2s_channel_enable");

  sfx_init();

  xm_queue = xQueueCreate(XM_BUF_NUM + 1, sizeof(TRACK_TASK));
  i2s_queue = xQueueCreateWithCaps(XM_BUF_NUM - 2, sizeof(int), task_ram_type_non_critical);
  player_queue = xQueueCreateWithCaps(2, sizeof(PLAYER_TASK), task_ram_type_non_critical);
  player_ack_queue = xQueueCreateWithCaps(1, sizeof(int), task_ram_type_non_critical);

  xTaskCreatePinnedToCoreWithCaps(xm_task, "tracker-helper", 3072, NULL, TRACKER_HELPER_TASK_PRIO, NULL, 0, task_ram_type_non_critical);     // Tracker helper task
  log_sram_used(__FILE_NAME__ ": TaskCreate xm_task");
  xTaskCreatePinnedToCoreWithCaps(i2s_task, "i2s-writer", 2048, NULL, I2S_TASK_PRIO, NULL, 1, task_ram_type_critical);         // I2S DAC writer
  log_sram_used(__FILE_NAME__ ": TaskCreate i2s_task");
  xTaskCreatePinnedToCoreWithCaps(player_task, "tracker-player", 2048, NULL, TRACKER_PLAYER_TASK_PRIO, NULL, 1, task_ram_type_non_critical);    // Tracker renderer (should work on a separate core)
  log_sram_used(__FILE_NAME__ ": TaskCreate player_task");
}

// -------------------- Slave tasks --------------------

void i2s_task(void *arg)
{
  int idx;

  while (1)
  {
    xQueueReceive(i2s_queue, &idx, portMAX_DELAY);

    size_t w_bytes = 0;
    i2s_channel_write(tx_chan, xm_buf[idx], XM_BUF_SIZE, &w_bytes, 1000);
  }
}

void player_task(void *arg)
{
  PLAYER_TASK t;
  t.task = PLAYER_STOP;
  int xm_buf_idx = 0;

  while (1)
  {
    BaseType_t got_task = xQueueReceive(player_queue, &t, 0);
    bool xm_active = t.task == PLAYER_PLAY;
    int t_us = 0;

    if (xm_active && got_task == pdTRUE)
    {
      // Acknowledge PLAY before rendering: from this point the new context is owned
      // by player_task and xm_task can mark the object as PLAYING.
      int ack = 0;
      xQueueOverwrite(player_ack_queue, &ack);
    }

    int tracker_us = 0;
    int opl_us = 0;
    int sfx_us = 0;

    auto t1 = esp_timer_get_time();

    if (xm_active)
    {
      xm_render(t.ctx, xm_mix_buf, XM_SAMPLES_PER_BUFFER);

      for (int i = 0; i < XM_SAMPLES_PER_BUFFER; i++)
      {
        float left = xm_mix_buf[2 * i];
        float right = xm_mix_buf[2 * i + 1];

        _st.xm_samp_min = min(_st.xm_samp_min, left);
        _st.xm_samp_min = min(_st.xm_samp_min, right);
        _st.xm_samp_max = max(_st.xm_samp_max, left);
        _st.xm_samp_max = max(_st.xm_samp_max, right);

        xm_mix_buf[2 * i] = left * master_volume;
        xm_mix_buf[2 * i + 1] = right * master_volume;
      }

      tracker_us = (int)(esp_timer_get_time() - t1);
    }
    else
    {
      memset(xm_mix_buf, 0, sizeof(xm_mix_buf));
    }

    auto t_opl = esp_timer_get_time();
    opl_render(xm_mix_buf, XM_SAMPLES_PER_BUFFER, XM_SAMPLE_RATE);
    opl_us = (int)(esp_timer_get_time() - t_opl);

    auto t_sfx = esp_timer_get_time();
    sfx_render(xm_mix_buf, XM_SAMPLES_PER_BUFFER, XM_SAMPLE_RATE);
    sfx_us = (int)(esp_timer_get_time() - t_sfx);

    for (int i = 0; i < XM_SAMPLES_PER_BUFFER; i++)
    {
      int l = (int)(xm_mix_buf[2 * i] * mix_volume / MIX_VOLUME_BASE);
      l = max(l, -32768);
      l = min(l, 32767);
      xm_buf[xm_buf_idx][2 * i] = l;

      int r = (int)(xm_mix_buf[2 * i + 1] * mix_volume / MIX_VOLUME_BASE);
      r = max(r, -32768);
      r = min(r, 32767);
      xm_buf[xm_buf_idx][2 * i + 1] = r;
    }

    stats::update_audio_render(
      &_st.audio_tracker_last_us, &_st.audio_tracker_min_us, &_st.audio_tracker_max_us,
      &_st.audio_tracker_last_cpu_x10, &_st.audio_tracker_min_cpu_x10, &_st.audio_tracker_max_cpu_x10,
      tracker_us, XM_SAMPLE_RATE, XM_SAMPLES_PER_BUFFER);

    stats::update_audio_render(
      &_st.audio_opl_last_us, &_st.audio_opl_min_us, &_st.audio_opl_max_us,
      &_st.audio_opl_last_cpu_x10, &_st.audio_opl_min_cpu_x10, &_st.audio_opl_max_cpu_x10,
      opl_us, XM_SAMPLE_RATE, XM_SAMPLES_PER_BUFFER);

    stats::update_audio_render(
      &_st.audio_sfx_last_us, &_st.audio_sfx_min_us, &_st.audio_sfx_max_us,
      &_st.audio_sfx_last_cpu_x10, &_st.audio_sfx_min_cpu_x10, &_st.audio_sfx_max_cpu_x10,
      sfx_us, XM_SAMPLE_RATE, XM_SAMPLES_PER_BUFFER);

    t_us = tracker_us + opl_us + sfx_us;
    stats::update_audio_render(
      &_st.audio_total_last_us, &_st.audio_total_min_us, &_st.audio_total_max_us,
      &_st.audio_total_last_cpu_x10, &_st.audio_total_min_cpu_x10, &_st.audio_total_max_cpu_x10,
      t_us, XM_SAMPLE_RATE, XM_SAMPLES_PER_BUFFER);

    xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
    xm_buf_idx++;
    xm_buf_idx %= XM_BUF_NUM;

    if (!xm_active && got_task == pdTRUE)
    {
      // Acknowledge STOP after the mixed buffer is queued. After this ack,
      // player_task will not call xm_sample() on the previous context anymore.
      int ack = 1;
      xQueueOverwrite(player_ack_queue, &ack);
    }
  }
}

void xm_task(void *arg)
{
  TRACK_TASK task;

  while (1)
  {
    xQueueReceive(xm_queue, &task, portMAX_DELAY);

    switch (task.task)
    {
      // Initialize tracker module. Format-specific builder is selected by raw object type.
      case TRACK_TASK_INIT:
      {
        if (!check_handle(task.handle) || (mem_obj[task.handle].type != OBJ_TYPE_XM && mem_obj[task.handle].type != OBJ_TYPE_MOD && mem_obj[task.handle].type != OBJ_TYPE_S3M))
        {
          ESP_LOGE("xm_task: TRACK_INIT", "Invalid handle: handle=%d\r\n", task.handle);
          set_status(ESP_ERR_INV_HANDLE);
          break;
        }

        MEM_OBJ *obj = &mem_obj[task.handle];
        int raw_type = obj->type;
        size_t raw_size = obj->size;
        xm_context_s *ctx = NULL;
        int rc = -1;
        size_t bytes_needed = 0;

        if (raw_type == OBJ_TYPE_XM)
        {
          if (!xm_info[task.handle].valid)
            xm_parse_info(task.handle, NULL, (const u8*)obj->addr, obj->size);

          rc = xm_create_context_safe(&ctx, obj->addr, obj->size, XM_SAMPLE_RATE, xm_malloc);

          if (rc == -1)
          {
            ESP_LOGE("xm_task", "XM module error!");
            set_status(ESP_ERR_INV_XM);
            break;
          }

          else if (rc == -2)
          {
            ESP_LOGE("xm_task", "XM context memory allocation error!");
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

#ifdef VERBOSE
          printf("XM context created\r\n");
#endif

          free(obj->addr);
          obj->type = OBJ_TYPE_XMC;
          obj->addr = (void*)ctx;
          obj->size = rc;

#ifdef VERBOSE
          printf("XM init success\r\n");
#endif
        }

        else if (raw_type == OBJ_TYPE_MOD)
        {
          rc = mod_create_context_safe(&ctx, obj->addr, obj->size, XM_SAMPLE_RATE, xm_malloc, &bytes_needed);

          if (rc == -1)
          {
            ESP_LOGE("xm_task", "MOD module error!");
            set_status(ESP_ERR_INV_MOD);
            break;
          }

          else if (rc == -2)
          {
            ESP_LOGE("xm_task", "MOD context memory allocation error: need=%u", (unsigned)bytes_needed);
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          free(obj->addr);
          obj->type = OBJ_TYPE_MDC;
          obj->addr = (void*)ctx;
          obj->size = rc;

          xm_clear_info(task.handle);
          XM_INFO *info = &xm_info[task.handle];
          info->valid = 1;
          info->header_size = 1084;
          info->song_length = ctx->module.length;
          info->restart_position = ctx->module.restart_position;
          info->num_channels = ctx->module.num_channels;
          info->num_patterns = ctx->module.num_patterns;
          info->num_instruments = ctx->module.num_instruments;
          info->tempo = ctx->tempo;
          info->bpm = ctx->bpm;
          info->file_size = (u32)raw_size;
#if XM_STRINGS
          xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
          xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif

#ifdef VERBOSE
          printf("MOD init success\r\n");
#endif
        }

        else
        {
          rc = s3m_create_context_safe(&ctx, obj->addr, obj->size, XM_SAMPLE_RATE, xm_malloc, &bytes_needed);

          if (rc == -1)
          {
            ESP_LOGE("xm_task", "%s", s3m_get_last_error());
            set_status(ESP_ERR_INV_S3M);
            break;
          }

          else if (rc == -2)
          {
            ESP_LOGE("xm_task", "S3M context memory allocation error: need=%u", (unsigned)bytes_needed);
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          else if (rc < 0)
          {
            ESP_LOGE("xm_task", "%s", s3m_get_last_error());
            set_status(ESP_ERR_INV_S3M);
            break;
          }

          free(obj->addr);
          obj->type = OBJ_TYPE_S3C;
          obj->addr = (void*)ctx;
          obj->size = rc;

          xm_set_s3m_context_info(task.handle, NULL, raw_size, ctx);
        }

        xm_reset_context_state((xm_context_t*)obj->addr, task.handle);
        obj->state = TRACK_OBJ_ST_STOPPED;
        xm_reset_render_stats();
        set_status(ESP_ST_READY);
      }
      break;

      case TRACK_TASK_PLAY:
      {
        // ESP_LOGW("xm_task", "PLAY");
        if (!check_handle(task.handle) || (mem_obj[task.handle].type != OBJ_TYPE_XMC && mem_obj[task.handle].type != OBJ_TYPE_MDC && mem_obj[task.handle].type != OBJ_TYPE_S3C))
        {
          ESP_LOGE("xm_task: PLAY", "Invalid handle: handle=%d\r\n", task.handle);
          set_status(ESP_ERR_INV_XM_HANDLE);
          break;
        }

        MEM_OBJ *obj = &mem_obj[task.handle];

        if (!obj->addr || (obj->state != TRACK_OBJ_ST_STOPPED))
        {
          ESP_LOGE("xm_task: PLAY", "Invalid state: handle=%02X obj_state=%02X(%s)\r\n",
            task.handle,
            obj->state,
            xm_obj_state_str(obj->state));
          set_status(ESP_ERR_INV_STATE);
        }
        else
        {
          PLAYER_TASK t = {};
          t.task = PLAYER_PLAY;
          t.ctx = (xm_context_s*)obj->addr;
          int ack;

          esp_err_t opl_err = s3m_adlib_prepare_opl(t.ctx);
          if (opl_err != ESP_OK)
          {
            ESP_LOGE("xm_task: PLAY", "S3M AdLib OPL prepare failed: %s", esp_err_to_name(opl_err));
            set_status(opl_err);
            break;
          }

          while (xQueueReceive(player_ack_queue, &ack, 0) == pdTRUE);
          xQueueSend(player_queue, &t, portMAX_DELAY);
          xQueueReceive(player_ack_queue, &ack, portMAX_DELAY);

          curr_xm_handle = task.handle;
          obj->state = TRACK_OBJ_ST_PLAYING;
          set_status(ESP_ST_READY);
        }
      }
      break;

      case TRACK_TASK_STOP:
      {
        // ESP_LOGW("xm_task", "STOP");
        xm_stop_player_sync(task.handle);
        set_status(ESP_ST_READY);
      }
      break;

      case TRACK_TASK_RESET:
      {
        // ESP_LOGW("xm_task", "RESET");
        if (!check_handle(task.handle) || (mem_obj[task.handle].type != OBJ_TYPE_XMC && mem_obj[task.handle].type != OBJ_TYPE_MDC && mem_obj[task.handle].type != OBJ_TYPE_S3C))
        {
          ESP_LOGE("xm_task: RESET", "Invalid handle: handle=%d\r\n", task.handle);
          set_status(ESP_ERR_INV_XM_HANDLE);
          break;
        }

        MEM_OBJ *obj = &mem_obj[task.handle];

        if (!obj->addr || (obj->type != OBJ_TYPE_XMC && obj->type != OBJ_TYPE_MDC && obj->type != OBJ_TYPE_S3C))
        {
          ESP_LOGE("xm_task: RESET", "Invalid obj type: type=%d\r\n", obj->type);
          set_status(ESP_ERR_INV_XM_HANDLE);
          break;
        }

        if (obj->state == TRACK_OBJ_ST_PLAYING)
          xm_stop_player_sync(task.handle);

        else if (obj->state != TRACK_OBJ_ST_STOPPED)
        {
          ESP_LOGE("xm_task: RESET", "Invalid state: handle=%02X obj_state=%02X(%s)\r\n",
            task.handle,
            obj->state,
            xm_obj_state_str(obj->state));
          set_status(ESP_ERR_INV_STATE);
          break;
        }

        xm_reset_context_state((xm_context_t*)obj->addr, task.handle);
        obj->state = TRACK_OBJ_ST_STOPPED;
        if (curr_xm_handle == task.handle)
          curr_xm_handle = -1;
        xm_reset_render_stats();
        set_status(ESP_ST_READY);
      }
      break;
    }
  }
}

// -------------------- Slave helpers --------------------

void xm_host_stream_free_parser_buffers()
{
  if (g_xm_host_stream.parser.mod_header)
  {
    heap_caps_free(g_xm_host_stream.parser.mod_header);
    g_xm_host_stream.parser.mod_header = NULL;
  }


  if (g_xm_host_stream.parser.s3m_layout)
  {
    heap_caps_free(g_xm_host_stream.parser.s3m_layout);
    g_xm_host_stream.parser.s3m_layout = NULL;
  }
}

void xm_host_stream_clear_state()
{
  xm_host_stream_free_parser_buffers();

  memset(&g_xm_host_stream, 0, sizeof(g_xm_host_stream));
  g_xm_host_stream.handle = -1;
  g_xm_host_stream.format = TRACK_HOST_STREAM_FORMAT_XM;
  g_xm_host_stream.err = ESP_OK;
}

void xm_host_stream_release_chunk()
{
  if (!g_xm_host_stream.chunk_active) return;

  g_xm_host_stream.chunk_active = false;
  g_xm_host_stream.chunk_data = NULL;
  g_xm_host_stream.chunk_size = 0;
  g_xm_host_stream.chunk_pos = 0;
}

void xm_host_stream_abort_current()
{
  if (g_xm_host_stream.active && g_xm_host_stream.parser.ctx)
  {
    xm_free_context(g_xm_host_stream.parser.ctx);
    g_xm_host_stream.parser.ctx = NULL;
    g_xm_host_stream.parser.arena = NULL;
  }

  xm_host_stream_release_chunk();
  xm_host_stream_clear_state();
}

// Copy bytes from the current DMA chunk only. No waiting here: chunk exhaustion
// returns NEED_MORE so the SPI side can request the next host chunk deterministically.
int xm_host_stream_take(void *dst, size_t size, size_t *done)
{
  uint8_t *out = (uint8_t*)dst;

  if (!done || (!dst && size))
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (*done > size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return TRACK_HOST_STEP_ERROR;
  }

  while (*done < size)
  {
    if (!g_xm_host_stream.chunk_active || g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
    {
      xm_host_stream_release_chunk();
      return TRACK_HOST_STEP_NEED_MORE;
    }

    size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
    size_t todo = size - *done;
    if (todo > avail) todo = avail;

    if (todo)
      memcpy(out + *done, g_xm_host_stream.chunk_data + g_xm_host_stream.chunk_pos, todo);

    g_xm_host_stream.chunk_pos += todo;
    g_xm_host_stream.pos += todo;
    *done += todo;

    if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      xm_host_stream_release_chunk();
  }

  return TRACK_HOST_STEP_OK;
}

int xm_host_mod_stream_take(void *dst, size_t size, size_t *done)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;
  uint8_t *out = (uint8_t*)dst;

  if (!done || (!dst && size))
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (*done > size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return TRACK_HOST_STEP_ERROR;
  }

  while (*done < size && parser->mod_preload_pos < parser->mod_preload_size)
  {
    size_t avail = parser->mod_preload_size - parser->mod_preload_pos;
    size_t todo = size - *done;
    if (todo > avail) todo = avail;

    if (todo)
      memcpy(out + *done, parser->mod_header + parser->mod_layout.header_size + parser->mod_preload_pos, todo);

    parser->mod_preload_pos += todo;
    *done += todo;
  }

  if (*done >= size) return TRACK_HOST_STEP_OK;
  return xm_host_stream_take(dst, size, done);
}

int xm_host_stream_skip_take(size_t size, size_t *done)
{
  if (!done)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (*done > size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return TRACK_HOST_STEP_ERROR;
  }

  while (*done < size)
  {
    if (!g_xm_host_stream.chunk_active || g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
    {
      xm_host_stream_release_chunk();
      return TRACK_HOST_STEP_NEED_MORE;
    }

    size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
    size_t todo = size - *done;
    if (todo > avail) todo = avail;

    g_xm_host_stream.chunk_pos += todo;
    g_xm_host_stream.pos += todo;
    *done += todo;

    if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      xm_host_stream_release_chunk();
  }

  return TRACK_HOST_STEP_OK;
}

// XM records may be larger than the fixed local header buffers. Keep the bounded
// prefix needed by libxm-compatible parsing and skip the remaining bytes resumably.
int xm_host_stream_read_record_take(uint8_t *dst, size_t dst_size, uint32_t rec_size, size_t *done)
{
  size_t copy_size;

  if (!done || (!dst && dst_size))
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (*done > rec_size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return TRACK_HOST_STEP_ERROR;
  }

  copy_size = rec_size;
  if (copy_size > dst_size)
    copy_size = dst_size;

  if (*done < copy_size)
  {
    int step = xm_host_stream_take(dst, copy_size, done);
    if (step != TRACK_HOST_STEP_OK) return step;
  }

  if (*done < rec_size)
    return xm_host_stream_skip_take(rec_size, done);

  return TRACK_HOST_STEP_OK;
}

void *xm_host_stream_alloc_block(size_t size, uint8_t type, bool clear)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;
  void *ptr;

  if (!parser->ctx || !size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return NULL;
  }

  if (SIZE_MAX - parser->used_size < size || parser->used_size + size > INT_MAX)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return NULL;
  }

  ptr = xm_malloc(size);
  if (!ptr)
  {
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  if (clear) memset(ptr, 0, size);

  if (!tracker_context_register_segment(parser->ctx, ptr, size, type))
  {
    free(ptr);
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  parser->used_size += size;
  parser->ctx->ctx_size = parser->used_size;
  return ptr;
}

void *xm_host_mempool_alloc(char **mempool, char *mempool_end, size_t size, bool clear)
{
  void *ptr;

  if (!mempool || !*mempool || !mempool_end) return NULL;
  if (*mempool > mempool_end)
  {
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  if (size > (size_t)(mempool_end - *mempool))
  {
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  ptr = *mempool;
  *mempool += size;
  if (clear && size) memset(ptr, 0, size);
  return ptr;
}

// Decode one packed pattern as a resumable state machine. A compressed slot may
// end at any field boundary, including exactly at the end of a DMA chunk.
int xm_host_load_pattern_stream(xm_pattern_t *pat, uint16_t packed_size)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;

  if (!pat || !pat->slots)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  while (parser->pattern_packed_pos < packed_size)
  {
    uint32_t slot_count = (uint32_t)parser->mod->num_channels * pat->num_rows;
    if (parser->pattern_slot_pos >= slot_count)
    {
      g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
      return TRACK_HOST_STEP_ERROR;
    }

    xm_pattern_slot_t *slot = pat->slots + parser->pattern_slot_pos;

    switch (parser->pattern_state)
    {
      case XM_HOST_PATTERN_READ_NOTE:
      {
        int step = xm_host_stream_take(&parser->pattern_note, sizeof(parser->pattern_note), &parser->pattern_read_done);
        if (step != TRACK_HOST_STEP_OK) return step;

        parser->pattern_read_done = 0;
        parser->pattern_packed_pos++;

        if (parser->pattern_note & (1 << 7))
        {
          parser->pattern_field_idx = 0;
          parser->pattern_state = XM_HOST_PATTERN_READ_COMPRESSED_FIELD;
        }
        else
        {
          slot->note = parser->pattern_note;
          parser->pattern_field_idx = 0;
          parser->pattern_state = XM_HOST_PATTERN_READ_UNCOMPRESSED_FIELD;
        }
      }
      break;

      case XM_HOST_PATTERN_READ_COMPRESSED_FIELD:
      {
        while (parser->pattern_field_idx < 5)
        {
          uint8_t mask = 1 << parser->pattern_field_idx;
          if (!(parser->pattern_note & mask))
          {
            parser->pattern_field_idx++;
            continue;
          }

          if (parser->pattern_packed_pos >= packed_size)
          {
            g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
            return TRACK_HOST_STEP_ERROR;
          }

          int step = xm_host_stream_take(&parser->pattern_field_value, sizeof(parser->pattern_field_value), &parser->pattern_read_done);
          if (step != TRACK_HOST_STEP_OK) return step;

          switch (parser->pattern_field_idx)
          {
            case 0: slot->note = parser->pattern_field_value; break;
            case 1: slot->instrument = parser->pattern_field_value; break;
            case 2: slot->volume_column = parser->pattern_field_value; break;
            case 3: slot->effect_type = parser->pattern_field_value; break;
            case 4: slot->effect_param = parser->pattern_field_value; break;
          }

          parser->pattern_read_done = 0;
          parser->pattern_packed_pos++;
          parser->pattern_field_idx++;
        }

        parser->pattern_slot_pos++;
        parser->pattern_state = XM_HOST_PATTERN_READ_NOTE;
      }
      break;

      case XM_HOST_PATTERN_READ_UNCOMPRESSED_FIELD:
      {
        while (parser->pattern_field_idx < 4)
        {
          if (parser->pattern_packed_pos >= packed_size)
          {
            g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
            return TRACK_HOST_STEP_ERROR;
          }

          int step = xm_host_stream_take(&parser->pattern_field_value, sizeof(parser->pattern_field_value), &parser->pattern_read_done);
          if (step != TRACK_HOST_STEP_OK) return step;

          switch (parser->pattern_field_idx)
          {
            case 0: slot->instrument = parser->pattern_field_value; break;
            case 1: slot->volume_column = parser->pattern_field_value; break;
            case 2: slot->effect_type = parser->pattern_field_value; break;
            case 3: slot->effect_param = parser->pattern_field_value; break;
          }

          parser->pattern_read_done = 0;
          parser->pattern_packed_pos++;
          parser->pattern_field_idx++;
        }

        parser->pattern_slot_pos++;
        parser->pattern_state = XM_HOST_PATTERN_READ_NOTE;
      }
      break;

      default:
        g_xm_host_stream.err = ESP_ERR_INVALID_STATE;
        return TRACK_HOST_STEP_ERROR;
    }
  }

  return TRACK_HOST_STEP_OK;
}

// Decode XM delta samples directly into the final sample buffer. This avoids a
// temporary stack buffer and keeps the decoder resumable across chunk boundaries.
int xm_host_load_sample_data_stream(xm_sample_t *sample)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;

  if (!sample)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (!sample->length) return TRACK_HOST_STEP_OK;

  if (!sample->data8)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_ARG;
    return TRACK_HOST_STEP_ERROR;
  }

  if (sample->bits == 16)
  {
    while (parser->sample_pos < sample->length || parser->sample_16_have_low)
    {
      if (!g_xm_host_stream.chunk_active || g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      {
        xm_host_stream_release_chunk();
        return TRACK_HOST_STEP_NEED_MORE;
      }

      const uint8_t *src = g_xm_host_stream.chunk_data + g_xm_host_stream.chunk_pos;
      size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
      size_t used = 0;

      if (parser->sample_16_have_low && avail)
      {
        // Previous chunk ended after the low byte of a 16-bit delta. Complete
        // that word first before processing aligned pairs from the current chunk.
        int16_t delta = (int16_t)((uint16_t)parser->sample_16_low | ((uint16_t)src[0] << 8));
        parser->sample_prev16 = (int16_t)(parser->sample_prev16 + delta);
        sample->data16[parser->sample_pos++] = parser->sample_prev16;
        parser->sample_16_have_low = 0;
        used = 1;
      }

      size_t left_samples = sample->length - parser->sample_pos;
      size_t pair_count = (avail - used) / 2;
      if (pair_count > left_samples) pair_count = left_samples;

      for (size_t i = 0; i < pair_count; i++)
      {
        int16_t delta = (int16_t)xm_rd_le16(src + used + i * 2);
        parser->sample_prev16 = (int16_t)(parser->sample_prev16 + delta);
        sample->data16[parser->sample_pos + i] = parser->sample_prev16;
      }

      parser->sample_pos += pair_count;
      used += pair_count * 2;

      if (parser->sample_pos < sample->length && used < avail)
      {
        parser->sample_16_low = src[used];
        parser->sample_16_have_low = 1;
        used++;
      }

      g_xm_host_stream.chunk_pos += used;
      g_xm_host_stream.pos += used;

      if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
        xm_host_stream_release_chunk();
    }
  }
  else
  {
    while (parser->sample_pos < sample->length)
    {
      if (!g_xm_host_stream.chunk_active || g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      {
        xm_host_stream_release_chunk();
        return TRACK_HOST_STEP_NEED_MORE;
      }

      const uint8_t *src = g_xm_host_stream.chunk_data + g_xm_host_stream.chunk_pos;
      size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
      size_t todo = sample->length - parser->sample_pos;
      if (todo > avail) todo = avail;

      for (size_t i = 0; i < todo; i++)
      {
        parser->sample_prev8 = (int8_t)(parser->sample_prev8 + (int8_t)src[i]);
        sample->data8[parser->sample_pos + i] = parser->sample_prev8;
      }

      parser->sample_pos += todo;
      g_xm_host_stream.chunk_pos += todo;
      g_xm_host_stream.pos += todo;

      if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
        xm_host_stream_release_chunk();
    }
  }

  return TRACK_HOST_STEP_OK;
}

void xm_host_stream_set_info(int handle, xm_context_t *ctx, size_t file_size)
{
  XM_INFO *info;

  if (!ctx) return;

  info = &xm_info[handle];
  memset(info, 0, sizeof(*info));
  info->valid = 1;
  info->version = xm_rd_le16(g_xm_host_stream.parser.header + 58);
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;
  strncpy(info->path, "host-stream", sizeof(info->path) - 1);
#if XM_STRINGS
  xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif
}

// Advance the host-stream FSM until it either finishes, fails, or runs out of
// current chunk data. Parser state is reset only from IDLE, never between chunks.
int xm_host_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;

  if (out_ctx) *out_ctx = NULL;
  if (out_used_size) *out_used_size = 0;
  if (!out_ctx || !out_used_size) return -1;

  if (parser->stage == TRACK_HOST_PARSE_IDLE)
  {
    memset(parser, 0, sizeof(*parser));
    parser->stage = TRACK_HOST_PARSE_READ_HEADER;
    g_xm_host_stream.err = ESP_OK;
  }

  while (1)
  {
    switch (parser->stage)
    {
      case TRACK_HOST_PARSE_READ_HEADER:
      {
        int step = xm_host_stream_take(parser->header, sizeof(parser->header), &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->read_done = 0;

#if XM_DEFENSIVE
        int ret = xm_check_sanity_preload((const char*)parser->header, sizeof(parser->header));
        if (ret) return -1;
#endif

        parser->stage = TRACK_HOST_PARSE_ALLOC_ARENA;
      }
      break;

      case TRACK_HOST_PARSE_ALLOC_ARENA:
      {
        parser->ctx = (xm_context_t*)xm_malloc(sizeof(xm_context_t));
        if (!parser->ctx) return -2;

        memset(parser->ctx, 0, sizeof(*parser->ctx));
        parser->ctx->ctx_size = sizeof(xm_context_t);
        parser->used_size = sizeof(xm_context_t);

        if (!tracker_context_register_segment(parser->ctx, parser->ctx, sizeof(xm_context_t), TRACKER_CONTEXT_SEG_CONTEXT))
        {
          free(parser->ctx);
          parser->ctx = NULL;
          return -2;
        }

        parser->ctx->tracker_format = TRACKER_FORMAT_XM;
        parser->ctx->rate = XM_SAMPLE_RATE;
        parser->mod = &parser->ctx->module;
        parser->stage = TRACK_HOST_PARSE_APPLY_XM_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_APPLY_XM_HEADER:
      {
#if XM_STRINGS
        memcpy(parser->mod->name, parser->header + 17, MODULE_NAME_LENGTH);
        memcpy(parser->mod->trackername, parser->header + 38, TRACKER_NAME_LENGTH);
#endif
        parser->stage = TRACK_HOST_PARSE_READ_MODULE_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_READ_MODULE_HEADER:
      {
        uint8_t *module_header = parser->module_header;
        if (!parser->read_done)
        {
          memset(module_header, 0, sizeof(parser->module_header));
          parser->module_header_size = 0;
        }

        if (parser->read_done < 4)
        {
          int step = xm_host_stream_take(module_header, 4, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        if (!parser->module_header_size)
        {
          parser->module_header_size = xm_rd_le32(module_header);
          if (parser->module_header_size < 4)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        int step = xm_host_stream_read_record_take(module_header, sizeof(parser->module_header), parser->module_header_size, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return -3;
        }

        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_APPLY_MODULE_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_APPLY_MODULE_HEADER:
      {
        uint8_t *module_header = parser->module_header;
        parser->mod->length = xm_rd_le16(module_header + 4);
        parser->mod->restart_position = xm_rd_le16(module_header + 6);
        parser->mod->num_channels = xm_rd_le16(module_header + 8);
        parser->mod->num_patterns = xm_rd_le16(module_header + 10);
        parser->mod->num_instruments = xm_rd_le16(module_header + 12);

        parser->mod->patterns = NULL;
        parser->mod->instruments = NULL;

        if (parser->mod->num_patterns)
        {
          size_t patterns_size;
          if (!tracker_size_mul(parser->mod->num_patterns, sizeof(xm_pattern_t), &patterns_size))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -3;
          }

          parser->mod->patterns = (xm_pattern_t*)xm_host_stream_alloc_block(patterns_size, TRACKER_CONTEXT_SEG_PATTERNS, true);
          if (!parser->mod->patterns)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -2;
          }
        }

        if (parser->mod->num_instruments)
        {
          size_t instruments_size;
          if (!tracker_size_mul(parser->mod->num_instruments, sizeof(xm_instrument_t), &instruments_size))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -3;
          }

          parser->mod->instruments = (xm_instrument_t*)xm_host_stream_alloc_block(instruments_size, TRACKER_CONTEXT_SEG_INSTRUMENTS, true);
          if (!parser->mod->instruments)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -2;
          }
        }

        {
          size_t runtime_size = 0;
          size_t channels_size;
          size_t loop_rows;
          size_t row_loop_size;
          xm_context_group_cursor_t runtime_cursor = {};

          if (!tracker_size_mul(parser->mod->num_channels, sizeof(xm_channel_context_t), &channels_size) ||
              !tracker_size_mul(parser->mod->length, MAX_NUM_ROWS, &loop_rows) ||
              !tracker_size_mul(loop_rows, sizeof(uint8_t), &row_loop_size) ||
              !tracker_size_add(&runtime_size, channels_size) ||
              !tracker_size_add(&runtime_size, row_loop_size))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -3;
          }

          runtime_cursor.runtime = runtime_size ? (char*)xm_host_stream_alloc_block(runtime_size, TRACKER_CONTEXT_SEG_RUNTIME, true) : NULL;
          if (runtime_size && !runtime_cursor.runtime)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -2;
          }

          runtime_cursor.runtime_end = runtime_cursor.runtime ? runtime_cursor.runtime + runtime_size : NULL;
          if (!xm_setup_runtime_group(parser->ctx, &runtime_cursor, XM_SAMPLE_RATE))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -2;
          }
        }

        uint16_t flags = xm_rd_le16(module_header + 14);
        parser->mod->frequency_type = (flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;
        parser->ctx->tempo = xm_rd_le16(module_header + 16);
        parser->ctx->bpm = xm_rd_le16(module_header + 18);
        memcpy(parser->mod->pattern_table, module_header + 20, PATTERN_ORDER_TABLE_LENGTH);

        parser->pattern_i = 0;
        parser->stage = TRACK_HOST_PARSE_READ_PATTERN_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_READ_PATTERN_HEADER:
      {
        if (parser->pattern_i >= parser->mod->num_patterns)
        {
          parser->instrument_i = 0;
          parser->read_done = 0;
          parser->stage = TRACK_HOST_PARSE_READ_INSTRUMENT_HEADER;
          break;
        }

        uint8_t *pattern_header = parser->pattern_header;
        if (!parser->read_done)
        {
          memset(pattern_header, 0, sizeof(parser->pattern_header));
          parser->pattern_header_size = 0;
        }

        if (parser->read_done < 4)
        {
          int step = xm_host_stream_take(pattern_header, 4, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        if (!parser->pattern_header_size)
        {
          parser->pattern_header_size = xm_rd_le32(pattern_header);
          if (parser->pattern_header_size < 4)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        int step = xm_host_stream_read_record_take(pattern_header, sizeof(parser->pattern_header), parser->pattern_header_size, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return -3;
        }

        parser->read_done = 0;
        parser->pat = parser->mod->patterns + parser->pattern_i;
        parser->pat->num_rows = xm_rd_le16(pattern_header + 5);
        parser->packed_pattern_size = xm_rd_le16(pattern_header + 7);
        {
          size_t slot_count;
          size_t slots_size;

          if (!tracker_size_mul(parser->mod->num_channels, parser->pat->num_rows, &slot_count) ||
              !tracker_size_mul(slot_count, sizeof(xm_pattern_slot_t), &slots_size))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -3;
          }

          parser->pat->slots = slots_size ? (xm_pattern_slot_t*)xm_host_stream_alloc_block(slots_size, TRACKER_CONTEXT_SEG_PATTERNS, true) : NULL;
          if (slots_size && !parser->pat->slots)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -2;
          }
        }

        parser->pattern_packed_pos = 0;
        parser->pattern_slot_pos = 0;
        parser->pattern_read_done = 0;
        parser->pattern_state = XM_HOST_PATTERN_READ_NOTE;
        parser->pattern_note = 0;
        parser->pattern_field_idx = 0;
        parser->pattern_field_value = 0;

        parser->stage = TRACK_HOST_PARSE_LOAD_PATTERN_DATA;
      }
      break;

      case TRACK_HOST_PARSE_LOAD_PATTERN_DATA:
      {
        if (parser->packed_pattern_size)
        {
          int step = xm_host_load_pattern_stream(parser->pat, parser->packed_pattern_size);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return g_xm_host_stream.err == ESP_ERR_NO_MEM ? -2 : -3;
          }
        }

        if (g_xm_host_stream.err != ESP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return g_xm_host_stream.err == ESP_ERR_NO_MEM ? -2 : -3;
        }

        parser->stage = TRACK_HOST_PARSE_NEXT_PATTERN;
      }
      break;

      case TRACK_HOST_PARSE_NEXT_PATTERN:
      {
        parser->pattern_i++;
        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_READ_PATTERN_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_READ_INSTRUMENT_HEADER:
      {
        if (parser->instrument_i >= parser->mod->num_instruments)
        {
          parser->read_done = 0;
          parser->stage = TRACK_HOST_PARSE_ALLOC_RUNTIME;
          break;
        }

        uint8_t *instrument_header = parser->instrument_header;
        if (!parser->read_done)
        {
          memset(instrument_header, 0, sizeof(parser->instrument_header));
          parser->instrument_header_size = 0;
          parser->instr = parser->mod->instruments + parser->instrument_i;
        }

        if (parser->read_done < 4)
        {
          int step = xm_host_stream_take(instrument_header, 4, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        if (!parser->instrument_header_size)
        {
          parser->instrument_header_size = xm_rd_le32(instrument_header);
          if (parser->instrument_header_size < 4)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }
        }

        int step = xm_host_stream_read_record_take(instrument_header, sizeof(parser->instrument_header), parser->instrument_header_size, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return -3;
        }

        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_APPLY_INSTRUMENT_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_APPLY_INSTRUMENT_HEADER:
      {
        uint8_t *instrument_header = parser->instrument_header;
#if XM_STRINGS
        memcpy(parser->instr->name, instrument_header + 4, INSTRUMENT_NAME_LENGTH);
#endif
        parser->instr->num_samples = xm_rd_le16(instrument_header + 27);

        if (parser->instr->num_samples > 0)
        {
          parser->sample_header_size = xm_rd_le32(instrument_header + 29);
          if (parser->sample_header_size < 4)
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            parser->arena = NULL;
            return -3;
          }

          memcpy(parser->instr->sample_of_notes, instrument_header + 33, NUM_NOTES);
          parser->instr->volume_envelope.num_points = instrument_header[225];
          parser->instr->panning_envelope.num_points = instrument_header[226];

          for (uint8_t j = 0; j < parser->instr->volume_envelope.num_points; j++)
          {
            parser->instr->volume_envelope.points[j].frame = xm_rd_le16(instrument_header + 129 + 4 * j);
            parser->instr->volume_envelope.points[j].value = xm_rd_le16(instrument_header + 129 + 4 * j + 2);
          }

          for (uint8_t j = 0; j < parser->instr->panning_envelope.num_points; j++)
          {
            parser->instr->panning_envelope.points[j].frame = xm_rd_le16(instrument_header + 177 + 4 * j);
            parser->instr->panning_envelope.points[j].value = xm_rd_le16(instrument_header + 177 + 4 * j + 2);
          }

          parser->instr->volume_envelope.sustain_point = instrument_header[227];
          parser->instr->volume_envelope.loop_start_point = instrument_header[228];
          parser->instr->volume_envelope.loop_end_point = instrument_header[229];
          parser->instr->panning_envelope.sustain_point = instrument_header[230];
          parser->instr->panning_envelope.loop_start_point = instrument_header[231];
          parser->instr->panning_envelope.loop_end_point = instrument_header[232];

          uint8_t env_flags = instrument_header[233];
          parser->instr->volume_envelope.enabled = env_flags & (1 << 0);
          parser->instr->volume_envelope.sustain_enabled = env_flags & (1 << 1);
          parser->instr->volume_envelope.loop_enabled = env_flags & (1 << 2);
          env_flags = instrument_header[234];
          parser->instr->panning_envelope.enabled = env_flags & (1 << 0);
          parser->instr->panning_envelope.sustain_enabled = env_flags & (1 << 1);
          parser->instr->panning_envelope.loop_enabled = env_flags & (1 << 2);
          parser->instr->vibrato_type = (xm_waveform_type_t)instrument_header[235];
          if (parser->instr->vibrato_type == 2)
            parser->instr->vibrato_type = XM_RAMP_DOWN_WAVEFORM;
          else if (parser->instr->vibrato_type == 1)
            parser->instr->vibrato_type = XM_SQUARE_WAVEFORM;
          parser->instr->vibrato_sweep = instrument_header[236];
          parser->instr->vibrato_depth = instrument_header[237];
          parser->instr->vibrato_rate = instrument_header[238];
          parser->instr->volume_fadeout = xm_rd_le16(instrument_header + 239);
          {
            size_t samples_meta_size;

            if (!tracker_size_mul(parser->instr->num_samples, sizeof(xm_sample_t), &samples_meta_size))
            {
              xm_free_context(parser->ctx);
              parser->ctx = NULL;
              return -3;
            }

            parser->instr->samples = (xm_sample_t*)xm_host_stream_alloc_block(samples_meta_size, TRACKER_CONTEXT_SEG_INSTRUMENTS, true);
            if (!parser->instr->samples)
            {
              xm_free_context(parser->ctx);
              parser->ctx = NULL;
              return -2;
            }
          }

          parser->sample_i = 0;
          parser->instrument_sample_bytes = 0;
          parser->instrument_sample_data = NULL;
          parser->instrument_sample_data_pos = NULL;
          parser->instrument_sample_data_end = NULL;
          parser->stage = TRACK_HOST_PARSE_READ_SAMPLE_HEADER;
        }
        else
        {
          parser->instr->samples = NULL;
          parser->stage = TRACK_HOST_PARSE_NEXT_INSTRUMENT;
        }
      }
      break;

      case TRACK_HOST_PARSE_READ_SAMPLE_HEADER:
      {
        if (parser->sample_i >= parser->instr->num_samples)
        {
          parser->read_done = 0;
          parser->sample_data_i = 0;

          if (parser->instrument_sample_bytes)
          {
            parser->instrument_sample_data = (char*)xm_host_stream_alloc_block(parser->instrument_sample_bytes, TRACKER_CONTEXT_SEG_SAMPLES, false);
            if (!parser->instrument_sample_data)
            {
              xm_free_context(parser->ctx);
              parser->ctx = NULL;
              return -2;
            }
          }

          parser->instrument_sample_data_pos = parser->instrument_sample_data;
          parser->instrument_sample_data_end = parser->instrument_sample_data ? parser->instrument_sample_data + parser->instrument_sample_bytes : NULL;

          for (uint16_t i = 0; i < parser->instr->num_samples; i++)
          {
            xm_sample_t *sample = parser->instr->samples + i;
            size_t sample_bytes = sample->bits == 16 ? (size_t)sample->length * 2 : (size_t)sample->length;

            if (!sample_bytes)
            {
              sample->data8 = NULL;
              sample->data16 = NULL;
              continue;
            }

            if (!parser->instrument_sample_data_pos || parser->instrument_sample_data_pos > parser->instrument_sample_data_end || sample_bytes > (size_t)(parser->instrument_sample_data_end - parser->instrument_sample_data_pos))
            {
              xm_free_context(parser->ctx);
              parser->ctx = NULL;
              return -3;
            }

            sample->data8 = (int8_t*)parser->instrument_sample_data_pos;
            parser->instrument_sample_data_pos += sample_bytes;
          }

          parser->stage = TRACK_HOST_PARSE_LOAD_SAMPLE_DATA;
          break;
        }

        uint8_t *sample_header = parser->sample_header;
        if (!parser->read_done)
        {
          parser->sample = parser->instr->samples + parser->sample_i;
          memset(sample_header, 0, sizeof(parser->sample_header));
        }

        int step = xm_host_stream_read_record_take(sample_header, sizeof(parser->sample_header), parser->sample_header_size, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return -3;
        }

        parser->read_done = 0;
        {
          uint32_t sample_bytes = xm_rd_le32(sample_header);

          parser->sample->length = sample_bytes;
          if (!tracker_size_add(&parser->instrument_sample_bytes, sample_bytes))
          {
            xm_free_context(parser->ctx);
            parser->ctx = NULL;
            return -3;
          }
        }

        parser->sample->loop_start = xm_rd_le32(sample_header + 4);
        parser->sample->loop_length = xm_rd_le32(sample_header + 8);
        parser->sample->loop_end = parser->sample->loop_start + parser->sample->loop_length;
        parser->sample->volume = (float)sample_header[12] / (float)0x40;
        parser->sample->finetune = (int8_t)sample_header[13];

        uint8_t sample_flags = sample_header[14];
        if ((sample_flags & 3) == 0)
          parser->sample->loop_type = XM_NO_LOOP;
        else if ((sample_flags & 3) == 1)
          parser->sample->loop_type = XM_FORWARD_LOOP;
        else
          parser->sample->loop_type = XM_PING_PONG_LOOP;

        parser->sample->bits = (sample_flags & (1 << 4)) ? 16 : 8;
        parser->sample->panning = (float)sample_header[15] / (float)0xFF;
        parser->sample->relative_note = (int8_t)sample_header[16];
#if XM_STRINGS
        memcpy(parser->sample->name, sample_header + 18, SAMPLE_NAME_LENGTH);
#endif
        parser->sample->data8 = NULL;
        parser->sample->data16 = NULL;

        if (parser->sample->bits == 16)
        {
          parser->sample->loop_start >>= 1;
          parser->sample->loop_length >>= 1;
          parser->sample->loop_end >>= 1;
          parser->sample->length >>= 1;
        }

        parser->sample_i++;
        parser->stage = TRACK_HOST_PARSE_READ_SAMPLE_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_LOAD_SAMPLE_DATA:
      {
        if (parser->sample_data_i >= parser->instr->num_samples)
        {
          parser->sample_data_started = 0;
          parser->stage = TRACK_HOST_PARSE_NEXT_INSTRUMENT;
          break;
        }

        xm_sample_t *sample = parser->instr->samples + parser->sample_data_i;
        if (!parser->sample_data_started)
        {
          parser->sample = sample;
          parser->sample_pos = 0;
          parser->sample_prev8 = 0;
          parser->sample_prev16 = 0;
          parser->sample_16_have_low = 0;
          parser->sample_16_low = 0;
          parser->sample_data_started = 1;
        }

        int step = xm_host_load_sample_data_stream(sample);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK || g_xm_host_stream.err != ESP_OK)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return g_xm_host_stream.err == ESP_ERR_NO_MEM ? -2 : -3;
        }

        parser->sample_data_i++;
        parser->sample_data_started = 0;
        parser->stage = TRACK_HOST_PARSE_LOAD_SAMPLE_DATA;
      }
      break;

      case TRACK_HOST_PARSE_NEXT_INSTRUMENT:
      {
        parser->instrument_i++;
        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_READ_INSTRUMENT_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_ALLOC_RUNTIME:
      {
        if (!parser->ctx->channels || !parser->ctx->row_loop_count)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          return -2;
        }

        parser->stage = TRACK_HOST_PARSE_POSTLOAD;
      }
      break;

      case TRACK_HOST_PARSE_POSTLOAD:
      {
#if XM_DEFENSIVE
        int ret = xm_check_sanity_postload(parser->ctx);
        if (ret)
        {
          xm_free_context(parser->ctx);
          parser->ctx = NULL;
          parser->arena = NULL;
          return -1;
        }
#endif

        if (g_xm_host_stream.pos < g_xm_host_stream.total_size)
        {
          parser->trailing_size = g_xm_host_stream.total_size - g_xm_host_stream.pos;
          parser->trailing_done = 0;
          parser->stage = TRACK_HOST_PARSE_DRAIN_TRAILING;
        }
        else
          parser->stage = TRACK_HOST_PARSE_DONE;
      }
      break;

      case TRACK_HOST_PARSE_SHRINK:
      {
        if (g_xm_host_stream.pos < g_xm_host_stream.total_size)
        {
          parser->trailing_size = g_xm_host_stream.total_size - g_xm_host_stream.pos;
          parser->trailing_done = 0;
          parser->stage = TRACK_HOST_PARSE_DRAIN_TRAILING;
        }
        else
          parser->stage = TRACK_HOST_PARSE_DONE;
      }
      break;

      case TRACK_HOST_PARSE_DRAIN_TRAILING:
      {
        int step = xm_host_stream_skip_take(parser->trailing_size, &parser->trailing_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->trailing_size = 0;
        parser->trailing_done = 0;
        parser->stage = TRACK_HOST_PARSE_DONE;
      }
      break;

      case TRACK_HOST_PARSE_DONE:
      {
        *out_ctx = parser->ctx;
        *out_used_size = parser->used_size;
        return (int)parser->used_size;
      }

      default:
        return -1;
    }
  }
}
void xm_host_mod_stream_set_info(int handle, xm_context_t *ctx, size_t file_size)
{
  if (!check_handle(handle) || !ctx) return;

  xm_clear_info(handle);
  XM_INFO *info = &xm_info[handle];
  info->valid = 1;
  info->header_size = MOD_HEADER_SIZE;
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;
#if XM_STRINGS
  strncpy(info->path, "host-mod-stream", sizeof(info->path) - 1);
  xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif
}

int xm_host_mod_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;

  if (out_ctx) *out_ctx = NULL;
  if (out_used_size) *out_used_size = 0;
  if (!out_ctx || !out_used_size) return -1;

  if (parser->stage == TRACK_HOST_PARSE_IDLE)
  {
    memset(parser, 0, sizeof(*parser));
    parser->stage = TRACK_HOST_PARSE_MOD_READ_HEADER;
    g_xm_host_stream.err = ESP_OK;
  }

  while (1)
  {
    switch (parser->stage)
    {
      case TRACK_HOST_PARSE_MOD_READ_HEADER:
      {
        if (!parser->mod_header)
        {
          parser->mod_header = (uint8_t*)heap_caps_malloc(MOD_HEADER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
          if (!parser->mod_header) return -2;
        }

        int step = xm_host_stream_take(parser->mod_header, MOD_15_HEADER_SIZE, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->mod_header_length = MOD_15_HEADER_SIZE;
        if (g_xm_host_stream.total_size >= MOD_HEADER_SIZE)
        {
          step = xm_host_stream_take(parser->mod_header, MOD_HEADER_SIZE, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;
          parser->mod_header_length = MOD_HEADER_SIZE;
        }

        if (!mod_read_layout_from_header(parser->mod_header, parser->mod_header_length, g_xm_host_stream.total_size, &parser->mod_layout))
          return -3;

        parser->mod_preload_pos = 0;
        parser->mod_preload_size = parser->mod_header_length > parser->mod_layout.header_size
          ? parser->mod_header_length - parser->mod_layout.header_size
          : 0;
        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_MOD_ALLOC_CONTEXT;
      }
      break;

      case TRACK_HOST_PARSE_MOD_ALLOC_CONTEXT:
      {
        if (!mod_get_group_sizes_for_layout(&parser->mod_layout, &parser->group_sizes)) return -2;
        parser->used_size = parser->group_sizes.total_size;
        if (!parser->used_size || parser->used_size > INT_MAX) return -2;

        if (!xm_allocate_context_groups(&parser->ctx, &parser->cursor, &parser->group_sizes, xm_malloc)) return -2;
        parser->arena = NULL;
        parser->arena_size = 0;
        parser->mod = &parser->ctx->module;

        if (!mod_setup_module_header(parser->ctx, parser->mod_header, &parser->mod_layout, &parser->cursor)) return -3;

        parser->pattern_i = 0;
        parser->pattern_slot_pos = 0;
        parser->pat = NULL;
        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_MOD_LOAD_PATTERN_DATA;
      }
      break;

      case TRACK_HOST_PARSE_MOD_LOAD_PATTERN_DATA:
      {
        if (parser->pattern_i >= parser->mod->num_patterns)
        {
          if (!mod_layout_set_efx_backup_mask(&parser->mod_layout, mod_scan_efx_sample_mask_from_context(parser->ctx, &parser->mod_layout)))
            return -3;
          if (!mod_get_group_sizes_for_layout(&parser->mod_layout, &parser->group_sizes))
            return -3;
          parser->used_size = parser->group_sizes.total_size;
          parser->ctx->ctx_size = parser->used_size;
          if (!mod_allocate_format_extra_group(parser->ctx, &parser->cursor, &parser->group_sizes, xm_malloc))
            return -2;

          parser->sample_i = 0;
          parser->sample_data_started = 0;
          parser->read_done = 0;
          parser->stage = TRACK_HOST_PARSE_MOD_LOAD_SAMPLE_DATA;
          break;
        }

        if (!parser->pat)
        {
          parser->pat = parser->mod->patterns + parser->pattern_i;
          parser->pat->num_rows = MOD_ROWS_PER_PATTERN;
          parser->pat->slots = (xm_pattern_slot_t*)xm_group_cursor_alloc(&parser->cursor.patterns, parser->cursor.patterns_end, (size_t)MOD_ROWS_PER_PATTERN * parser->mod->num_channels * sizeof(xm_pattern_slot_t), true);
          if (!parser->pat->slots) return -3;
          parser->pattern_slot_pos = 0;
        }

        uint32_t slot_count = (uint32_t)MOD_ROWS_PER_PATTERN * parser->mod->num_channels;
        while (parser->pattern_slot_pos < slot_count)
        {
          int step = xm_host_mod_stream_take(parser->mod_entry, sizeof(parser->mod_entry), &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;

          mod_decode_pattern_slot(parser->pat->slots + parser->pattern_slot_pos, parser->mod_entry);
          parser->read_done = 0;
          parser->pattern_slot_pos++;
        }

        parser->pattern_i++;
        parser->pattern_slot_pos = 0;
        parser->pat = NULL;
        parser->stage = TRACK_HOST_PARSE_MOD_LOAD_PATTERN_DATA;
      }
      break;

      case TRACK_HOST_PARSE_MOD_LOAD_SAMPLE_DATA:
      {
        if (parser->sample_i >= parser->mod_layout.sample_count)
        {
          parser->stage = TRACK_HOST_PARSE_MOD_ALLOC_RUNTIME;
          break;
        }

        if (!parser->sample_data_started)
        {
          if (!mod_setup_sample_metadata(parser->ctx, parser->mod_header, &parser->mod_layout, parser->sample_i, &parser->cursor, xm_malloc)) return -3;

          parser->sample = parser->mod->instruments[parser->sample_i].samples;
          parser->sample_pos = 0;
          parser->sample_data_started = 1;
        }

        if (parser->sample && parser->mod_layout.sample_file_length[parser->sample_i])
        {
          int step = xm_host_mod_stream_take(parser->sample->data8, parser->mod_layout.sample_file_length[parser->sample_i], &parser->sample_pos);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;
        }
        if (parser->sample && parser->mod_layout.sample_file_length[parser->sample_i] < parser->sample->length)
          memset(parser->sample->data8 + parser->mod_layout.sample_file_length[parser->sample_i], 0, parser->sample->length - parser->mod_layout.sample_file_length[parser->sample_i]);

        parser->sample_i++;
        parser->sample_data_started = 0;
        parser->sample = NULL;
        parser->stage = TRACK_HOST_PARSE_MOD_LOAD_SAMPLE_DATA;
      }
      break;

      case TRACK_HOST_PARSE_MOD_ALLOC_RUNTIME:
      {
        if (!mod_setup_context_runtime(parser->ctx, &parser->cursor, XM_SAMPLE_RATE)) return -3;

        if (!mod_setup_efx_backups(parser->ctx, &parser->mod_layout, &parser->cursor)) return -3;

        parser->ctx->ctx_size = parser->used_size;

        int ret = mod_finish_context(&parser->ctx, parser->ctx);
        if (ret < 0) return -3;

        if (parser->mod_header)
        {
          heap_caps_free(parser->mod_header);
          parser->mod_header = NULL;
        }

        if (g_xm_host_stream.pos < g_xm_host_stream.total_size)
        {
          parser->trailing_size = g_xm_host_stream.total_size - g_xm_host_stream.pos;
          parser->trailing_done = 0;
          parser->stage = TRACK_HOST_PARSE_DRAIN_TRAILING;
        }
        else
          parser->stage = TRACK_HOST_PARSE_MOD_DONE;
      }
      break;

      case TRACK_HOST_PARSE_DRAIN_TRAILING:
      {
        int step = xm_host_stream_skip_take(parser->trailing_size, &parser->trailing_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->trailing_size = 0;
        parser->trailing_done = 0;
        parser->stage = TRACK_HOST_PARSE_MOD_DONE;
      }
      break;

      case TRACK_HOST_PARSE_MOD_DONE:
      {
        if (parser->mod_header)
        {
          heap_caps_free(parser->mod_header);
          parser->mod_header = NULL;
        }

        *out_ctx = parser->ctx;
        *out_used_size = parser->used_size;
        return (int)parser->used_size;
      }

      default:
        return -1;
    }
  }
}

int xm_host_stream_take_at(size_t offset, void *dst, size_t size, size_t *done)
{
  if (!done) return TRACK_HOST_STEP_ERROR;

  if (*done == 0 && g_xm_host_stream.pos != offset)
  {
    if (offset > g_xm_host_stream.total_size)
    {
      g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
      return TRACK_HOST_STEP_ERROR;
    }

    if (g_xm_host_stream.chunk_active)
    {
      size_t chunk_base = g_xm_host_stream.pos - g_xm_host_stream.chunk_pos;
      size_t chunk_end = chunk_base + g_xm_host_stream.chunk_size;

      if (offset >= chunk_base && offset <= chunk_end)
      {
        g_xm_host_stream.chunk_pos = offset - chunk_base;
        g_xm_host_stream.pos = offset;
        return xm_host_stream_take(dst, size, done);
      }
    }

    xm_host_stream_release_chunk();
    g_xm_host_stream.pos = offset;
  }

  return xm_host_stream_take(dst, size, done);
}

int xm_host_s3m_apply_header(s3m_layout_t *layout, const uint8_t *header, size_t file_size)
{
  size_t tables_size = 0;
  size_t v;

  if (!layout || !header) return 0;
  memset(layout, 0, sizeof(*layout));

  if (file_size < S3M_HEADER_SIZE)
  {
    s3m_set_error("S3M header is shorter than 96 bytes");
    return 0;
  }

  if (memcmp(header + S3M_MAGIC_OFFSET, "SCRM", 4) != 0)
  {
    s3m_set_error("S3M missing SCRM signature");
    return 0;
  }

  if (header[S3M_FILE_TYPE_OFFSET] != S3M_FILE_TYPE_MODULE)
  {
    s3m_set_error_fmt("S3M invalid file type: 0x%02X", header[S3M_FILE_TYPE_OFFSET]);
    return 0;
  }

  memcpy(layout->name, header, sizeof(layout->name));
  layout->order_count = tracker_rd_le16(header + S3M_ORDERS_OFFSET);
  layout->sample_count = tracker_rd_le16(header + S3M_SAMPLES_OFFSET);
  layout->pattern_count = tracker_rd_le16(header + S3M_PATTERNS_OFFSET);
  layout->flags = tracker_rd_le16(header + S3M_FLAGS_OFFSET);
  layout->cwtv = tracker_rd_le16(header + S3M_CWTV_OFFSET);
  layout->format_version = tracker_rd_le16(header + S3M_FORMAT_VERSION_OFFSET);
  layout->global_volume = header[S3M_GLOBAL_VOLUME_OFFSET];
  layout->speed = header[S3M_SPEED_OFFSET];
  layout->tempo = header[S3M_TEMPO_OFFSET];
  layout->master_volume = header[S3M_MASTER_VOLUME_OFFSET];
  layout->stereo = (layout->master_volume & 0x80) ? 1 : 0;
  layout->has_panning_table = (header[S3M_PANNING_TABLE_FLAG_OFFSET] == S3M_PANNING_TABLE_PRESENT);
  memcpy(layout->channel_settings, header + S3M_CHANNEL_SETTINGS_OFFSET, S3M_MAX_CHANNELS);
  memset(layout->channel_map, 0xFF, sizeof(layout->channel_map));
  memset(layout->channel_opl, S3M_ADLIB_CHANNEL_NONE, sizeof(layout->channel_opl));

  if (layout->format_version != S3M_FORMAT_SIGNED && layout->format_version != S3M_FORMAT_UNSIGNED)
  {
    s3m_set_error_fmt("S3M unsupported sample format version: 0x%04X", layout->format_version);
    return 0;
  }
  layout->signed_samples = (layout->format_version == S3M_FORMAT_SIGNED);

  if (!layout->order_count || layout->order_count > S3M_MAX_ORDERS)
  {
    s3m_set_error_fmt("S3M unsupported order count: %u", layout->order_count);
    return 0;
  }

  if (layout->sample_count > S3M_MAX_SAMPLES)
  {
    s3m_set_error_fmt("S3M unsupported sample count: %u", layout->sample_count);
    return 0;
  }

  if (!layout->pattern_count || layout->pattern_count > S3M_MAX_PATTERNS)
  {
    s3m_set_error_fmt("S3M unsupported pattern count: %u", layout->pattern_count);
    return 0;
  }

  for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
  {
    uint8_t setting = layout->channel_settings[i];

    if (setting != 0xFF)
    {
      uint8_t mapped = (uint8_t)layout->channels++;

      layout->channel_map[i] = mapped;
      setting &= 0x7F;
      if (setting >= S3M_ADLIB_CHANNEL_FIRST && setting < S3M_ADLIB_CHANNEL_FIRST + S3M_ADLIB_CHANNEL_COUNT)
      {
        layout->channel_opl[mapped] = (uint8_t)(setting - S3M_ADLIB_CHANNEL_FIRST);
        layout->has_opl_channels = 1;
      }
    }
  }

  if (layout->channels < S3M_MIN_CHANNELS || layout->channels > S3M_MAX_CHANNELS)
  {
    s3m_set_error_fmt("S3M unsupported channel count: %u", layout->channels);
    return 0;
  }

  if (!tracker_size_add(&tables_size, layout->order_count)) return 0;
  if (!tracker_size_mul(layout->sample_count, 2, &v)) return 0;
  if (!tracker_size_add(&tables_size, v)) return 0;
  if (!tracker_size_mul(layout->pattern_count, 2, &v)) return 0;
  if (!tracker_size_add(&tables_size, v)) return 0;

  if (file_size < S3M_HEADER_SIZE || file_size - S3M_HEADER_SIZE < tables_size)
  {
    s3m_set_error("S3M truncated order/pointer tables");
    return 0;
  }

  s3m_set_default_panning(layout);
  return 1;
}

int xm_host_s3m_finish_tables(s3m_layout_t *layout)
{
  uint8_t max_pattern = 0;

  if (!layout) return 0;

  s3m_compact_channel_panning(layout);

  for (uint16_t i = 0; i < layout->order_count; i++)
  {
    uint8_t order = layout->orders[i];

    if (order == 0xFF) break;
    if (order == 0xFE) continue;
    if (order >= layout->pattern_count)
    {
      s3m_set_error_fmt("S3M order %u references invalid pattern %u", i, order);
      return 0;
    }

    layout->pattern_table[layout->song_length++] = order;
    if (order > max_pattern) max_pattern = order;
  }

  if (!layout->song_length)
  {
    s3m_set_error("S3M has no playable orders");
    return 0;
  }

  if ((uint16_t)max_pattern + 1 < layout->pattern_count)
    layout->pattern_count = (uint16_t)max_pattern + 1;

  if (!tracker_size_mul(layout->pattern_count, S3M_ROWS_PER_PATTERN, &layout->pattern_slots_bytes)) return 0;
  if (!tracker_size_mul(layout->pattern_slots_bytes, layout->channels, &layout->pattern_slots_bytes)) return 0;
  if (!tracker_size_mul(layout->pattern_slots_bytes, sizeof(xm_pattern_slot_t), &layout->pattern_slots_bytes)) return 0;

  return 1;
}

int xm_host_s3m_apply_sample_header(s3m_layout_t *layout, uint16_t i, const uint8_t *h, size_t file_size)
{
  s3m_sample_layout_t *sample;
  size_t data_offset;
  size_t sample_bytes;

  if (!layout || !h || i >= layout->sample_count) return 0;

  sample = layout->samples + i;
  sample->header_offset = (uint32_t)s3m_para_offset(layout->sample_para[i]);
  memcpy(sample->name, h + 48, sizeof(sample->name));
  sample->type = h[0];
  sample->data_offset = (((uint32_t)h[13] << 16) | tracker_rd_le16(h + 14)) << 4;
  sample->length = tracker_rd_le32(h + 16);
  sample->loop_start = tracker_rd_le32(h + 20);
  sample->loop_end = tracker_rd_le32(h + 24);
  sample->volume = h[28];
  sample->pack = h[30];
  sample->flags = h[31];
  sample->c4speed = tracker_rd_le32(h + 32);
  if (!sample->c4speed) sample->c4speed = S3M_DEFAULT_C4SPEED;

  sample->channels = (sample->flags & S3M_SAMPLE_FLAG_STEREO) ? 2 : 1;

  if (sample->type == S3M_SAMPLE_TYPE_NONE)
  {
    sample->length = 0;
    sample->file_length = 0;
    sample->right_file_length = 0;
    sample->channels = 1;
    return 1;
  }

  if (sample->type == S3M_SAMPLE_TYPE_ADLIB)
  {
    s3m_adlib_instrument_t *adlib = layout->adlib_instruments + i;

    if (memcmp(h + 76, "SCRI", 4) != 0)
    {
      s3m_set_error_fmt("S3M AdLib instrument %u missing SCRI signature", i + 1);
      return 0;
    }

    adlib->type = sample->type;
    adlib->volume = sample->volume;
    adlib->c4speed = sample->c4speed;
    memcpy(adlib->regs, h + 16, S3M_ADLIB_REG_COUNT);
    layout->has_adlib_instruments = 1;
    sample->length = 0;
    sample->file_length = 0;
    sample->right_file_length = 0;
    sample->loop_start = 0;
    sample->loop_end = 0;
    sample->flags = 0;
    sample->channels = 1;
    return 1;
  }

  if (sample->type != S3M_SAMPLE_TYPE_PCM)
  {
    s3m_set_error_fmt("S3M unsupported sample %u type: %u", i + 1, sample->type);
    return 0;
  }

  if (memcmp(h + 76, "SCRS", 4) != 0)
  {
    s3m_set_error_fmt("S3M sample %u missing SCRS signature", i + 1);
    return 0;
  }

  if (sample->pack != S3M_SAMPLE_PACK_NONE)
  {
    s3m_set_error_fmt("S3M unsupported packed sample %u: pack=%u", i + 1, sample->pack);
    return 0;
  }

  if (sample->length > INT_MAX / 2)
  {
    s3m_set_error_fmt("S3M sample %u is too large", i + 1);
    return 0;
  }

  sample_bytes = sample->length;
  if (sample->flags & S3M_SAMPLE_FLAG_16BIT)
  {
    if (!tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;
  }
  if (sample->channels == 2 && !tracker_size_mul(sample_bytes, 2, &sample_bytes)) return 0;

  data_offset = sample->data_offset;
  sample->file_length = sample->length;
  sample->right_file_length = 0;
  if (sample_bytes)
  {
    size_t bytes_per_sample = (sample->flags & S3M_SAMPLE_FLAG_16BIT) ? 2 : 1;
    size_t left_bytes = (size_t)sample->length * bytes_per_sample;

    if (data_offset >= file_size)
      sample->file_length = 0;
    else
    {
      size_t available = file_size - data_offset;
      size_t left_available = available < left_bytes ? available : left_bytes;
      sample->file_length = (uint32_t)(left_available / bytes_per_sample);

      if (sample->channels == 2 && available > left_bytes)
      {
        size_t right_available = available - left_bytes;
        if (right_available > left_bytes) right_available = left_bytes;
        sample->right_file_length = (uint32_t)(right_available / bytes_per_sample);
      }
    }
  }

  if (!tracker_size_add(&layout->sample_bytes, sample_bytes)) return 0;
  return 1;
}

int xm_host_s3m_read_u8(XmHostStreamParser *parser, size_t base, size_t *pos, uint8_t *out)
{
  int step;

  if (!parser || !pos || !out) return TRACK_HOST_STEP_ERROR;
  if (*pos >= parser->packed_pattern_size || base > g_xm_host_stream.total_size || *pos > g_xm_host_stream.total_size - base)
  {
    return TRACK_HOST_STEP_NO_MORE;
  }

  step = xm_host_stream_take_at(base + *pos, out, 1, &parser->read_done);
  if (step != TRACK_HOST_STEP_OK) return step;

  parser->read_done = 0;
  (*pos)++;
  return TRACK_HOST_STEP_OK;
}

int xm_host_s3m_decode_pattern_stream(XmHostStreamParser *parser)
{
  s3m_layout_t *layout;
  xm_pattern_slot_t *slot;
  uint8_t channel;
  size_t base;
  int step;

  if (!parser || !parser->pat || !parser->s3m_layout) return TRACK_HOST_STEP_ERROR;

  layout = parser->s3m_layout;
  if (!layout->pattern_para[parser->pattern_i]) return TRACK_HOST_STEP_OK;

  base = s3m_para_offset(layout->pattern_para[parser->pattern_i]);
  if (base > g_xm_host_stream.total_size || g_xm_host_stream.total_size - base < 2)
  {
    s3m_set_error_fmt("S3M pattern %u offset outside file", parser->pattern_i);
    return TRACK_HOST_STEP_ERROR;
  }

  if (parser->packed_pattern_size < 2) return TRACK_HOST_STEP_OK;
  if ((size_t)parser->packed_pattern_size > g_xm_host_stream.total_size - base)
  {
    s3m_set_error_fmt("S3M pattern %u data outside file", parser->pattern_i);
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return TRACK_HOST_STEP_ERROR;
  }

  while (parser->s3m_pattern_row < S3M_ROWS_PER_PATTERN)
  {
    if (parser->pattern_state == XM_HOST_PATTERN_READ_NOTE && parser->pattern_packed_pos >= parser->packed_pattern_size)
      return TRACK_HOST_STEP_OK;

    switch (parser->pattern_state)
    {
      case XM_HOST_PATTERN_READ_NOTE:
        parser->pattern_event_pos = parser->pattern_packed_pos;
        step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_info);
        if (step == TRACK_HOST_STEP_NO_MORE) return TRACK_HOST_STEP_OK;
        if (step != TRACK_HOST_STEP_OK) return step;
        if (!parser->s3m_info)
        {
          parser->s3m_pattern_row++;
          break;
        }

        parser->s3m_note = S3M_NOTE_NONE;
        parser->s3m_instr = 0;
        parser->s3m_volume = 0xFF;
        parser->s3m_command = 0;
        parser->s3m_param = 0;
        parser->pattern_state = XM_HOST_PATTERN_READ_COMPRESSED_FIELD;
        parser->pattern_field_idx = 0;
        break;

      case XM_HOST_PATTERN_READ_COMPRESSED_FIELD:
        if (parser->pattern_field_idx == 0)
        {
          if (parser->s3m_info & S3M_PATTERN_NOTE_PRESENT)
          {
            step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_note);
            if (step == TRACK_HOST_STEP_NO_MORE)
            {
              if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
              s3m_set_error_fmt("S3M pattern %u truncated note at row %u", parser->pattern_i, parser->s3m_pattern_row);
              return TRACK_HOST_STEP_ERROR;
            }
            if (step != TRACK_HOST_STEP_OK) return step;
            parser->pattern_field_idx = 1;
            break;
          }

          parser->pattern_field_idx = 2;
        }

        if (parser->pattern_field_idx == 1)
        {
          step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_instr);
          if (step == TRACK_HOST_STEP_NO_MORE)
          {
            if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
            s3m_set_error_fmt("S3M pattern %u truncated instrument at row %u", parser->pattern_i, parser->s3m_pattern_row);
            return TRACK_HOST_STEP_ERROR;
          }
          if (step != TRACK_HOST_STEP_OK) return step;
          parser->pattern_field_idx = 2;
          break;
        }

        if (parser->pattern_field_idx == 2)
        {
          if (parser->s3m_info & S3M_PATTERN_VOLUME_PRESENT)
          {
            step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_volume);
            if (step == TRACK_HOST_STEP_NO_MORE)
            {
              if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
              s3m_set_error_fmt("S3M pattern %u truncated volume at row %u", parser->pattern_i, parser->s3m_pattern_row);
              return TRACK_HOST_STEP_ERROR;
            }
            if (step != TRACK_HOST_STEP_OK) return step;
            parser->pattern_field_idx = 3;
            break;
          }

          parser->pattern_field_idx = 3;
        }

        if (parser->pattern_field_idx == 3)
        {
          if (parser->s3m_info & S3M_PATTERN_EFFECT_PRESENT)
          {
            step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_command);
            if (step == TRACK_HOST_STEP_NO_MORE)
            {
              if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
              s3m_set_error_fmt("S3M pattern %u truncated effect at row %u", parser->pattern_i, parser->s3m_pattern_row);
              return TRACK_HOST_STEP_ERROR;
            }
            if (step != TRACK_HOST_STEP_OK) return step;
            parser->pattern_field_idx = 4;
            break;
          }

          parser->pattern_field_idx = 5;
        }

        if (parser->pattern_field_idx == 4)
        {
          step = xm_host_s3m_read_u8(parser, base, &parser->pattern_packed_pos, &parser->s3m_param);
          if (step == TRACK_HOST_STEP_NO_MORE)
          {
            if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
            s3m_set_error_fmt("S3M pattern %u truncated effect parameter at row %u", parser->pattern_i, parser->s3m_pattern_row);
            return TRACK_HOST_STEP_ERROR;
          }
          if (step != TRACK_HOST_STEP_OK) return step;
          parser->pattern_field_idx = 5;
          break;
        }

        channel = parser->s3m_info & S3M_PATTERN_CHANNEL_MASK;
        channel = layout->channel_map[channel];
        if (channel != 0xFF)
        {
          slot = parser->pat->slots + (size_t)parser->s3m_pattern_row * layout->channels + channel;
          slot->note = s3m_note_to_xm(parser->s3m_note);
          slot->instrument = parser->s3m_instr;
          if (parser->s3m_volume != 0xFF)
          {
            if (parser->s3m_volume >= 128 && parser->s3m_volume <= 192)
              slot->volume_column = parser->s3m_volume;
            else
            {
              if (parser->s3m_volume > 64) parser->s3m_volume = 64;
              slot->volume_column = (uint8_t)(0x10 + parser->s3m_volume);
            }
          }

          if (!s3m_convert_effect(slot, parser->s3m_command, parser->s3m_param, parser->pattern_i, parser->s3m_pattern_row, channel))
          {
            if (parser->pattern_event_pos >= parser->packed_pattern_soft_size) return TRACK_HOST_STEP_OK;
            return TRACK_HOST_STEP_ERROR;
          }
        }

        parser->pattern_state = XM_HOST_PATTERN_READ_NOTE;
        parser->pattern_field_idx = 0;
        break;

      default:
        return TRACK_HOST_STEP_ERROR;
    }
  }

  return TRACK_HOST_STEP_OK;
}

void xm_host_s3m_convert_sample_data(xm_sample_t *sample, const s3m_sample_layout_t *sl, bool signed_samples)
{
  uint32_t left_file_length;
  uint32_t right_file_length;
  size_t bytes_per_sample;
  size_t right_offset;

  if (!sample || !sl || !sample->data8 || !sample->length) return;

  bytes_per_sample = (sample->bits == 16) ? 2 : 1;
  right_offset = (size_t)sample->length * bytes_per_sample;
  left_file_length = sl->file_length;
  if (left_file_length > sample->length) left_file_length = sample->length;
  right_file_length = sl->right_file_length;
  if (right_file_length > sample->length) right_file_length = sample->length;

  if (sample->bits == 16)
  {
    int16_t *left = sample->data16;
    int16_t *right = (int16_t*)(sample->data8 + right_offset);

    for (uint32_t i = 0; i < sample->length; i++)
    {
      int32_t left_val = 0;
      int32_t right_val = 0;

      if (i < left_file_length)
      {
        uint16_t raw = tracker_rd_le16((const uint8_t*)left + (size_t)i * 2);
        if (!signed_samples) raw ^= 0x8000;
        left_val = (int16_t)raw;
      }

      if (sl->channels == 2)
      {
        if (i < right_file_length)
        {
          uint16_t raw = tracker_rd_le16((const uint8_t*)right + (size_t)i * 2);
          if (!signed_samples) raw ^= 0x8000;
          right_val = (int16_t)raw;
        }
        left[i] = (int16_t)((left_val + right_val) / 2);
      }
      else
        left[i] = (int16_t)left_val;
    }
  }
  else
  {
    int8_t *left = sample->data8;
    int8_t *right = sample->data8 + right_offset;

    for (uint32_t i = 0; i < sample->length; i++)
    {
      int16_t left_val = 0;
      int16_t right_val = 0;

      if (i < left_file_length)
        left_val = signed_samples ? left[i] : (int8_t)((uint8_t)left[i] ^ 0x80);

      if (sl->channels == 2)
      {
        if (i < right_file_length)
          right_val = signed_samples ? right[i] : (int8_t)((uint8_t)right[i] ^ 0x80);
        left[i] = (int8_t)((left_val + right_val) / 2);
      }
      else
        left[i] = (int8_t)left_val;
    }
  }
}

// Called from the SPI slave command path. module_size != 0 starts a new stream;
// module_size == 0 is a continuation request after the parser returned NEED_MORE.
int xm_host_s3m_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size)
{
  XmHostStreamParser *parser = &g_xm_host_stream.parser;
  s3m_layout_t *layout;

  if (out_ctx) *out_ctx = NULL;
  if (out_used_size) *out_used_size = 0;
  if (!out_ctx || !out_used_size) return -1;

  if (parser->stage == TRACK_HOST_PARSE_IDLE)
  {
    memset(parser, 0, sizeof(*parser));
    parser->stage = TRACK_HOST_PARSE_S3M_READ_HEADER;
    g_xm_host_stream.err = ESP_OK;
  }

  while (1)
  {
    layout = parser->s3m_layout;

    switch (parser->stage)
    {
      case TRACK_HOST_PARSE_S3M_READ_HEADER:
      {
        if (!parser->s3m_layout)
        {
          parser->s3m_layout = (s3m_layout_t*)xm_malloc(sizeof(s3m_layout_t));
          if (!parser->s3m_layout) return -2;
        }

        int step = xm_host_stream_take_at(0, parser->s3m_header, S3M_HEADER_SIZE, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->read_done = 0;
        layout = parser->s3m_layout;
        if (!xm_host_s3m_apply_header(layout, parser->s3m_header, g_xm_host_stream.total_size)) return -3;

        parser->s3m_table_offset = S3M_HEADER_SIZE;
        parser->stage = TRACK_HOST_PARSE_S3M_READ_ORDERS;
      }
      break;

      case TRACK_HOST_PARSE_S3M_READ_ORDERS:
      {
        int step = xm_host_stream_take_at(parser->s3m_table_offset, layout->orders, layout->order_count, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->read_done = 0;
        parser->s3m_table_offset += layout->order_count;
        parser->sample_i = 0;
        parser->stage = TRACK_HOST_PARSE_S3M_READ_SAMPLE_PTRS;
      }
      break;

      case TRACK_HOST_PARSE_S3M_READ_SAMPLE_PTRS:
      {
        while (parser->sample_i < layout->sample_count)
        {
          int step = xm_host_stream_take_at(parser->s3m_table_offset + (size_t)parser->sample_i * 2, parser->mod_entry, 2, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;

          layout->sample_para[parser->sample_i] = tracker_rd_le16(parser->mod_entry);
          parser->read_done = 0;
          parser->sample_i++;
        }

        parser->s3m_table_offset += (size_t)layout->sample_count * 2;
        parser->pattern_i = 0;
        parser->stage = TRACK_HOST_PARSE_S3M_READ_PATTERN_PTRS;
      }
      break;

      case TRACK_HOST_PARSE_S3M_READ_PATTERN_PTRS:
      {
        while (parser->pattern_i < layout->pattern_count)
        {
          int step = xm_host_stream_take_at(parser->s3m_table_offset + (size_t)parser->pattern_i * 2, parser->mod_entry, 2, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;

          layout->pattern_para[parser->pattern_i] = tracker_rd_le16(parser->mod_entry);
          parser->read_done = 0;
          parser->pattern_i++;
        }

        parser->s3m_table_offset += (size_t)layout->pattern_count * 2;
        parser->stage = layout->has_panning_table ? TRACK_HOST_PARSE_S3M_READ_PANNING : TRACK_HOST_PARSE_S3M_READ_SAMPLE_HEADER;
        parser->sample_i = 0;
        if (!layout->has_panning_table && !xm_host_s3m_finish_tables(layout)) return -3;
      }
      break;

      case TRACK_HOST_PARSE_S3M_READ_PANNING:
      {
        int step = xm_host_stream_take_at(parser->s3m_table_offset, parser->s3m_panning, S3M_MAX_CHANNELS, &parser->read_done);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->read_done = 0;
        for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
        {
          uint8_t pan = parser->s3m_panning[i];
          if (pan & 0x20) layout->channel_panning[i] = pan & 0x0F;
        }

        if (!xm_host_s3m_finish_tables(layout)) return -3;
        parser->sample_i = 0;
        parser->stage = TRACK_HOST_PARSE_S3M_READ_SAMPLE_HEADER;
      }
      break;

      case TRACK_HOST_PARSE_S3M_READ_SAMPLE_HEADER:
      {
        while (parser->sample_i < layout->sample_count)
        {
          size_t header_offset = s3m_para_offset(layout->sample_para[parser->sample_i]);

          if (!layout->sample_para[parser->sample_i])
          {
            parser->sample_i++;
            continue;
          }

          if (header_offset > g_xm_host_stream.total_size || g_xm_host_stream.total_size - header_offset < S3M_SAMPLE_HEADER_SIZE)
          {
            s3m_set_error_fmt("S3M sample %u header outside file", parser->sample_i + 1);
            return -3;
          }

          int step = xm_host_stream_take_at(header_offset, parser->s3m_sample_header, S3M_SAMPLE_HEADER_SIZE, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;

          parser->read_done = 0;
          if (!xm_host_s3m_apply_sample_header(layout, parser->sample_i, parser->s3m_sample_header, g_xm_host_stream.total_size)) return -3;
          parser->sample_i++;
        }

        parser->stage = TRACK_HOST_PARSE_S3M_ALLOC_CONTEXT;
      }
      break;

      case TRACK_HOST_PARSE_S3M_ALLOC_CONTEXT:
      {
        if (!s3m_get_group_sizes_for_layout(layout, &parser->group_sizes)) return -2;
        parser->used_size = parser->group_sizes.total_size;
        if (!parser->used_size || parser->used_size > INT_MAX) return -2;

        if (!xm_allocate_context_groups(&parser->ctx, &parser->cursor, &parser->group_sizes, xm_malloc)) return -2;
        parser->arena = NULL;
        parser->arena_size = 0;
        parser->mod = &parser->ctx->module;

        if (!s3m_setup_module_header_grouped(parser->ctx, layout, &parser->cursor)) return -3;

        parser->pattern_i = 0;
        parser->pat = NULL;
        parser->read_done = 0;
        parser->stage = TRACK_HOST_PARSE_S3M_LOAD_PATTERN_DATA;
      }
      break;

      case TRACK_HOST_PARSE_S3M_LOAD_PATTERN_DATA:
      {
        if (parser->pattern_i >= parser->mod->num_patterns)
        {
          s3m_apply_post_pattern_compat(parser->ctx, layout);
          parser->sample_i = 0;
          parser->sample = NULL;
          parser->sample_data_started = 0;
          parser->read_done = 0;
          parser->stage = TRACK_HOST_PARSE_S3M_LOAD_SAMPLE_DATA;
          break;
        }

        if (!parser->pat)
        {
          size_t slots_size = (size_t)S3M_ROWS_PER_PATTERN * parser->mod->num_channels * sizeof(xm_pattern_slot_t);

          parser->pat = parser->mod->patterns + parser->pattern_i;
          parser->pat->num_rows = S3M_ROWS_PER_PATTERN;
          parser->pat->slots = slots_size ? (xm_pattern_slot_t*)xm_group_cursor_alloc(&parser->cursor.patterns, parser->cursor.patterns_end, slots_size, true) : NULL;
          if (slots_size && !parser->pat->slots) return -3;

          parser->packed_pattern_size = 0;
          parser->packed_pattern_soft_size = 0;
          parser->pattern_packed_pos = 2;
          parser->pattern_event_pos = 2;
          parser->s3m_pattern_row = 0;
          parser->pattern_state = XM_HOST_PATTERN_READ_NOTE;
          parser->pattern_field_idx = 0;
          parser->read_done = 0;
        }

        if (layout->pattern_para[parser->pattern_i] && !parser->packed_pattern_size)
        {
          size_t pattern_offset = s3m_para_offset(layout->pattern_para[parser->pattern_i]);

          if (pattern_offset > g_xm_host_stream.total_size || g_xm_host_stream.total_size - pattern_offset < 2)
          {
            s3m_set_error_fmt("S3M pattern %u offset outside file", parser->pattern_i);
            return -3;
          }

          int step = xm_host_stream_take_at(pattern_offset, parser->mod_entry, 2, &parser->read_done);
          if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
          if (step != TRACK_HOST_STEP_OK) return -3;

          {
            uint16_t packed_len = tracker_rd_le16(parser->mod_entry);
            size_t remaining = g_xm_host_stream.total_size - pattern_offset;

            parser->read_done = 0;
            if (packed_len < 2)
            {
              parser->packed_pattern_soft_size = 2;
              parser->packed_pattern_size = 2;
            }
            else
            {
              parser->packed_pattern_soft_size = packed_len;
              parser->packed_pattern_size = (size_t)packed_len + 2;
              for (uint16_t i = 0; i < layout->pattern_count; i++)
              {
                size_t next_offset;

                if (i == parser->pattern_i || !layout->pattern_para[i]) continue;
                next_offset = s3m_para_offset(layout->pattern_para[i]);
                if (next_offset > pattern_offset && next_offset - pattern_offset < remaining) remaining = next_offset - pattern_offset;
              }
              if (parser->packed_pattern_soft_size > remaining) parser->packed_pattern_soft_size = remaining;
              if (parser->packed_pattern_size > remaining) parser->packed_pattern_size = remaining;
            }
          }
        }

        int step = xm_host_s3m_decode_pattern_stream(parser);
        if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
        if (step != TRACK_HOST_STEP_OK) return -3;

        parser->pattern_i++;
        parser->pat = NULL;
        parser->stage = TRACK_HOST_PARSE_S3M_LOAD_PATTERN_DATA;
      }
      break;

      case TRACK_HOST_PARSE_S3M_LOAD_SAMPLE_DATA:
      {
        if (parser->sample_i >= layout->sample_count)
        {
          parser->stage = TRACK_HOST_PARSE_S3M_ALLOC_RUNTIME;
          break;
        }

        if (!parser->sample_data_started)
        {
          const s3m_sample_layout_t *sl = layout->samples + parser->sample_i;

          if (!s3m_setup_sample_metadata_grouped(parser->ctx, layout, parser->sample_i, &parser->cursor, xm_malloc)) return -3;
          parser->sample = parser->mod->instruments[parser->sample_i].samples;
          parser->s3m_sample_bytes = sl->file_length;
          if ((sl->flags & S3M_SAMPLE_FLAG_16BIT) && !tracker_size_mul(parser->s3m_sample_bytes, 2, &parser->s3m_sample_bytes)) return -3;
          if (sl->channels == 2 && sl->file_length == sl->length)
          {
            size_t right_bytes = sl->right_file_length;
            size_t left_storage_bytes = sl->length;

            if ((sl->flags & S3M_SAMPLE_FLAG_16BIT) && !tracker_size_mul(right_bytes, 2, &right_bytes)) return -3;
            if ((sl->flags & S3M_SAMPLE_FLAG_16BIT) && !tracker_size_mul(left_storage_bytes, 2, &left_storage_bytes)) return -3;
            parser->s3m_sample_bytes = left_storage_bytes;
            if (!tracker_size_add(&parser->s3m_sample_bytes, right_bytes)) return -3;
          }
          parser->sample_pos = 0;
          parser->sample_data_started = 1;
        }

        if (parser->sample)
        {
          const s3m_sample_layout_t *sl = layout->samples + parser->sample_i;

          if (parser->s3m_sample_bytes)
          {
            int step = xm_host_stream_take_at(sl->data_offset, parser->sample->data8, parser->s3m_sample_bytes, &parser->sample_pos);
            if (step == TRACK_HOST_STEP_NEED_MORE) return TRACK_HOST_BUILD_NEED_MORE;
            if (step != TRACK_HOST_STEP_OK) return -3;
          }

          xm_host_s3m_convert_sample_data(parser->sample, sl, layout->signed_samples != 0);
        }

        parser->sample_i++;
        parser->sample_data_started = 0;
        parser->sample = NULL;
        parser->stage = TRACK_HOST_PARSE_S3M_LOAD_SAMPLE_DATA;
      }
      break;

      case TRACK_HOST_PARSE_S3M_ALLOC_RUNTIME:
      {
        if (!s3m_setup_context_runtime_grouped(parser->ctx, layout, &parser->cursor, XM_SAMPLE_RATE)) return -3;

        parser->ctx->ctx_size = parser->used_size;

        int ret = s3m_finish_context(&parser->ctx, parser->ctx);
        if (ret < 0) return -3;

        if (parser->s3m_layout)
        {
          heap_caps_free(parser->s3m_layout);
          parser->s3m_layout = NULL;
        }

        parser->stage = TRACK_HOST_PARSE_S3M_DONE;
      }
      break;

      case TRACK_HOST_PARSE_S3M_DONE:
      {
        *out_ctx = parser->ctx;
        *out_used_size = parser->used_size;
        return (int)parser->used_size;
      }

      default:
        return -4;
    }
  }
}

int tracker_stream_format_is_valid(int format)
{
  return format == TRACK_HOST_STREAM_FORMAT_XM || format == TRACK_HOST_STREAM_FORMAT_MOD || format == TRACK_HOST_STREAM_FORMAT_S3M;
}

int tracker_stream_obj_type(int format)
{
  if (format == TRACK_HOST_STREAM_FORMAT_MOD) return OBJ_TYPE_MDC;
  if (format == TRACK_HOST_STREAM_FORMAT_S3M) return OBJ_TYPE_S3C;
  return OBJ_TYPE_XMC;
}

int tracker_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size)
{
  if (g_xm_host_stream.format == TRACK_HOST_STREAM_FORMAT_MOD)
    return xm_host_mod_stream_build_context(out_ctx, out_used_size);

  if (g_xm_host_stream.format == TRACK_HOST_STREAM_FORMAT_S3M)
    return xm_host_s3m_stream_build_context(out_ctx, out_used_size);

  return xm_host_stream_build_context(out_ctx, out_used_size);
}

void tracker_stream_free_pending_context()
{
  if (g_xm_host_stream.parser.ctx)
  {
    xm_free_context(g_xm_host_stream.parser.ctx);
    g_xm_host_stream.parser.ctx = NULL;
    g_xm_host_stream.parser.arena = NULL;
  }
}

esp_err_t tracker_stream_begin(size_t module_size, int format)
{
  if (!tracker_stream_format_is_valid(format)) return ESP_ERR_INVALID_ARG;
  if (!module_size || module_size > INT_MAX) return ESP_ERR_INVALID_SIZE;
  if (g_xm_host_stream.active) return ESP_ERR_INVALID_STATE;

  xm_host_stream_clear_state();
  g_xm_host_stream.active = true;
  g_xm_host_stream.format = format;
  g_xm_host_stream.total_size = module_size;
  g_xm_host_stream.handle = -1;
  g_xm_host_stream.err = ESP_OK;
  memset(g_xm_host_stream.parser.header, 0, sizeof(g_xm_host_stream.parser.header));
  return ESP_OK;
}

esp_err_t tracker_stream_request_chunk(size_t rx_size, TrackerStreamChunkRequest *request)
{
  size_t left;
  size_t max_rx_size;

  if (!request) return ESP_ERR_INVALID_ARG;
  memset(request, 0, sizeof(*request));

  if (!rx_size) return ESP_ERR_INVALID_SIZE;
  if (!g_xm_host_stream.active || g_xm_host_stream.waiting_rx) return ESP_ERR_INVALID_STATE;
  if (g_xm_host_stream.pos && !g_xm_host_stream.need_chunk) return ESP_ERR_INVALID_STATE;

  if (g_xm_host_stream.pos >= g_xm_host_stream.total_size)
  {
    xm_host_stream_abort_current();
    return ESP_ERR_INVALID_SIZE;
  }

  left = g_xm_host_stream.total_size - g_xm_host_stream.pos;
  max_rx_size = (left + 3) & ~((size_t)3);
  if (!max_rx_size || rx_size > max_rx_size)
  {
    xm_host_stream_abort_current();
    return ESP_ERR_INVALID_SIZE;
  }

  g_xm_host_stream.requested_size = rx_size;
  g_xm_host_stream.logical_size = rx_size > left ? left : rx_size;
  g_xm_host_stream.waiting_rx = true;
  g_xm_host_stream.need_chunk = false;

  request->offset = g_xm_host_stream.pos;
  request->rx_size = rx_size;
  request->logical_size = g_xm_host_stream.logical_size;
  return ESP_OK;
}

int tracker_stream_push_chunk(const u8 *data, size_t size, TrackerStreamPushResult *result)
{
  xm_context_t *ctx = NULL;
  size_t used_size = 0;
  int rc;

  if (result) memset(result, 0, sizeof(*result));

  if (!g_xm_host_stream.active || !g_xm_host_stream.waiting_rx || !data || !size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_STATE;
    return -4;
  }

  if (size != g_xm_host_stream.requested_size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    g_xm_host_stream.waiting_rx = false;
    g_xm_host_stream.chunk_active = false;
    xm_host_stream_abort_current();
    return -3;
  }

  g_xm_host_stream.chunk_data = data;
  g_xm_host_stream.chunk_size = g_xm_host_stream.logical_size;
  g_xm_host_stream.chunk_pos = 0;
  g_xm_host_stream.chunk_active = true;
  g_xm_host_stream.waiting_rx = false;

  rc = tracker_stream_build_context(&ctx, &used_size);

  if (rc == TRACK_HOST_BUILD_NEED_MORE)
  {
    xm_host_stream_release_chunk();

    if (g_xm_host_stream.pos >= g_xm_host_stream.total_size)
    {
      tracker_stream_free_pending_context();
      xm_host_stream_free_parser_buffers();
      g_xm_host_stream.active = false;
      g_xm_host_stream.need_chunk = false;
      g_xm_host_stream.waiting_rx = false;
      g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
      return -3;
    }

    g_xm_host_stream.need_chunk = true;
    return TRACK_HOST_BUILD_NEED_MORE;
  }

  if (rc < 0)
  {
    xm_host_stream_release_chunk();
    tracker_stream_free_pending_context();
    xm_host_stream_free_parser_buffers();
    g_xm_host_stream.active = false;
    g_xm_host_stream.need_chunk = false;
    g_xm_host_stream.waiting_rx = false;
    return rc;
  }

  if (result)
  {
    result->ctx = ctx;
    result->used_size = used_size;
    result->obj_type = tracker_stream_obj_type(g_xm_host_stream.format);
    result->build_rc = rc;
  }

  return TRACK_HOST_STEP_OK;
}

void tracker_stream_finish_success()
{
  g_xm_host_stream.parser.ctx = NULL;
  g_xm_host_stream.parser.arena = NULL;
  xm_host_stream_free_parser_buffers();
  xm_host_stream_release_chunk();
  g_xm_host_stream.active = false;
  g_xm_host_stream.need_chunk = false;
  g_xm_host_stream.waiting_rx = false;
}

int tracker_file_stream_format(tracker_file_format_t format)
{
  if (format == TRACK_FILE_FORMAT_MOD) return TRACK_HOST_STREAM_FORMAT_MOD;
  if (format == TRACK_FILE_FORMAT_S3M) return TRACK_HOST_STREAM_FORMAT_S3M;
  return TRACK_HOST_STREAM_FORMAT_XM;
}

void tracker_stream_set_context_info(int handle, int obj_type, const char *path, size_t file_size, xm_context_t *ctx)
{
  if (obj_type == OBJ_TYPE_MDC)
  {
    xm_set_mod_context_info(handle, path, file_size, ctx);
    return;
  }

  if (obj_type == OBJ_TYPE_S3C)
  {
    xm_set_s3m_context_info(handle, path, file_size, ctx);
    return;
  }

  xm_host_stream_set_info(handle, ctx, file_size);
  if (path)
  {
    strncpy(xm_info[handle].path, path, sizeof(xm_info[handle].path) - 1);
    xm_info[handle].path[sizeof(xm_info[handle].path) - 1] = 0;
  }
}

int tracker_stream_load_file_to_handle(const char *path, size_t size, tracker_file_format_t format, int *out_handle, bool quiet)
{
  XmStreamReader reader;
  TrackerStreamChunkRequest request;
  TrackerStreamPushResult result;
  u8 *chunk = NULL;
  size_t chunk_size;
  int stream_format;
  int handle = -1;
  int rc;
  esp_err_t err;

  if (out_handle) *out_handle = -1;
  if (!path || !path[0] || !size || size > INT_MAX) return 1;

  stream_format = tracker_file_stream_format(format);
  chunk_size = size < TRACKER_FILE_STREAM_CHUNK_SIZE ? size : TRACKER_FILE_STREAM_CHUNK_SIZE;
  chunk = (u8*)malloc_spiram(chunk_size);
  if (!chunk)
  {
    if (!quiet)
      printf("E: %s stream chunk alloc failed: %u bytes\r\n", xm_file_format_str(format), (unsigned)chunk_size);
    return 1;
  }

  err = xm_stream_reader_open(&reader, path, size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s stream open failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    free(chunk);
    return 1;
  }

  err = tracker_stream_begin(size, stream_format);
  if (err != ESP_OK)
  {
    if (!quiet)
    {
      if (err == ESP_ERR_NO_MEM)
      {
        size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        printf("E: %s stream alloc precheck failed: size=%u free=%u largest=%u\r\n",
          xm_file_format_str(format),
          (unsigned)size,
          (unsigned)free_size,
          (unsigned)largest);
      }
      else
        printf("E: %s stream begin failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    }
    reader.close(&reader);
    free(chunk);
    return 1;
  }

  while (1)
  {
    size_t left;
    size_t rx_size;

    if (g_xm_host_stream.pos >= g_xm_host_stream.total_size)
    {
      if (!quiet) printf("E: %s stream requested data past EOF\r\n", xm_file_format_str(format));
      xm_host_stream_abort_current();
      reader.close(&reader);
      free(chunk);
      return 1;
    }

    left = g_xm_host_stream.total_size - g_xm_host_stream.pos;
    rx_size = left < chunk_size ? left : chunk_size;

    err = tracker_stream_request_chunk(rx_size, &request);
    if (err != ESP_OK)
    {
      if (!quiet) printf("E: %s stream request failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
      xm_host_stream_abort_current();
      reader.close(&reader);
      free(chunk);
      return 1;
    }

    if (reader.pos != request.offset)
    {
      err = xm_stream_reader_seek(&reader, request.offset);
      if (err != ESP_OK)
      {
        if (!quiet) printf("E: %s stream seek failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
        xm_host_stream_abort_current();
        reader.close(&reader);
        free(chunk);
        return 1;
      }
    }

    err = xm_stream_reader_read_exact(&reader, chunk, request.logical_size);
    if (err != ESP_OK)
    {
      if (!quiet) printf("E: %s stream read failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
      xm_host_stream_abort_current();
      reader.close(&reader);
      free(chunk);
      return 1;
    }

    rc = tracker_stream_push_chunk(chunk, request.rx_size, &result);
    if (rc == TRACK_HOST_BUILD_NEED_MORE)
      continue;

    if (rc < 0)
    {
      if (!quiet)
      {
        if (rc == -2)
        {
          size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
          size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
          printf("E: %s stream context alloc failed: free=%u largest=%u\r\n",
            xm_file_format_str(format),
            (unsigned)free_size,
            (unsigned)largest);
        }
        else if (stream_format == TRACK_HOST_STREAM_FORMAT_S3M)
          printf("E: invalid S3M stream: %s\r\n", s3m_get_last_error());
        else
          printf("E: invalid %s stream\r\n", xm_file_format_str(format));
      }
      xm_host_stream_abort_current();
      reader.close(&reader);
      free(chunk);
      return 1;
    }

    handle = attach_obj(result.ctx, (int)result.used_size, result.obj_type);
    if (handle < 0)
    {
      xm_free_context(result.ctx);
      tracker_stream_finish_success();
      reader.close(&reader);
      free(chunk);
      return 1;
    }

    tracker_stream_set_context_info(handle, result.obj_type, path, size, result.ctx);
    xm_reset_context_state(result.ctx, handle);
    mem_obj[handle].state = TRACK_OBJ_ST_STOPPED;
    xm_reset_render_stats();
    tracker_stream_finish_success();
    reader.close(&reader);
    free(chunk);

    if (!quiet)
    {
      printf("%s stream loaded: handle=%02X size=%u ctx=%u\r\n",
        xm_file_format_str(format),
        handle,
        (unsigned)size,
        (unsigned)mem_obj[handle].size);
      if (xm_info[handle].module_name[0])
        printf("  Module : %s\r\n", xm_info[handle].module_name);
      if (xm_info[handle].tracker_name[0])
        printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);
    }

    if (out_handle) *out_handle = handle;
    return 0;
  }
}

esp_err_t xm_host_stream_prepare_command_format(size_t module_size, size_t rx_size, int format)
{
  TrackerStreamChunkRequest request;
  esp_err_t err;

  if (!tracker_stream_format_is_valid(format))
  {
    set_status(ESP_ERR_INV_STATE);
    return ESP_ERR_INVALID_ARG;
  }

  if (!rx_size || rx_size > DMA_BUF_SIZE)
  {
    set_status(ESP_ERR_INV_SIZE);
    return ESP_ERR_INVALID_SIZE;
  }

  if (module_size)
  {
    err = tracker_stream_begin(module_size, format);
    if (err != ESP_OK)
    {
      if (err == ESP_ERR_NO_MEM)
        set_status(ESP_ERR_OUT_OF_MEMORY);
      else if (err == ESP_ERR_INVALID_SIZE)
        set_status(ESP_ERR_INV_SIZE);
      else
      {
        ESP_LOGW("xm_stream",
                "prepare first failed: module_size=%u rx_size=%u active=%u need=%u waiting=%u pos=%u total=%u err=%s",
                (unsigned)module_size,
                (unsigned)rx_size,
                g_xm_host_stream.active,
                g_xm_host_stream.need_chunk,
                g_xm_host_stream.waiting_rx,
                (unsigned)g_xm_host_stream.pos,
                (unsigned)g_xm_host_stream.total_size,
                esp_err_to_name(err));
        set_status(ESP_ERR_INV_STATE);
      }
      return err;
    }
  }
  else
  {
    if (!g_xm_host_stream.active || !g_xm_host_stream.need_chunk || g_xm_host_stream.waiting_rx || g_xm_host_stream.format != format)
    {
      ESP_LOGW("xm_stream",
              "prepare bad continuation: module_size=%u rx_size=%u active=%u need=%u waiting=%u pos=%u total=%u",
              (unsigned)module_size,
              (unsigned)rx_size,
              g_xm_host_stream.active,
              g_xm_host_stream.need_chunk,
              g_xm_host_stream.waiting_rx,
              (unsigned)g_xm_host_stream.pos,
              (unsigned)g_xm_host_stream.total_size);

      set_status(ESP_ERR_INV_STATE);
      return ESP_ERR_INVALID_STATE;
    }
  }

  err = tracker_stream_request_chunk(rx_size, &request);
  if (err != ESP_OK)
  {
    if (err == ESP_ERR_INVALID_SIZE)
      set_status(ESP_ERR_INV_SIZE);
    else
      set_status(ESP_ERR_INV_STATE);
    return err;
  }

  wr_reg32(ESP_REG_DATA_OFFSET, request.offset);
  wr_reg32(ESP_REG_DATA_SIZE, request.rx_size);
  put_rxq(DREQ_TRACK_STREAM);
  return ESP_OK;
}

esp_err_t xm_host_stream_prepare_command(size_t module_size, size_t rx_size)
{
  return xm_host_stream_prepare_command_format(module_size, rx_size, TRACK_HOST_STREAM_FORMAT_XM);
}

esp_err_t mod_host_stream_prepare_command(size_t module_size, size_t rx_size)
{
  return xm_host_stream_prepare_command_format(module_size, rx_size, TRACK_HOST_STREAM_FORMAT_MOD);
}

esp_err_t s3m_host_stream_prepare_command(size_t module_size, size_t rx_size)
{
  return xm_host_stream_prepare_command_format(module_size, rx_size, TRACK_HOST_STREAM_FORMAT_S3M);
}

// RX callback after one host DMA chunk. The parser runs inline here and returns
// READY only when it has consumed the chunk or completed the module.
void xm_host_stream_process_rx_data(const u8 *data, size_t size)
{
  TrackerStreamPushResult result;
  int rc;

  set_status(ESP_ST_BUSY);

  rc = tracker_stream_push_chunk(data, size, &result);
  if (rc == TRACK_HOST_BUILD_NEED_MORE)
  {
    set_status(ESP_ST_READY);
    return;
  }

  if (rc < 0)
  {
    if (rc == -2)
      set_status(ESP_ERR_OUT_OF_MEMORY);
    else if (rc == -4)
      set_status(ESP_ERR_INV_STATE);
    else if (g_xm_host_stream.format == TRACK_HOST_STREAM_FORMAT_MOD)
      set_status(ESP_ERR_INV_MOD);
    else if (g_xm_host_stream.format == TRACK_HOST_STREAM_FORMAT_S3M)
    {
      ESP_LOGE("xm_stream", "%s", s3m_get_last_error());
      set_status(ESP_ERR_INV_S3M);
    }
    else if (g_xm_host_stream.err == ESP_ERR_INVALID_SIZE)
      set_status(ESP_ERR_INV_SIZE);
    else
      set_status(ESP_ERR_INV_XM);
    return;
  }

  // From this point attach_obj owns ctx on success. Clear parser.ctx after attach
  // so abort/error paths do not free the live object.
  int handle = attach_obj(result.ctx, (int)result.used_size, result.obj_type);
  if (handle < 0)
  {
    xm_free_context(result.ctx);
    tracker_stream_finish_success();
    set_status(ESP_ERR_OUT_OF_MEMORY);
    return;
  }

  if (result.obj_type == OBJ_TYPE_MDC)
    xm_host_mod_stream_set_info(handle, result.ctx, g_xm_host_stream.total_size);
  else if (result.obj_type == OBJ_TYPE_S3C)
    xm_set_s3m_context_info(handle, "host-stream", g_xm_host_stream.total_size, result.ctx);
  else
    xm_host_stream_set_info(handle, result.ctx, g_xm_host_stream.total_size);
  xm_reset_context_state(result.ctx, handle);
  mem_obj[handle].state = TRACK_OBJ_ST_STOPPED;
  xm_reset_render_stats();
  wr_reg8(ESP_REG_OBJ_HANDLE, handle);
  wr_reg32(ESP_REG_DATA_SIZE, result.used_size);
  g_xm_host_stream.handle = handle;
  tracker_stream_finish_success();
  set_status(ESP_ST_READY);
}

// -------------------- XM helpers --------------------

void xm_reset_render_stats()
{
  stats::reset_audio_render();
}

void xm_reset_context_state(xm_context_t *ctx, int handle)
{
  if (!ctx) return;

  mod_restore_efx_backups(ctx);

  uint16_t tempo = ctx->tempo;
  uint16_t bpm = ctx->bpm;

  if (handle >= 0 && handle < OBJ_HANDLES_MAX && xm_info[handle].valid)
  {
    tempo = xm_info[handle].tempo;
    bpm = xm_info[handle].bpm;
  }

  ctx->tempo = tempo;
  ctx->bpm = bpm;
  ctx->global_volume = 1.f;
  ctx->amplification = .25f;

#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  ctx->current_table_index = 0;
  ctx->current_row = 0;
  ctx->current_tick = 0;
  ctx->remaining_samples_in_tick = 0;
  ctx->generated_samples = 0;
  ctx->position_jump = false;
  ctx->pattern_break = false;
  ctx->jump_dest = 0;
  ctx->jump_row = 0;
  ctx->extra_ticks = 0;
  ctx->loop_count = 0;
  ctx->max_loop_count = 0;

  if (ctx->row_loop_count)
    memset(ctx->row_loop_count, 0, ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t));

  for (uint8_t i = 0; i < ctx->module.num_channels; i++)
  {
    xm_channel_context_t *ch = ctx->channels + i;
    float default_panning = ch->default_panning;
    if (default_panning < .0f) default_panning = -1.f;
    memset(ch, 0, sizeof(*ch));

    ch->ping = true;
    ch->vibrato_waveform = XM_SINE_WAVEFORM;
    ch->vibrato_waveform_retrigger = true;
    ch->tremolo_waveform = XM_SINE_WAVEFORM;
    ch->tremolo_waveform_retrigger = true;
    ch->panbrello_waveform = XM_SINE_WAVEFORM;
    ch->panbrello_waveform_retrigger = true;

    ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
    ch->channel_volume = 1.0f;
    ch->default_panning = default_panning;
    ch->tracker_format = ctx->tracker_format;
    ch->period_note_offset = .0f;
    if (ch->default_panning >= .0f)
      ch->panning = ch->actual_panning = ch->default_panning;
    else
      ch->panning = ch->actual_panning = .5f;
    ch->panning_envelope_panning = .5f;
    ch->actual_volume = .0f;
#if XM_RAMPING
    ch->target_volume = .0f;
    ch->target_panning = ch->actual_panning;
#endif
  }

  for (uint16_t i = 0; i < ctx->module.num_instruments; i++)
  {
    xm_instrument_t *instr = ctx->module.instruments + i;
    instr->latest_trigger = 0;
    instr->muted = false;

    for (uint16_t j = 0; j < instr->num_samples; j++)
      instr->samples[j].latest_trigger = 0;
  }
}

void xm_stop_player_sync(int handle)
{
  PLAYER_TASK t = {};
  t.task = PLAYER_STOP;
  t.ctx = NULL;
  int ack;

  while (xQueueReceive(player_ack_queue, &ack, 0) == pdTRUE);
  xQueueSend(player_queue, &t, portMAX_DELAY);
  xQueueReceive(player_ack_queue, &ack, portMAX_DELAY);

  if (!mem_obj[handle].addr || mem_obj[handle].state != TRACK_OBJ_ST_PLAYING)
    handle = curr_xm_handle;

  if (handle >= 0 && handle < OBJ_HANDLES_MAX && mem_obj[handle].addr)
  {
    xm_context_t *ctx = (xm_context_t*)mem_obj[handle].addr;
    if (s3m_context_uses_adlib(ctx))
    {
      s3m_adlib_all_notes_off(ctx);
      opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
    }
    mem_obj[handle].state = TRACK_OBJ_ST_STOPPED;
  }
}

void *xm_malloc(size_t size)
{
  void *ctx_mem = malloc_spiram(size);

  if (!ctx_mem)
    ESP_LOGE("xm_malloc", "Cannot allocate memory for XM context (%u bytes)!", size);

#ifdef VERBOSE
  else
    printf("Memory for XM allocated: 0x%08X, %u bytes\r\n", (unsigned int)ctx_mem, size);
#endif

  return ctx_mem;
}

u16 xm_rd_le16(const u8 *p)
{
  return (u16)p[0] | ((u16)p[1] << 8);
}

u32 xm_rd_le32(const u8 *p)
{
  return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

void xm_copy_trimmed(char *dst, size_t dst_size, const u8 *src, size_t src_size)
{
  if (!dst || !dst_size) return;

  size_t n = src_size;
  while (n && (src[n - 1] == 0 || src[n - 1] == ' ')) n--;
  if (n >= dst_size) n = dst_size - 1;

  if (n) memcpy(dst, src, n);
  dst[n] = 0;
}

void xm_clear_info(int handle)
{
  memset(&xm_info[handle], 0, sizeof(xm_info[handle]));
}

int xm_parse_info(int handle, const char *path, const u8 *data, size_t size)
{
  if (!data || size < 80) return 0;
  if (memcmp(data, "Extended Module: ", 17)) return 0;
  if (data[37] != 0x1A) return 0;

  XM_INFO *info = &xm_info[handle];
  memset(info, 0, sizeof(*info));

  info->valid = 1;
  info->version = xm_rd_le16(data + 58);
  info->header_size = xm_rd_le32(data + 60);
  info->song_length = xm_rd_le16(data + 64);
  info->restart_position = xm_rd_le16(data + 66);
  info->num_channels = xm_rd_le16(data + 68);
  info->num_patterns = xm_rd_le16(data + 70);
  info->num_instruments = xm_rd_le16(data + 72);
  info->flags = xm_rd_le16(data + 74);
  info->tempo = xm_rd_le16(data + 76);
  info->bpm = xm_rd_le16(data + 78);
  info->file_size = (u32)size;

  xm_copy_trimmed(info->module_name, sizeof(info->module_name), data + 17, 20);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), data + 38, 20);

  if (path)
  {
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = 0;
  }

  return 1;
}

const char *xm_obj_state_str(u8 st)
{
  switch (st)
  {
    case TRACK_OBJ_ST_STOPPED: return "Stopped";
    case TRACK_OBJ_ST_PLAYING: return "Playing";
    case OBJ_ST_NONE:       return "None";
    case OBJ_ST_ERROR:      return "Error";
    default:                return "Unknown";
  }
}

const char *xm_obj_type_str(u8 type)
{
  switch (type)
  {
    case OBJ_TYPE_XM:  return "XM";
    case OBJ_TYPE_XMC: return "XMC";
    case OBJ_TYPE_MOD: return "MOD";
    case OBJ_TYPE_MDC: return "MDC";
    case OBJ_TYPE_S3M: return "S3M";
    case OBJ_TYPE_S3C: return "S3C";
    default:           return "Other";
  }
}

int xm_find_playing_handle()
{
  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (mem_obj[i].addr && (mem_obj[i].type == OBJ_TYPE_XMC || mem_obj[i].type == OBJ_TYPE_MDC || mem_obj[i].type == OBJ_TYPE_S3C) && mem_obj[i].state == TRACK_OBJ_ST_PLAYING)
      return i;

  return -1;
}

int xm_wait_for_state(int handle, u8 state, int timeout_ms)
{
  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    if (mem_obj[handle].addr && mem_obj[handle].state == state)
      return 1;

    u8 st = rd_reg8(ESP_REG_STATUS);
    if (st >= 0x80) return 0;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return 0;
}

int xm_wait_for_status_ready(int timeout_ms)
{
  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    u8 st = rd_reg8(ESP_REG_STATUS);
    if (st == ESP_ST_READY)
      return 1;
    if (st >= 0x80)
      return 0;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return 0;
}

int xm_wait_for_init(int handle, int timeout_ms)
{
  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    if (mem_obj[handle].addr && (mem_obj[handle].type == OBJ_TYPE_XMC || mem_obj[handle].type == OBJ_TYPE_MDC || mem_obj[handle].type == OBJ_TYPE_S3C) && mem_obj[handle].state == TRACK_OBJ_ST_STOPPED)
      return 1;

    u8 st = rd_reg8(ESP_REG_STATUS);
    if (st >= 0x80) return 0;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return 0;
}

// Console-side stop waits on object state, but the actual stop barrier is in
// xm_task/player_task via player_ack_queue.
int xm_stop_current_playback(bool quiet)
{
  int handle = xm_find_playing_handle();
  if (handle < 0) return 0;

  TRACK_TASK task = {};
  task.task = TRACK_TASK_STOP;
  task.handle = handle;
  set_status(ESP_ST_BUSY);
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, TRACK_OBJ_ST_STOPPED, 2000))
  {
    if (!quiet) ESP_LOGE("xm_stop_current_playback", "XM stop failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  return 0;
}

int xm_parse_handle_arg(const char *s, int *out_handle)
{
  if (!s || !out_handle) return 0;

  char *endp = NULL;
  unsigned long v = strtoul(s, &endp, 0);
  if (!endp || *endp || v >= OBJ_HANDLES_MAX) return 0;

  *out_handle = (int)v;
  return 1;
}

// Delete path always stops playback first so delete_obj() never frees a context
// still referenced by player_task.
int xm_delete_all_modules(bool quiet)
{
  if (xm_stop_current_playback(quiet)) return 1;

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
  {
    if (!mem_obj[i].addr) continue;
    if (mem_obj[i].type != OBJ_TYPE_XM && mem_obj[i].type != OBJ_TYPE_XMC && mem_obj[i].type != OBJ_TYPE_MOD && mem_obj[i].type != OBJ_TYPE_MDC && mem_obj[i].type != OBJ_TYPE_S3M && mem_obj[i].type != OBJ_TYPE_S3C) continue;

    if (!delete_obj(i))
    {
      if (!quiet) printf("Delete failed for handle %02X\r\n", i);
      return 1;
    }

    if (curr_xm_handle == i) curr_xm_handle = -1;
    xm_clear_info(i);
  }

  return 0;
}

int xm_init_handle(int handle, bool quiet)
{
  TRACK_TASK task = {};

  if (!mem_obj[handle].addr || (mem_obj[handle].type != OBJ_TYPE_XM && mem_obj[handle].type != OBJ_TYPE_MOD && mem_obj[handle].type != OBJ_TYPE_S3M))
  {
    if (!quiet) printf("Handle %02X is not a raw module object\r\n", handle);
    return 1;
  }

  task.task = TRACK_TASK_INIT;
  task.handle = handle;
  set_status(ESP_ST_BUSY);
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_init(handle, 3000))
  {
    if (!quiet) ESP_LOGE("xm_init_handle", "Module init failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    delete_obj(handle);
    xm_clear_info(handle);
    return 1;
  }

  return 0;
}

// -------------------- XM stream parser --------------------

esp_err_t xm_stream_file_read(XmStreamReader *reader, void *dst, size_t size, size_t *out_size)
{
  size_t got;

  if (out_size) *out_size = 0;
  if (!reader || !reader->fp || (!dst && size)) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  got = fread(dst, 1, size, reader->fp);
  reader->pos += got;
  if (out_size) *out_size = got;

  if (got != size && ferror(reader->fp))
  {
    reader->err = ESP_FAIL;
    return ESP_FAIL;
  }

  return ESP_OK;
}

void xm_stream_file_close(XmStreamReader *reader)
{
  if (!reader) return;

  if (reader->fp)
  {
    fclose(reader->fp);
    reader->fp = NULL;
  }

  if (reader->sd_mounted)
  {
    sd_fs_unmount("/sd", NULL);
    reader->sd_mounted = false;
  }
}

esp_err_t xm_stream_reader_seek(XmStreamReader *reader, size_t offset)
{
  if (!reader || !reader->fp || offset > reader->file_size) return ESP_ERR_INVALID_ARG;

  if (fseek(reader->fp, (long)offset, SEEK_SET))
  {
    reader->err = ESP_FAIL;
    return reader->err;
  }

  reader->pos = offset;
  reader->err = ESP_OK;
  return ESP_OK;
}

esp_err_t xm_stream_reader_open(XmStreamReader *reader, const char *path, size_t file_size)
{
  bool was_sd_mounted;
  char full[XM_STREAM_PATH_MAX];

  if (!reader || !path || !path[0]) return ESP_ERR_INVALID_ARG;

  memset(reader, 0, sizeof(*reader));
  reader->err = ESP_OK;
  reader->read = xm_stream_file_read;
  reader->close = xm_stream_file_close;
  reader->file_size = file_size;

  reader->fp = xm_fopen_any_ext_case(path, "rb", reader->opened_path, sizeof(reader->opened_path));
  if (reader->fp) return ESP_OK;

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;
  reader->sd_mounted = !was_sd_mounted;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    reader->close(reader);
    return ESP_ERR_INVALID_ARG;
  }

  reader->fp = xm_fopen_any_ext_case(full, "rb", reader->opened_path, sizeof(reader->opened_path));
  if (!reader->fp)
  {
    reader->close(reader);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t xm_stream_reader_read_exact(XmStreamReader *reader, void *dst, size_t size)
{
  size_t got = 0;

  if (!reader || (!dst && size)) return ESP_ERR_INVALID_ARG;
  if (reader->err != ESP_OK) return reader->err;
  if (!size) return ESP_OK;

  if (reader->pos > reader->file_size || size > reader->file_size - reader->pos)
  {
    reader->err = ESP_ERR_INVALID_SIZE;
    return reader->err;
  }

  esp_err_t err = reader->read(reader, dst, size, &got);
  if (err != ESP_OK)
  {
    reader->err = err;
    return err;
  }

  if (got != size)
  {
    reader->err = ESP_FAIL;
    return reader->err;
  }

  return ESP_OK;
}

void xm_set_mod_context_info(int handle, const char *path, size_t file_size, xm_context_t *ctx)
{
  XM_INFO *info;

  if (!ctx) return;

  info = &xm_info[handle];
  memset(info, 0, sizeof(*info));
  info->valid = 1;
  info->header_size = 1084;
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;

  if (path)
  {
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = 0;
  }

#if XM_STRINGS
  xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif
}

void xm_set_s3m_context_info(int handle, const char *path, size_t file_size, xm_context_t *ctx)
{
  XM_INFO *info;

  if (!ctx) return;

  info = &xm_info[handle];
  memset(info, 0, sizeof(*info));
  info->valid = 1;
  info->header_size = S3M_HEADER_SIZE;
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;

  if (path)
  {
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = 0;
  }

#if XM_STRINGS
  xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif
}

// -------------------- Console helpers --------------------

bool xm_path_is_sd_abs(const char *path)
{
  if (!path) return false;
  if (!strcmp(path, "/sd")) return true;
  return strncmp(path, "/sd/", 4) == 0;
}

bool xm_copy_path_with_ext_case(const char *path, char *out, size_t out_size, bool upper)
{
  const char *slash;
  const char *ext;
  size_t len;

  if (!path || !out || !out_size) return false;

  slash = strrchr(path, '/');
  ext = strrchr(path, '.');
  if (!ext || (slash && ext < slash) || ext[1] == 0) return false;

  len = strlen(path);
  if (len >= out_size) return false;

  memcpy(out, path, len + 1);

  for (char *p = out + (ext - path) + 1; *p; p++)
  {
    unsigned char c = (unsigned char)*p;
    *p = (char)(upper ? toupper(c) : tolower(c));
  }

  return strcmp(out, path) != 0;
}

esp_err_t xm_stat_file_size(const char *path, size_t *out_size)
{
  struct stat st;

  if (!path || !path[0] || !out_size) return ESP_ERR_INVALID_ARG;
  if (stat(path, &st) != 0) return ESP_FAIL;
  if (st.st_size <= 0 || st.st_size > INT_MAX) return ESP_ERR_INVALID_SIZE;

  *out_size = (size_t)st.st_size;
  return ESP_OK;
}

esp_err_t xm_stat_file_size_any_ext_case(const char *path, size_t *out_size)
{
  esp_err_t err;
  char alt[256];

  err = xm_stat_file_size(path, out_size);
  if (err == ESP_OK) return ESP_OK;
  if (err == ESP_ERR_INVALID_ARG || err == ESP_ERR_INVALID_SIZE) return err;

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), false))
  {
    err = xm_stat_file_size(alt, out_size);
    if (err == ESP_OK) return ESP_OK;
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), true))
  {
    err = xm_stat_file_size(alt, out_size);
    if (err == ESP_OK) return ESP_OK;
  }

  return err;
}

FILE *xm_fopen_any_ext_case(const char *path, const char *mode, char *opened_path, size_t opened_path_size)
{
  FILE *fp;
  char alt[256];

  if (opened_path && opened_path_size) opened_path[0] = 0;
  if (!path || !path[0] || !mode) return NULL;

  fp = fopen(path, mode);
  if (fp)
  {
    if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", path);
    return fp;
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), false))
  {
    fp = fopen(alt, mode);
    if (fp)
    {
      if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", alt);
      return fp;
    }
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), true))
  {
    fp = fopen(alt, mode);
    if (fp)
    {
      if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", alt);
      return fp;
    }
  }

  return NULL;
}

esp_err_t xm_get_file_size(const char *path, size_t *out_size)
{
  esp_err_t err;
  bool was_sd_mounted;
  char full[256];

  if (!path || !path[0] || !out_size) return ESP_ERR_INVALID_ARG;

  *out_size = 0;
  err = xm_stat_file_size_any_ext_case(path, out_size);
  if (err == ESP_OK) return ESP_OK;

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
    return ESP_ERR_INVALID_ARG;
  }

  err = xm_stat_file_size_any_ext_case(full, out_size);
  if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
  return err;
}

esp_err_t xm_read_file_data(const char *path, void *dst, size_t size)
{
  FILE *fp;
  size_t got;
  bool was_sd_mounted;
  char full[256];
  esp_err_t err = ESP_FAIL;

  if (!path || !path[0] || !dst) return ESP_ERR_INVALID_ARG;

  fp = xm_fopen_any_ext_case(path, "rb", NULL, 0);
  if (fp)
  {
    got = fread(dst, 1, size, fp);
    fclose(fp);
    if (got != size) return ESP_FAIL;
    return ESP_OK;
  }

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
    return ESP_ERR_INVALID_ARG;
  }

  fp = xm_fopen_any_ext_case(full, "rb", NULL, 0);
  if (fp)
  {
    got = fread(dst, 1, size, fp);
    fclose(fp);
    err = (got == size) ? ESP_OK : ESP_FAIL;
  }

  if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
  return err;
}

const char *xm_file_format_str(tracker_file_format_t format)
{
  switch (format)
  {
    case TRACK_FILE_FORMAT_XMZ: return "XMZ";
    case TRACK_FILE_FORMAT_MOD: return "MOD";
    case TRACK_FILE_FORMAT_S3M: return "S3M";
    case TRACK_FILE_FORMAT_XM:
    default: return "XM";
  }
}

void xmz_inflate_stream_close(XmzInflateStream *stream)
{
  if (!stream) return;

  if (stream->reader.close)
    stream->reader.close(&stream->reader);

  if (stream->in_buf)
  {
    free(stream->in_buf);
    stream->in_buf = NULL;
  }

  if (stream->dict_buf)
  {
    free(stream->dict_buf);
    stream->dict_buf = NULL;
  }
}

int xmz_inflate_stream_open(XmzInflateStream *stream, const char *path, size_t packed_size, size_t *out_unpacked_size, tracker_file_format_t format, bool quiet)
{
  u8 header[sizeof(u32)];
  esp_err_t err;

  if (!stream || !path || !out_unpacked_size) return 1;
  memset(stream, 0, sizeof(*stream));
  *out_unpacked_size = 0;

  if (packed_size <= sizeof(u32) || packed_size > INT_MAX)
  {
    if (!quiet) printf("E: invalid %s size: %u bytes\r\n", xm_file_format_str(format), (unsigned)packed_size);
    return 1;
  }

  err = xm_stream_reader_open(&stream->reader, path, packed_size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s stream open failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    return 1;
  }

  err = xm_stream_reader_read_exact(&stream->reader, header, sizeof(header));
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s header read failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    xmz_inflate_stream_close(stream);
    return 1;
  }

  stream->unpacked_size = xm_rd_le32(header);
  if (!stream->unpacked_size || stream->unpacked_size > INT_MAX)
  {
    if (!quiet) printf("E: invalid %s unpacked size: %u bytes\r\n", xm_file_format_str(format), (unsigned)stream->unpacked_size);
    xmz_inflate_stream_close(stream);
    return 1;
  }

  stream->in_buf = (u8*)malloc_spiram(TRACKER_FILE_STREAM_CHUNK_SIZE);
  stream->dict_buf = (u8*)malloc_spiram(XMZ_INFLATE_DICT_SIZE);
  if (!stream->in_buf || !stream->dict_buf)
  {
    if (!quiet) printf("E: %s inflate buffer alloc failed\r\n", xm_file_format_str(format));
    xmz_inflate_stream_close(stream);
    return 1;
  }

  tinfl_init(decomp);
  stream->status = (tinfl_status)1;
  *out_unpacked_size = stream->unpacked_size;
  return 0;
}

int xmz_inflate_stream_read_input(XmzInflateStream *stream, tracker_file_format_t format, bool quiet)
{
  size_t left;
  size_t todo;
  esp_err_t err;

  if (!stream) return 1;
  if (stream->in_pos < stream->in_size) return 0;

  stream->in_pos = 0;
  stream->in_size = 0;

  if (stream->reader.pos >= stream->reader.file_size) return 0;

  left = stream->reader.file_size - stream->reader.pos;
  todo = left < TRACKER_FILE_STREAM_CHUNK_SIZE ? left : TRACKER_FILE_STREAM_CHUNK_SIZE;
  err = xm_stream_reader_read_exact(&stream->reader, stream->in_buf, todo);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s compressed read failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    return 1;
  }

  stream->in_size = todo;
  return 0;
}

int xmz_inflate_stream_fill(XmzInflateStream *stream, u8 *dst, size_t size, tracker_file_format_t format, bool quiet)
{
  size_t out_pos = 0;

  if (!stream || (!dst && size)) return 1;

  while (out_pos < size)
  {
    size_t inbytes;
    size_t outbytes;
    size_t dict_space;
    int flags;

    if (stream->done)
    {
      if (!quiet) printf("E: %s stream ended before expected output\r\n", xm_file_format_str(format));
      return 1;
    }

    if (stream->in_pos >= stream->in_size)
    {
      if (xmz_inflate_stream_read_input(stream, format, quiet)) return 1;
      if (stream->in_pos >= stream->in_size)
      {
        if (!quiet) printf("E: %s compressed stream truncated\r\n", xm_file_format_str(format));
        return 1;
      }
    }

    if (stream->dict_pos >= XMZ_INFLATE_DICT_SIZE)
      stream->dict_pos = 0;

    dict_space = XMZ_INFLATE_DICT_SIZE - stream->dict_pos;
    inbytes = stream->in_size - stream->in_pos;
    outbytes = size - out_pos;
    if (outbytes > dict_space) outbytes = dict_space;

    flags = TINFL_FLAG_PARSE_ZLIB_HEADER;
    if (stream->reader.pos < stream->reader.file_size)
      flags |= TINFL_FLAG_HAS_MORE_INPUT;

    stream->status = tinfl_decompress(decomp,
      stream->in_buf + stream->in_pos,
      &inbytes,
      stream->dict_buf,
      stream->dict_buf + stream->dict_pos,
      &outbytes,
      flags);

    stream->in_pos += inbytes;

    if (outbytes)
    {
      memcpy(dst + out_pos, stream->dict_buf + stream->dict_pos, outbytes);
      stream->dict_pos += outbytes;
      out_pos += outbytes;
    }

    if (stream->status < TINFL_STATUS_DONE)
    {
      if (!quiet) printf("E: %s unzip failed: status=%d\r\n", xm_file_format_str(format), (int)stream->status);
      return 1;
    }

    if (stream->status == TINFL_STATUS_DONE)
    {
      stream->done = true;
      if (out_pos != size)
      {
        if (!quiet) printf("E: %s unpacked size mismatch\r\n", xm_file_format_str(format));
        return 1;
      }
      break;
    }

    if (!inbytes && !outbytes)
    {
      if (stream->in_pos >= stream->in_size) continue;
      if (!quiet) printf("E: %s inflate stalled\r\n", xm_file_format_str(format));
      return 1;
    }
  }

  return 0;
}

int xmz_inflate_stream_finish(XmzInflateStream *stream, tracker_file_format_t format, bool quiet)
{
  if (!stream) return 1;

  while (!stream->done)
  {
    size_t inbytes;
    size_t outbytes;
    size_t dict_space;
    int flags;

    if (stream->in_pos >= stream->in_size)
    {
      if (xmz_inflate_stream_read_input(stream, format, quiet)) return 1;
    }

    if (stream->dict_pos >= XMZ_INFLATE_DICT_SIZE)
      stream->dict_pos = 0;

    dict_space = XMZ_INFLATE_DICT_SIZE - stream->dict_pos;
    inbytes = stream->in_size - stream->in_pos;
    outbytes = dict_space;

    flags = TINFL_FLAG_PARSE_ZLIB_HEADER;
    if (stream->reader.pos < stream->reader.file_size)
      flags |= TINFL_FLAG_HAS_MORE_INPUT;

    stream->status = tinfl_decompress(decomp,
      stream->in_buf + stream->in_pos,
      &inbytes,
      stream->dict_buf,
      stream->dict_buf + stream->dict_pos,
      &outbytes,
      flags);

    stream->in_pos += inbytes;
    stream->dict_pos += outbytes;

    if (stream->status < TINFL_STATUS_DONE)
    {
      if (!quiet) printf("E: %s unzip failed: status=%d\r\n", xm_file_format_str(format), (int)stream->status);
      return 1;
    }

    if (outbytes)
    {
      if (!quiet) printf("E: %s unpacked data exceeds header size\r\n", xm_file_format_str(format));
      return 1;
    }

    if (stream->status == TINFL_STATUS_DONE)
    {
      stream->done = true;
      return 0;
    }

    if (!inbytes)
    {
      if (stream->reader.pos < stream->reader.file_size) continue;
      if (!quiet) printf("E: %s compressed stream truncated\r\n", xm_file_format_str(format));
      return 1;
    }
  }

  return 0;
}

int xm_load_compressed_file_to_handle(const char *path, size_t size, tracker_file_format_t format, int *out_handle, bool quiet)
{
  XmzInflateStream stream;
  TrackerStreamChunkRequest request;
  TrackerStreamPushResult result;
  u8 *chunk = NULL;
  size_t unpacked_size = 0;
  size_t chunk_size;
  int handle = -1;
  int rc;
  esp_err_t err;

  if (out_handle) *out_handle = -1;

  if (xmz_inflate_stream_open(&stream, path, size, &unpacked_size, format, quiet)) return 1;

  chunk_size = unpacked_size < TRACKER_FILE_STREAM_CHUNK_SIZE ? unpacked_size : TRACKER_FILE_STREAM_CHUNK_SIZE;
  chunk = (u8*)malloc_spiram(chunk_size);
  if (!chunk)
  {
    if (!quiet) printf("E: %s stream chunk alloc failed: %u bytes\r\n", xm_file_format_str(format), (unsigned)chunk_size);
    xmz_inflate_stream_close(&stream);
    return 1;
  }

  err = tracker_stream_begin(unpacked_size, TRACK_HOST_STREAM_FORMAT_XM);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s stream begin failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    xmz_inflate_stream_close(&stream);
    free(chunk);
    return 1;
  }

  while (1)
  {
    size_t left;
    size_t rx_size;

    if (g_xm_host_stream.pos >= g_xm_host_stream.total_size)
    {
      if (!quiet) printf("E: %s stream requested data past EOF\r\n", xm_file_format_str(format));
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    left = g_xm_host_stream.total_size - g_xm_host_stream.pos;
    rx_size = left < chunk_size ? left : chunk_size;

    err = tracker_stream_request_chunk(rx_size, &request);
    if (err != ESP_OK)
    {
      if (!quiet) printf("E: %s stream request failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    if (request.offset != stream.unpacked_pos)
    {
      if (!quiet) printf("E: %s stream requested non-linear offset: %u expected %u\r\n",
        xm_file_format_str(format),
        (unsigned)request.offset,
        (unsigned)stream.unpacked_pos);
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    if (xmz_inflate_stream_fill(&stream, chunk, request.logical_size, format, quiet))
    {
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }
    stream.unpacked_pos += request.logical_size;

    rc = tracker_stream_push_chunk(chunk, request.rx_size, &result);
    if (rc == TRACK_HOST_BUILD_NEED_MORE)
      continue;

    if (rc < 0)
    {
      if (!quiet) printf("E: invalid %s stream after unzip\r\n", xm_file_format_str(format));
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    if (xmz_inflate_stream_finish(&stream, format, quiet))
    {
      xm_host_stream_abort_current();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    handle = attach_obj(result.ctx, (int)result.used_size, result.obj_type);
    if (handle < 0)
    {
      xm_free_context(result.ctx);
      tracker_stream_finish_success();
      xmz_inflate_stream_close(&stream);
      free(chunk);
      return 1;
    }

    tracker_stream_set_context_info(handle, result.obj_type, path, unpacked_size, result.ctx);
    xm_reset_context_state(result.ctx, handle);
    mem_obj[handle].state = TRACK_OBJ_ST_STOPPED;
    xm_reset_render_stats();
    tracker_stream_finish_success();
    xmz_inflate_stream_close(&stream);
    free(chunk);

    if (!quiet)
    {
      printf("%s stream loaded: handle=%02X packed=%u unpacked=%u ctx=%u\r\n",
        xm_file_format_str(format),
        handle,
        (unsigned)size,
        (unsigned)unpacked_size,
        (unsigned)mem_obj[handle].size);
      if (xm_info[handle].module_name[0])
        printf("  Module : %s\r\n", xm_info[handle].module_name);
      if (xm_info[handle].tracker_name[0])
        printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);
    }

    if (out_handle) *out_handle = handle;
    return 0;
  }
}

int xm_load_file_to_handle(const char *path, int *out_handle, bool quiet)
{
  size_t size = 0;
  esp_err_t err;
  tracker_file_format_t format;
  int handle = -1;

  if (out_handle) *out_handle = -1;

  if (!path || !path[0])
  {
    if (!quiet) printf("Usage: xm load <path>\r\n");
    return 1;
  }

  const char *ext = strrchr(path, '.');
  if (ext && !strcasecmp(ext, ".xmz"))
    format = TRACK_FILE_FORMAT_XMZ;
  else if (ext && !strcasecmp(ext, ".mod"))
    format = TRACK_FILE_FORMAT_MOD;
  else if (ext && !strcasecmp(ext, ".s3m"))
    format = TRACK_FILE_FORMAT_S3M;
  else
    format = TRACK_FILE_FORMAT_XM;

  err = xm_get_file_size(path, &size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s size failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    return 1;
  }

  if (size == 0 || size > INT_MAX)
  {
    if (!quiet) printf("E: %s file too large: %u bytes\r\n", xm_file_format_str(format), (unsigned)size);
    return 1;
  }

  if (format == TRACK_FILE_FORMAT_XM || format == TRACK_FILE_FORMAT_MOD || format == TRACK_FILE_FORMAT_S3M)
  {
    if (tracker_stream_load_file_to_handle(path, size, format, &handle, quiet)) return 1;
    if (out_handle) *out_handle = handle;
    return 0;
  }

  if (xm_load_compressed_file_to_handle(path, size, format, &handle, quiet)) return 1;

  if (!quiet)
  {
    printf("%s loaded: handle=%02X  size=%u\r\n",
      xm_file_format_str(format),
      handle,
      (unsigned)mem_obj[handle].size);
    if (xm_info[handle].module_name[0])
      printf("  Module : %s\r\n", xm_info[handle].module_name);
    if (xm_info[handle].tracker_name[0])
      printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);
  }

  if (out_handle) *out_handle = handle;
  return 0;
}

int xm_load_file(const char *path)
{
  int handle = -1;
  return xm_load_file_to_handle(path, &handle, false);
}

int xm_load_play_file(const char *path, bool quiet)
{
  int handle = -1;

  if (xm_delete_all_modules(quiet)) return 1;
  if (xm_load_file_to_handle(path, &handle, quiet)) return 1;
  return xm_play_cmd(handle, quiet);
}

// -------------------- Console --------------------

int xm_list_cmd()
{
  int found = 0;

  printf("\r\nModule objects\r\n");
  printf("HH  Type  State     Size      Name\r\n");

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
  {
    if (!mem_obj[i].addr) continue;
    if (mem_obj[i].type != OBJ_TYPE_XM && mem_obj[i].type != OBJ_TYPE_XMC && mem_obj[i].type != OBJ_TYPE_MOD && mem_obj[i].type != OBJ_TYPE_MDC && mem_obj[i].type != OBJ_TYPE_S3M && mem_obj[i].type != OBJ_TYPE_S3C) continue;

    const char *name = xm_info[i].module_name[0] ? xm_info[i].module_name : xm_info[i].path;
    printf("%02X  %-4s  %-8s  %-8u  %s\r\n",
      i,
      xm_obj_type_str(mem_obj[i].type),
      xm_obj_state_str(mem_obj[i].state),
      (unsigned)mem_obj[i].size,
      name && name[0] ? name : "-");
    found++;
  }

  if (!found)
    printf("(none)\r\n");

  printf("\r\n");
  return 0;
}
void xm_context_copy_name(char *dst, size_t dst_size, const char *src, size_t src_size)
{
  if (!dst || dst_size == 0) return;

  xm_copy_trimmed(dst, dst_size, (const u8*)src, src_size);
}

const char *xm_context_loop_text(xm_loop_type_t loop_type)
{
  switch (loop_type)
  {
    case XM_FORWARD_LOOP:   return "fwd";
    case XM_PING_PONG_LOOP: return "ping";
    default:                return "no";
  }
}

unsigned xm_context_sample_length_bytes(const xm_sample_t *sample)
{
  if (!sample) return 0;
  return (unsigned)sample->length * (sample->bits == 16 ? 2u : 1u);
}

unsigned xm_context_sample_volume_u8(const xm_sample_t *sample)
{
  int value;

  if (!sample) return 0;
  value = (int)(sample->volume * 64.0f + 0.5f);
  if (value < 0) value = 0;
  if (value > 64) value = 64;
  return (unsigned)value;
}

unsigned xm_context_sample_panning_u8(const xm_sample_t *sample)
{
  int value;

  if (!sample) return 0;
  value = (int)(sample->panning * 255.0f + 0.5f);
  if (value < 0) value = 0;
  if (value > 255) value = 255;
  return (unsigned)value;
}

int xm_context_sample_finetune_print(const xm_sample_t *sample, bool mod_context)
{
  if (!sample) return 0;
  if (mod_context) return sample->finetune / 16;
  return sample->finetune;
}

const char *xm_context_frequency_text(xm_frequency_type_t frequency_type)
{
  switch (frequency_type)
  {
    case XM_LINEAR_FREQUENCIES: return "linear";
    case XM_S3M_FREQUENCIES: return "s3m";
    case XM_AMIGA_FREQUENCIES:
    default: return "amiga";
  }
}

void xm_print_object_info(int handle)
{
  XM_INFO *info = &xm_info[handle];

  printf("\r\nModule object %02X\r\n", handle);
  printf("Type            : %s\r\n", xm_obj_type_str(mem_obj[handle].type));
  printf("State           : %s\r\n", xm_obj_state_str(mem_obj[handle].state));
  printf("Object size     : %u\r\n", (unsigned)mem_obj[handle].size);

  if (info->valid)
  {
    printf("Path            : %s\r\n", info->path[0] ? info->path : "-");
    printf("Module name     : %s\r\n", info->module_name[0] ? info->module_name : "-");
    printf("Tracker name    : %s\r\n", info->tracker_name[0] ? info->tracker_name : "-");
    printf("Version         : %u.%02u\r\n", (unsigned)(info->version >> 8), (unsigned)(info->version & 0xFF));
    printf("File size       : %u\r\n", (unsigned)info->file_size);
    printf("Header size     : %u\r\n", (unsigned)info->header_size);
    printf("Song length     : %u\r\n", (unsigned)info->song_length);
    printf("Restart pos     : %u\r\n", (unsigned)info->restart_position);
    printf("Channels        : %u\r\n", (unsigned)info->num_channels);
    printf("Patterns        : %u\r\n", (unsigned)info->num_patterns);
    printf("Instruments     : %u\r\n", (unsigned)info->num_instruments);
    printf("Flags           : 0x%04X\r\n", (unsigned)info->flags);
    printf("Tempo           : %u\r\n", (unsigned)info->tempo);
    printf("BPM             : %u\r\n", (unsigned)info->bpm);
  }
  else
    printf("Header info     : not available\r\n");
}

int xm_print_context_module_info(int handle)
{
  xm_context_t *ctx;
  XM_INFO *info;
  bool is_mod;
  bool is_s3m;
  const char *module_type;
  char name[24];
  char tracker[24];

  if (!mem_obj[handle].addr) return 1;
  if (mem_obj[handle].type != OBJ_TYPE_XMC && mem_obj[handle].type != OBJ_TYPE_MDC && mem_obj[handle].type != OBJ_TYPE_S3C)
  {
    printf("\r\nDetailed module info: not available for raw %s object, context object required\r\n", xm_obj_type_str(mem_obj[handle].type));
    return 0;
  }

  ctx = (xm_context_t*)mem_obj[handle].addr;
  info = &xm_info[handle];
  is_mod = mem_obj[handle].type == OBJ_TYPE_MDC;
  is_s3m = mem_obj[handle].type == OBJ_TYPE_S3C;
  module_type = is_mod ? "MOD" : (is_s3m ? "S3M" : "XM");

  xm_context_copy_name(name, sizeof(name), ctx->module.name, MODULE_NAME_LENGTH);
  xm_context_copy_name(tracker, sizeof(tracker), ctx->module.trackername, TRACKER_NAME_LENGTH);

  printf("\r\n%s module: %s\r\n", module_type, name[0] ? name : "<unnamed>");
  printf("File: %s\r\n", info->path[0] ? info->path : "-");

  if (is_mod)
  {
    printf("Format: %s  channels=%u  orders=%u  patterns=%u  restart=%u\r\n",
      tracker[0] ? tracker : "ProTracker MOD",
      (unsigned)ctx->module.num_channels,
      (unsigned)ctx->module.length,
      (unsigned)ctx->module.num_patterns,
      (unsigned)ctx->module.restart_position);
    printf("File size=%u\r\n", (unsigned)info->file_size);
    printf("\r\nSamples / instruments:\r\n");
    printf(" #  Name                    LenB   LoopSt LoopLen Loop Vol Fine\r\n");

    for (uint16_t i = 0; i < ctx->module.num_instruments; i++)
    {
      xm_instrument_t *instr = ctx->module.instruments + i;
      xm_sample_t *sample = instr->num_samples ? instr->samples : NULL;

      xm_context_copy_name(name, sizeof(name), instr->name, INSTRUMENT_NAME_LENGTH);
      printf("%02u  %-22s %6u %6u %7u %-4s %3u %4d\r\n",
        (unsigned)i + 1,
        name,
        xm_context_sample_length_bytes(sample),
        sample ? (unsigned)sample->loop_start : 0u,
        sample ? (unsigned)sample->loop_length : 0u,
        sample ? xm_context_loop_text(sample->loop_type) : "no",
        xm_context_sample_volume_u8(sample),
        xm_context_sample_finetune_print(sample, true));
    }

    return 0;
  }

  printf("Tracker: %s  version=%u.%02u\r\n",
    tracker[0] ? tracker : "?",
    (unsigned)(info->version >> 8),
    (unsigned)(info->version & 0xff));
  printf("Orders=%u  restart=%u  channels=%u  patterns=%u  instruments=%u\r\n",
    (unsigned)ctx->module.length,
    (unsigned)ctx->module.restart_position,
    (unsigned)ctx->module.num_channels,
    (unsigned)ctx->module.num_patterns,
    (unsigned)ctx->module.num_instruments);
  printf("Tempo=%u  BPM=%u  frequency=%s  file size=%u\r\n",
    (unsigned)ctx->tempo,
    (unsigned)ctx->bpm,
    xm_context_frequency_text(ctx->module.frequency_type),
    (unsigned)info->file_size);

  printf("\r\nInstruments:\r\n");
  printf("Idx Samples Name\r\n");
  for (uint16_t i = 0; i < ctx->module.num_instruments; i++)
  {
    xm_instrument_t *instr = ctx->module.instruments + i;
    xm_context_copy_name(name, sizeof(name), instr->name, INSTRUMENT_NAME_LENGTH);
    printf("%03u %7u %s\r\n",
      (unsigned)i + 1,
      (unsigned)instr->num_samples,
      name);
  }

  printf("\r\nSamples:\r\n");
  if (is_s3m)
    printf("Ins Smp Name                    LenB   LoopSt LoopLen Loop Bit Vol Pan C4Spd\r\n");
  else
    printf("Ins Smp Name                    LenB   LoopSt LoopLen Loop Bit Vol Pan Rel Fine\r\n");

  for (uint16_t i = 0; i < ctx->module.num_instruments; i++)
  {
    xm_instrument_t *instr = ctx->module.instruments + i;

    for (uint16_t j = 0; j < instr->num_samples; j++)
    {
      xm_sample_t *sample = instr->samples + j;
      unsigned bytes_per_sample = sample->bits == 16 ? 2u : 1u;

      xm_context_copy_name(name, sizeof(name), sample->name, SAMPLE_NAME_LENGTH);
      if (is_s3m)
      {
        printf("%03u %03u %-22s %6u %6u %7u %-4s %3u %3u %3u %5u\r\n",
          (unsigned)i + 1,
          (unsigned)j + 1,
          name,
          xm_context_sample_length_bytes(sample),
          (unsigned)sample->loop_start * bytes_per_sample,
          (unsigned)sample->loop_length * bytes_per_sample,
          xm_context_loop_text(sample->loop_type),
          (unsigned)sample->bits,
          xm_context_sample_volume_u8(sample),
          xm_context_sample_panning_u8(sample),
          (unsigned)sample->c4speed);
      }
      else
      {
        printf("%03u %03u %-22s %6u %6u %7u %-4s %3u %3u %3u %3d %4d\r\n",
          (unsigned)i + 1,
          (unsigned)j + 1,
          name,
          xm_context_sample_length_bytes(sample),
          (unsigned)sample->loop_start * bytes_per_sample,
          (unsigned)sample->loop_length * bytes_per_sample,
          xm_context_loop_text(sample->loop_type),
          (unsigned)sample->bits,
          xm_context_sample_volume_u8(sample),
          xm_context_sample_panning_u8(sample),
          (int)sample->relative_note,
          xm_context_sample_finetune_print(sample, false));
      }
    }
  }

  return 0;
}

int xm_info_cmd(int argc, char **argv)
{
  int handle = -1;
  bool detailed = argc >= 3;

  if (detailed)
  {
    if (!xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Bad <handle>: %s\r\n", argv[2]);
      return 1;
    }
  }
  else if (curr_xm_handle >= 0 && curr_xm_handle < OBJ_HANDLES_MAX && mem_obj[curr_xm_handle].addr)
    handle = curr_xm_handle;
  else
  {
    for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    {
      if (mem_obj[i].addr && (mem_obj[i].type == OBJ_TYPE_XM || mem_obj[i].type == OBJ_TYPE_XMC || mem_obj[i].type == OBJ_TYPE_MOD || mem_obj[i].type == OBJ_TYPE_MDC || mem_obj[i].type == OBJ_TYPE_S3M || mem_obj[i].type == OBJ_TYPE_S3C))
      {
        handle = i;
        break;
      }
    }
  }

  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !mem_obj[handle].addr)
  {
    printf("No module object selected\r\n");
    return 1;
  }

  if (detailed)
  {
    if (xm_print_context_module_info(handle)) return 1;
    printf("\r\n");
    return 0;
  }

  xm_print_object_info(handle);
  printf("\r\n");
  return 0;
}
int xm_play_cmd(int handle, bool quiet)
{
  if (!mem_obj[handle].addr)
  {
    if (!quiet) printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XMC && mem_obj[handle].type != OBJ_TYPE_MDC && mem_obj[handle].type != OBJ_TYPE_S3C)
  {
    if (!quiet) printf("Handle %02X is not an initialized module context object\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].state == TRACK_OBJ_ST_PLAYING)
  {
    if (xm_stop_current_playback(quiet)) return 1;
  }
  else
  {
    int playing = xm_find_playing_handle();
    if (playing >= 0 && playing != handle)
      if (xm_stop_current_playback(quiet)) return 1;
  }

  TRACK_TASK task = {};
  task.task = TRACK_TASK_PLAY;
  task.handle = handle;
  set_status(ESP_ST_BUSY);
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, TRACK_OBJ_ST_PLAYING, 2000))
  {
    if (!quiet) ESP_LOGE("xm_play_cmd", "XM play failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  if (!quiet) printf("XM playing: %02X\r\n", handle);
  return 0;
}

int xm_stop_cmd(bool quiet)
{
  int handle = xm_find_playing_handle();
  if (handle < 0)
  {
    if (!quiet) printf("XM is not playing\r\n");
    return 0;
  }

  if (xm_stop_current_playback(quiet)) return 1;

  if (!quiet) printf("XM stopped: %02X\r\n", handle);
  return 0;
}

int xm_reset_cmd(int handle, bool quiet)
{
  if (!mem_obj[handle].addr)
  {
    if (!quiet) printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XMC && mem_obj[handle].type != OBJ_TYPE_MDC && mem_obj[handle].type != OBJ_TYPE_S3C)
  {
    if (!quiet) printf("Handle %02X is not an initialized module context object\r\n", handle);
    return 1;
  }

  TRACK_TASK task = {};
  task.task = TRACK_TASK_RESET;
  task.handle = handle;
  set_status(ESP_ST_BUSY);
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_status_ready(3000) || mem_obj[handle].state != TRACK_OBJ_ST_STOPPED)
  {
    if (!quiet) ESP_LOGE("xm_reset_cmd", "XM reset failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  if (!quiet) printf("XM reset: %02X\r\n", handle);
  return 0;
}

int xm_del_cmd(int handle)
{
  if (!mem_obj[handle].addr)
  {
    printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XM && mem_obj[handle].type != OBJ_TYPE_XMC && mem_obj[handle].type != OBJ_TYPE_MOD && mem_obj[handle].type != OBJ_TYPE_MDC && mem_obj[handle].type != OBJ_TYPE_S3M && mem_obj[handle].type != OBJ_TYPE_S3C)
  {
    printf("Handle %02X is not a module object\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].state == TRACK_OBJ_ST_PLAYING)
    if (xm_stop_current_playback(false)) return 1;

  if (!delete_obj(handle))
  {
    printf("Delete failed for handle %02X\r\n", handle);
    return 1;
  }

  if (curr_xm_handle == handle)
    curr_xm_handle = -1;

  xm_clear_info(handle);

  printf("XM deleted: %02X\r\n", handle);
  return 0;
}

int xm_vol_cmd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Tracker volume: %d\r\n", master_volume / 1000);
    return 0;
  }

  char *endp = NULL;
  unsigned long vol = strtoul(argv[2], &endp, 0);
  if (!endp || *endp || vol > 255)
  {
    printf("Bad <volume>: %s (expected 0..255)\r\n", argv[2]);
    return 1;
  }

  master_volume = (int)(vol * 1000UL);
  printf("Tracker volume: %d\r\n", master_volume / 1000);
  return 0;
}

int xm_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  xm load <file.xm|xmz|mod|s3m>\r\n");
    printf("  xm list\r\n");
    printf("  xm info             (brief current/first module)\r\n");
    printf("  xm info <handle>    (detailed module tables)\r\n");
    printf("  xm play <handle>\r\n");
    printf("  xm stop\r\n");
    printf("  xm reset <handle>\r\n");
    printf("  xm del <handle>\r\n");
    printf("  xm vol [0..255]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "load"))
    return xm_load_file(argc >= 3 ? argv[2] : NULL);

  if (!strcmp(op, "list"))
    return xm_list_cmd();

  if (!strcmp(op, "info"))
    return xm_info_cmd(argc, argv);

  if (!strcmp(op, "stop"))
    return xm_stop_cmd();

  if (!strcmp(op, "vol"))
    return xm_vol_cmd(argc, argv);

  if (!strcmp(op, "play"))
  {
    int handle = -1;
    if (argc < 3 || !xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Usage: xm play <handle>\r\n");
      return 1;
    }
    return xm_play_cmd(handle);
  }

  if (!strcmp(op, "reset"))
  {
    int handle = -1;
    if (argc < 3 || !xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Usage: xm reset <handle>\r\n");
      return 1;
    }
    return xm_reset_cmd(handle);
  }

  if (!strcmp(op, "del"))
  {
    int handle = -1;
    if (argc < 3 || !xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Usage: xm del <handle>\r\n");
      return 1;
    }
    return xm_del_cmd(handle);
  }

  printf("Unknown xm subcommand: %s\r\n", op);
  return 1;
}

void xm_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "xm",
    .help     = "XM commands: load/list/info/play/stop/del/vol",
    .hint     = NULL,
    .func     = &xm_cmd,
    .argtable = NULL,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
