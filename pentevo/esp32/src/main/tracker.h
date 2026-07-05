#pragma once

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
// ============================================================================
// Common tracker declarations
// ============================================================================

// -------------------- Common tracker types --------------------

typedef enum tracker_format_e
{
  TRACKER_FORMAT_XM = 0,
  TRACKER_FORMAT_MOD,
  TRACKER_FORMAT_S3M
} tracker_format_t;

#define TRACKER_CONTEXT_SEGMENT_PAGE_CAPACITY 64
#define TRACKER_CONTEXT_SEGMENTS_MAGIC 0x5453474dU
#define TRACKER_CONTEXT_SEGMENTS_MAGIC_CHECK 0xaba7b8b2U

typedef enum tracker_context_segment_type_e
{
  TRACKER_CONTEXT_SEG_CONTEXT = 0,
  TRACKER_CONTEXT_SEG_RUNTIME,
  TRACKER_CONTEXT_SEG_PATTERNS,
  TRACKER_CONTEXT_SEG_INSTRUMENTS,
  TRACKER_CONTEXT_SEG_SAMPLES,
  TRACKER_CONTEXT_SEG_FORMAT_EXTRA
} tracker_context_segment_type_t;

typedef struct tracker_context_segment_s
{
  void *addr;
  size_t size;
  uint8_t type;
  uint8_t flags;
  uint16_t index;
} tracker_context_segment_t;

typedef struct tracker_context_segment_page_s tracker_context_segment_page_t;

// -------------------- Common byte order helpers --------------------

uint16_t tracker_rd_be16(const uint8_t *p);
uint16_t tracker_rd_le16(const uint8_t *p);
uint32_t tracker_rd_le24(const uint8_t *p);
uint32_t tracker_rd_le32(const uint8_t *p);

// -------------------- Common size helpers --------------------

int tracker_size_add(size_t *dst, size_t add);
int tracker_size_mul(size_t a, size_t b, size_t *out);

// -------------------- Common string/memory helpers --------------------

void tracker_copy_trimmed(char *dst, size_t dst_size, const uint8_t *src, size_t src_size);
void tracker_memcpy_pad(void *dst, size_t dst_len, const void *src, size_t src_len, size_t offset);

// -------------------- XM format helpers --------------------


// -------------------- XM grouped context allocation helpers --------------------

typedef struct xm_context_group_sizes_s
{
  size_t context_size;
  size_t patterns_size;
  size_t instruments_size;
  size_t samples_size;
  size_t runtime_size;
  size_t format_extra_size;
  size_t total_size;
} xm_context_group_sizes_t;

typedef struct xm_context_group_cursor_s
{
  char *patterns;
  char *patterns_end;
  char *instruments;
  char *instruments_end;
  char *samples;
  char *samples_end;
  char *runtime;
  char *runtime_end;
  char *format_extra;
  char *format_extra_end;
} xm_context_group_cursor_t;

// -------------------- MOD format helpers --------------------

// -------------------- S3M format helpers --------------------

// ============================================================================
// XM public API
// ============================================================================

/* Author: Romain "Artefact2" Dalmaso <artefact2@gmail.com> */
/* Contributor: Dan Spencer <dan@atomicpotato.net> */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */




#define XM_DEFENSIVE 1
#define XM_LIBXMIZE_DELTA_SAMPLES 0
#define XM_DEBUG 0
#define XM_LINEAR_INTERPOLATION 1
#define XM_RAMPING 1
#define XM_STRINGS 1

struct xm_context_s;
typedef struct xm_context_s xm_context_t;

typedef struct s3m_adlib_instrument_s
{
  uint8_t type;
  uint8_t volume;
  uint32_t c4speed;
  uint8_t regs[12];
} s3m_adlib_instrument_t;

typedef void* (*MFUNC)(size_t);

/** Create a XM context.
 *
 * @param moddata the contents of the module
 * @param moddata_length the length of the contents of the module, in bytes
 * @param rate play rate in Hz, recommended value of 48000
 *
 * @returns size on success
 * @returns -1 if module data is not sane
 * @returns -2 if memory allocation failed
 */

int xm_create_context_safe(xm_context_t**, void* moddata, size_t moddata_length, uint32_t rate, MFUNC mfunc);

/** Create a XM context.
 *
 * This function will produce smaller code size compared to
 * xm_create_context(), but requires converting the .xm file to a
 * non-portable format beforehand.
 *
 * This function doesn't do any kind of error checking.
 *
 * @param libxmizeddata: pointer to module data, needs to point to a
 * writable area, freeing it is equivalent as calling
 * xm_free_context().
 *
 * @see xm_create_context()
 */
void xm_create_context_from_libxmize(xm_context_t**, char* libxmizeddata, uint32_t rate);

/** Free a XM context created by xm_create_context(). */
void xm_free_context(xm_context_t*);

int tracker_context_register_segment(xm_context_t *ctx, void *addr, size_t size, uint8_t type);
int tracker_context_has_registered_segments(const xm_context_t *ctx);
uint16_t tracker_context_get_segment_count(const xm_context_t *ctx);
int tracker_context_get_segment(const xm_context_t *ctx, uint16_t index, tracker_context_segment_t *segment);
void tracker_context_free_segments(xm_context_t *ctx);

void xm_sample(xm_context_t* ctx, float* p_left, float* p_right);
void xm_render(xm_context_t* ctx, float* out, size_t samples);

/** Set the maximum number of times a module can loop. After the
 * specified number of loops, calls to xm_generate_samples will only
 * generate silence. You can control the current number of loops with
 * xm_get_loop_count().
 *
 * @param loopcnt maximum number of loops. Use 0 to loop
 * indefinitely. */
void xm_set_max_loop_count(xm_context_t*, uint8_t loopcnt);

/** Get the loop count of the currently playing module. This value is
 * 0 when the module is still playing, 1 when the module has looped
 * once, etc. */
uint8_t xm_get_loop_count(xm_context_t*);



/** Seek to a specific position in a module.
 *
 * WARNING, WITH BIG LETTERS: seeking modules is broken by design,
 * don't expect miracles.
 */
void xm_seek(xm_context_t*, uint8_t pot, uint8_t row, uint16_t tick);



/** Mute or unmute a channel.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 *
 * @return whether the channel was muted.
 */
bool xm_mute_channel(xm_context_t*, uint16_t, bool);

/** Mute or unmute an instrument.
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 *
 * @return whether the instrument was muted.
 */
bool xm_mute_instrument(xm_context_t*, uint16_t, bool);



/** Get the module name as a NUL-terminated string. */
const char* xm_get_module_name(xm_context_t*);

/** Get the tracker name as a NUL-terminated string. */
const char* xm_get_tracker_name(xm_context_t*);



/** Get the number of channels. */
uint16_t xm_get_number_of_channels(xm_context_t*);

/** Get the module length (in patterns). */
uint16_t xm_get_module_length(xm_context_t*);

/** Get the number of patterns. */
uint16_t xm_get_number_of_patterns(xm_context_t*);

/** Get the number of rows of a pattern.
 *
 * @note Pattern numbers go from 0 to
 * xm_get_number_of_patterns(...)-1.
 */
uint16_t xm_get_number_of_rows(xm_context_t*, uint16_t);

/** Get the number of instruments. */
uint16_t xm_get_number_of_instruments(xm_context_t*);

/** Get the number of samples of an instrument.
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 */
uint16_t xm_get_number_of_samples(xm_context_t*, uint16_t);

/** Get the internal buffer for a given sample waveform.
 *
 * This buffer can be read from or written to, at any time, but the
 * length cannot change. The buffer must be cast to (int8_t*) or
 * (int16_t*) depending on the sample type.
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 *
 * @note Sample numbers go from 0 to
 * xm_get_nubmer_of_samples(...,instr)-1.
 */
void* xm_get_sample_waveform(xm_context_t*, uint16_t instr, uint16_t sample, size_t* length, uint8_t* bits);



/** Get the current module speed.
 *
 * @param bpm will receive the current BPM
 * @param tempo will receive the current tempo (ticks per line)
 */
void xm_get_playing_speed(xm_context_t*, uint16_t* bpm, uint16_t* tempo);

/** Get the current position in the module being played.
 *
 * @param pattern_index if not NULL, will receive the current pattern
 * index in the POT (pattern order table)
 *
 * @param pattern if not NULL, will receive the current pattern number
 *
 * @param row if not NULL, will receive the current row
 *
 * @param samples if not NULL, will receive the total number of
 * generated samples (divide by sample rate to get seconds of
 * generated audio)
 */
void xm_get_position(xm_context_t*, uint8_t* pattern_index, uint8_t* pattern, uint8_t* row, uint64_t* samples);

/** Get the latest time (in number of generated samples) when a
 * particular instrument was triggered in any channel.
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 */
uint64_t xm_get_latest_trigger_of_instrument(xm_context_t*, uint16_t);

/** Get the latest time (in number of generated samples) when a
 * particular sample was triggered in any channel.
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 *
 * @note Sample numbers go from 0 to
 * xm_get_nubmer_of_samples(...,instr)-1.
 */
uint64_t xm_get_latest_trigger_of_sample(xm_context_t*, uint16_t instr, uint16_t sample);

/** Get the latest time (in number of generated samples) when any
 * instrument was triggered in a given channel.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 */
uint64_t xm_get_latest_trigger_of_channel(xm_context_t*, uint16_t);

/** Checks whether a channel is active (ie: is playing something).
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 */
bool xm_is_channel_active(xm_context_t*, uint16_t);

/** Get the instrument number currently playing in a channel.
 *
 * @returns instrument number, or 0 if channel is not active.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 *
 * @note Instrument numbers go from 1 to
 * xm_get_number_of_instruments(...).
 */
uint16_t xm_get_instrument_of_channel(xm_context_t*, uint16_t);

/** Get the frequency of the sample currently playing in a channel.
 *
 * @returns a frequency in Hz. If the channel is not active, return
 * value is undefined.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 */
float xm_get_frequency_of_channel(xm_context_t*, uint16_t);

/** Get the volume of the sample currently playing in a channel. This
 * takes into account envelopes, etc.
 *
 * @returns a volume between 0 or 1. If the channel is not active,
 * return value is undefined.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 */
float xm_get_volume_of_channel(xm_context_t*, uint16_t);

/** Get the panning of the sample currently playing in a channel. This
 * takes into account envelopes, etc.
 *
 * @returns a panning between 0 (L) and 1 (R). If the channel is not
 * active, return value is undefined.
 *
 * @note Channel numbers go from 1 to xm_get_number_of_channels(...).
 */
float xm_get_panning_of_channel(xm_context_t*, uint16_t);

// ============================================================================
// Common tracker runtime internals
// ============================================================================

/* Author: Romain "Artefact2" Dalmaso <artefact2@gmail.com> */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */


#if XM_DEBUG
#include <stdio.h>
#define DEBUG(fmt, ...) \
  { \
		fprintf(stderr, "%s(): " fmt "\n", __func__, __VA_ARGS__); \
		fflush(stderr); \
	}
#else
#define DEBUG(...)
#endif

#if XM_BIG_ENDIAN
#error "Big endian platforms are not yet supported, sorry"
/* Make sure the compiler stops, even if #error is ignored */
extern int __fail[-1];
#endif

/* ----- XM constants ----- */

#define SAMPLE_NAME_LENGTH 22
#define INSTRUMENT_NAME_LENGTH 22
#define MODULE_NAME_LENGTH 20
#define TRACKER_NAME_LENGTH 20
#define PATTERN_ORDER_TABLE_LENGTH 256
#define NUM_NOTES 96
#define NUM_ENVELOPE_POINTS 12
#define MAX_NUM_ROWS 256

#if XM_RAMPING
#define XM_SAMPLE_RAMPING_POINTS 32
#endif

/* ----- Data types ----- */

enum xm_waveform_type_e {
	XM_SINE_WAVEFORM = 0,
	XM_RAMP_DOWN_WAVEFORM = 1,
	XM_SQUARE_WAVEFORM = 2,
	XM_RANDOM_WAVEFORM = 3,
	XM_RAMP_UP_WAVEFORM = 4,
};
typedef enum xm_waveform_type_e xm_waveform_type_t;

enum xm_loop_type_e {
	XM_NO_LOOP,
	XM_FORWARD_LOOP,
	XM_PING_PONG_LOOP,
};
typedef enum xm_loop_type_e xm_loop_type_t;

enum xm_frequency_type_e {
	XM_LINEAR_FREQUENCIES,
	XM_AMIGA_FREQUENCIES,
	XM_S3M_FREQUENCIES,
};

#define XM_EFFECT_S3M_SPEED 34
#define XM_EFFECT_S3M_TEMPO 35
#define XM_EFFECT_S3M_FINE_VIBRATO 36
#define XM_EFFECT_S3M_CHANNEL_VOLUME 37
#define XM_EFFECT_S3M_CHANNEL_VOLUME_SLIDE 38
#define XM_EFFECT_S3M_POSITION_JUMP 39
#define XM_EFFECT_S3M_PATTERN_BREAK 40
#define XM_EFFECT_S3M_VOLUME_SLIDE 41
#define XM_EFFECT_S3M_PORTAMENTO_DOWN 42
#define XM_EFFECT_S3M_PORTAMENTO_UP 43
#define XM_EFFECT_S3M_TONE_PORTAMENTO 44
#define XM_EFFECT_S3M_VIBRATO 45
#define XM_EFFECT_S3M_TREMOR 46
#define XM_EFFECT_S3M_ARPEGGIO 47
#define XM_EFFECT_S3M_VIBRATO_VOLUME_SLIDE 48
#define XM_EFFECT_S3M_TONE_PORTAMENTO_VOLUME_SLIDE 49
#define XM_EFFECT_S3M_SAMPLE_OFFSET 50
#define XM_EFFECT_S3M_PANNING_SLIDE 51
#define XM_EFFECT_S3M_RETRIG 52
#define XM_EFFECT_S3M_TREMOLO 53
#define XM_EFFECT_S3M_GLOBAL_VOLUME 54
#define XM_EFFECT_S3M_GLOBAL_VOLUME_SLIDE 55
#define XM_EFFECT_S3M_PANNING 56
#define XM_EFFECT_S3M_EXTENDED 57
#define XM_EFFECT_S3M_PANBRELLO 58
#define XM_EFFECT_S3M_MIDI_MACRO 59
#define XM_EFFECT_S3M_NONE 60
typedef enum xm_frequency_type_e xm_frequency_type_t;

struct xm_envelope_point_s {
	uint16_t frame;
	uint16_t value;
};
typedef struct xm_envelope_point_s xm_envelope_point_t;

struct xm_envelope_s {
	xm_envelope_point_t points[NUM_ENVELOPE_POINTS];
	uint8_t num_points;
	uint8_t sustain_point;
	uint8_t loop_start_point;
	uint8_t loop_end_point;
	bool enabled;
	bool sustain_enabled;
	bool loop_enabled;
};
typedef struct xm_envelope_s xm_envelope_t;

struct xm_sample_s {
#if XM_STRINGS
	char name[SAMPLE_NAME_LENGTH + 1];
#endif
	uint8_t bits; /* Either 8 or 16 */

	uint32_t length;
	uint32_t loop_start;
	uint32_t loop_length;
	uint32_t loop_end;
	float volume;
	int8_t finetune;
	xm_loop_type_t loop_type;
	float panning;
	int8_t relative_note;
	uint32_t c4speed; /* S3M C4Speed / OpenMPT c5speed field; used by S3M frequency mode. */
	uint64_t latest_trigger;

	union {
		int8_t* data8;
		int16_t* data16;
	};
};
typedef struct xm_sample_s xm_sample_t;

struct xm_instrument_s {
#if XM_STRINGS
	char name[INSTRUMENT_NAME_LENGTH + 1];
#endif
	uint16_t num_samples;
	uint8_t sample_of_notes[NUM_NOTES];
	xm_envelope_t volume_envelope;
	xm_envelope_t panning_envelope;
	xm_waveform_type_t vibrato_type;
	uint8_t vibrato_sweep;
	uint8_t vibrato_depth;
	uint8_t vibrato_rate;
	uint16_t volume_fadeout;
	uint64_t latest_trigger;
	bool muted;

	xm_sample_t* samples;
};
typedef struct xm_instrument_s xm_instrument_t;

struct xm_pattern_slot_s {
	uint16_t period; /* MOD raw Amiga period; 0 means XM note-based slot */
	uint8_t note; /* 1-96, 97 = Key Off note */
	uint8_t instrument; /* 1-128 */
	uint8_t volume_column;
	uint8_t effect_type;
	uint8_t effect_param;
};
typedef struct xm_pattern_slot_s xm_pattern_slot_t;

struct xm_pattern_s {
	uint16_t num_rows;
	xm_pattern_slot_t* slots; /* Array of size num_rows * num_channels */
};
typedef struct xm_pattern_s xm_pattern_t;

struct xm_mod_efx_backup_s {
	xm_sample_t* sample;
	int8_t* backup8;
	uint32_t loop_start;
	uint32_t loop_length;
};
typedef struct xm_mod_efx_backup_s xm_mod_efx_backup_t;

struct xm_module_s {
#if XM_STRINGS
	char name[MODULE_NAME_LENGTH + 1];
	char trackername[TRACKER_NAME_LENGTH + 1];
#endif
	uint16_t length;
	uint16_t restart_position;
	uint16_t num_channels;
	uint16_t num_patterns;
	uint16_t num_instruments;
	xm_frequency_type_t frequency_type;
	uint8_t pattern_table[PATTERN_ORDER_TABLE_LENGTH];

	xm_pattern_t* patterns;
	xm_instrument_t* instruments; /* Instrument 1 has index 0,
								   * instrument 2 has index 1, etc. */
};
typedef struct xm_module_s xm_module_t;

struct xm_channel_context_s {
	float note;
	float orig_note; /* The original note before effect modifications, as read in the pattern. */
	xm_instrument_t* instrument; /* Could be NULL */
	xm_sample_t* sample; /* Could be NULL */
	xm_pattern_slot_t* current;

	float sample_position;
	float period;
	float frequency;
	float step;
	bool ping; /* For ping-pong samples: true is -->, false is <-- */

	float volume; /* Ideally between 0 (muted) and 1 (loudest) */
	float channel_volume; /* S3M Mxx/Nxy channel global volume multiplier, 0..1. */
	float panning; /* Between 0 (left) and 1 (right); 0.5 is centered */
	float default_panning; /* < 0: use sample panning; >= 0: use channel default panning */
	bool surround; /* S3M XA4: inverse right channel phase around centered panning. */
	tracker_format_t tracker_format;
	float period_note_offset; /* MOD finetune offset for raw period slots */

	uint16_t autovibrato_ticks;

	bool sustained;
	float fadeout_volume;
	float volume_envelope_volume;
	float panning_envelope_panning;
	uint16_t volume_envelope_frame_count;
	uint16_t panning_envelope_frame_count;

	float autovibrato_note_offset;

	bool arp_in_progress;
	uint8_t arp_note_offset;
	uint8_t arpeggio_param;
	uint8_t volume_slide_param;
	uint8_t fine_volume_slide_param;
	uint8_t global_volume_slide_param;
	uint8_t channel_volume_slide_param;
	uint8_t panning_slide_param;
	uint8_t portamento_up_param;
	uint8_t portamento_down_param;
	uint8_t fine_portamento_up_param;
	uint8_t fine_portamento_down_param;
	uint8_t extra_fine_portamento_up_param;
	uint8_t extra_fine_portamento_down_param;
	uint8_t tone_portamento_param;
	float tone_portamento_target_period;
	uint8_t multi_retrig_param;
	uint8_t note_delay_param;
	uint8_t sample_offset_param;
	uint8_t s3m_high_sample_offset_param;
	uint8_t invert_loop_speed;
	uint16_t invert_loop_delay;
	uint32_t invert_loop_offset;
	uint8_t pattern_loop_origin; /* Where to restart a E6y loop */
	uint8_t pattern_loop_count; /* How many loop passes have been done */
	bool vibrato_in_progress;
	xm_waveform_type_t vibrato_waveform;
	bool vibrato_waveform_retrigger; /* True if a new note retriggers the waveform */
	uint8_t vibrato_param;
	uint16_t vibrato_ticks; /* Position in the waveform */
	float vibrato_note_offset;
	float vibrato_period_offset;
	xm_waveform_type_t tremolo_waveform;
	bool tremolo_waveform_retrigger;
	uint8_t tremolo_param;
	uint8_t tremolo_ticks;
	float tremolo_volume;
	xm_waveform_type_t panbrello_waveform;
	bool panbrello_waveform_retrigger;
	bool panbrello_in_progress;
	uint8_t panbrello_param;
	uint16_t panbrello_ticks;
	float panbrello_panning_offset;
	uint8_t tremor_param;
	uint8_t s3m_effect_memory;
	bool s3m_glissando_control;
	bool s3m_note_cut;
	bool tremor_on;
	uint8_t s3m_opl_key_on;
	uint8_t s3m_opl_instrument;
	uint32_t s3m_opl_trigger_id;
	uint32_t s3m_opl_synced_trigger_id;

	uint64_t latest_trigger;
	bool muted;

#if XM_RAMPING
	/* These values are updated at the end of each tick, to save
	 * a couple of float operations on every generated sample. */
	float target_panning;
	float target_volume;

	unsigned long frame_count;
	float end_of_previous_sample[XM_SAMPLE_RAMPING_POINTS];
#endif

	float actual_panning;
	float actual_volume;
};
typedef struct xm_channel_context_s xm_channel_context_t;

struct xm_context_s {
	size_t ctx_size; /* Must be first, see xm_create_context_from_libxmize() */
	xm_module_t module;
	tracker_format_t tracker_format;
  bool s3m_fast_volume_slides;
  uint8_t s3m_tempo_slide_param;
  uint16_t s3m_adlib_instrument_count;
  s3m_adlib_instrument_t *s3m_adlib_instruments;
  uint8_t *s3m_channel_opl;
	uint32_t rate;

	uint16_t tempo;
	uint16_t bpm;
	float global_volume;
	float amplification;

#if XM_RAMPING
	/* How much is a channel final volume allowed to change per
	 * sample; this is used to avoid abrubt volume changes which
	 * manifest as "clicks" in the generated sound. */
	float volume_ramp;
	float panning_ramp; /* Same for panning. */
#endif

	uint8_t current_table_index;
	uint8_t current_row;
	uint16_t current_tick; /* Can go below 255, with high tempo and a pattern delay */
	float remaining_samples_in_tick;
	uint64_t generated_samples;

	bool position_jump;
	bool pattern_break;
	uint8_t jump_dest;
	uint8_t jump_row;

	/* Extra ticks to be played before going to the next row -
	 * Used for EEy effect */
	uint16_t extra_ticks;

	uint8_t* row_loop_count; /* Array of size MAX_NUM_ROWS * module_length */
	uint8_t loop_count;
	uint8_t max_loop_count;

	xm_channel_context_t* channels;

	uint16_t mod_efx_backup_count;
	xm_mod_efx_backup_t* mod_efx_backups;
	int8_t* mod_efx_backup_data;

	uint32_t tracker_segments_magic;
	uint32_t tracker_segments_magic_check;
	uint16_t tracker_segment_count;
	tracker_context_segment_page_t* tracker_segment_pages;
};

/* ----- Internal API ----- */

/** Check the module data for errors/inconsistencies.
 *
 * @returns 0 if everything looks OK. Module should be safe to load.
 */
int xm_check_sanity_preload(const char*, size_t);

/** Check a loaded module for errors/inconsistencies.
 *
 * @returns 0 if everything looks OK.
 */
int xm_check_sanity_postload(xm_context_t*);

/** Get the number of bytes needed to store the module data in a
 * dynamically allocated blank context.
 *
 * Things that are dynamically allocated:
 * - sample data
 * - sample structures in instruments
 * - pattern data
 * - row loop count arrays
 * - pattern structures in module
 * - instrument structures in module
 * - channel contexts
 * - context structure itself

 * @returns 0 if everything looks OK.
 */
size_t xm_get_memory_needed_for_context(const char*, size_t);

/** Populate the context from module data.
 *
 * @returns non-zero on success
 */
int xm_load_module(xm_context_t*, const char*, size_t, xm_context_group_cursor_t*);

// ============================================================================
// MOD module support
// ============================================================================

#define MOD_TITLE_SIZE 20
#define MOD_SAMPLE_COUNT 31
#define MOD_15_SAMPLE_COUNT 15
#define MOD_15_SONG_LENGTH_OFFSET 470
#define MOD_15_RESTART_OFFSET 471
#define MOD_15_ORDER_OFFSET 472
#define MOD_15_HEADER_SIZE 600
#define MOD_SAMPLE_HEADER_SIZE 30
#define MOD_SONG_LENGTH_OFFSET 950
#define MOD_RESTART_OFFSET 951
#define MOD_ORDER_OFFSET 952
#define MOD_ORDER_COUNT 128
#define MOD_MAGIC_OFFSET 1080
#define MOD_HEADER_SIZE 1084
#define MOD_ROWS_PER_PATTERN 64
#define MOD_DEFAULT_TEMPO 6
#define MOD_DEFAULT_BPM 125
#define MOD_MAX_CHANNELS 32

struct xm_module_s;
struct xm_pattern_slot_s;

typedef struct
{
  uint16_t channels;
  uint16_t patterns;
  uint16_t song_length;
  uint16_t restart_position;
  uint16_t sample_count;
  size_t header_size;
  size_t pattern_data_offset;
  size_t sample_data_offset;
  uint32_t sample_length[MOD_SAMPLE_COUNT];
  uint32_t sample_loop_start[MOD_SAMPLE_COUNT];
  uint32_t sample_loop_length[MOD_SAMPLE_COUNT];
  uint32_t sample_file_length[MOD_SAMPLE_COUNT];
  size_t pattern_bytes;
  size_t sample_bytes;
  size_t expected_size;
  uint32_t efx_sample_mask;
  uint16_t efx_backup_count;
  size_t efx_backup_bytes;
} mod_layout_t;

int mod_create_context_safe(xm_context_t **ctxp, void *moddata, size_t moddata_length, uint32_t rate, MFUNC mfunc, size_t *out_bytes_needed);
int mod_check_sanity_preload(const void *moddata, size_t moddata_length);
uint16_t mod_get_channel_count(const void *moddata, size_t moddata_length);
int mod_read_layout_from_header(const uint8_t *header, size_t header_length, size_t moddata_length, mod_layout_t *layout);
size_t mod_get_memory_needed_for_layout(const mod_layout_t *layout);
int mod_layout_set_efx_backup_mask(mod_layout_t *layout, uint32_t sample_mask);
int mod_layout_set_efx_backup_all_looped(mod_layout_t *layout);
uint32_t mod_scan_efx_sample_mask_from_context(const xm_context_t *ctx, const mod_layout_t *layout);
uint32_t mod_scan_efx_sample_mask_from_data(const void *moddata, const mod_layout_t *layout);
int mod_get_group_sizes_for_layout(const mod_layout_t *layout, xm_context_group_sizes_t *sizes);
int mod_setup_module_header(xm_context_t *ctx, const uint8_t *header, const mod_layout_t *layout, xm_context_group_cursor_t *cursor);
int mod_setup_sample_metadata(xm_context_t *ctx, const uint8_t *header, const mod_layout_t *layout, uint16_t i, xm_context_group_cursor_t *cursor, MFUNC mfunc);
void mod_decode_pattern_slot(struct xm_pattern_slot_s *slot, const uint8_t *entry);
int mod_setup_context_runtime(xm_context_t *ctx, xm_context_group_cursor_t *cursor, uint32_t rate);
int mod_setup_efx_backups(xm_context_t *ctx, const mod_layout_t *layout, xm_context_group_cursor_t *cursor);
void mod_restore_efx_backups(xm_context_t *ctx);
int mod_finish_context(xm_context_t **ctxp, xm_context_t *ctx);

// ============================================================================
// S3M module support
// ============================================================================

#define S3M_HEADER_SIZE 96
#define S3M_MAGIC_OFFSET 44
#define S3M_MIN_CHANNELS 1
#define S3M_MAX_CHANNELS 32
#define S3M_MAX_ORDERS 256
#define S3M_MAX_SAMPLES 99
#define S3M_MAX_PATTERNS 256
#define S3M_SAMPLE_HEADER_SIZE 80
#define S3M_ROWS_PER_PATTERN 64
#define S3M_DEFAULT_C4SPEED 8363

int s3m_create_context_safe(xm_context_t **ctxp, void *s3mdata, size_t s3mdata_length, uint32_t rate, MFUNC mfunc, size_t *out_bytes_needed);
const char *s3m_get_last_error();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// ============================================================================
// ESP32 tracker task / CLI integration
// ============================================================================

enum
{
  TRACK_TASK_INIT,
  TRACK_TASK_PLAY,
  TRACK_TASK_STOP,
  TRACK_TASK_RESET,
  TRACK_TASK_SET_POS,
};

typedef struct
{
  uint8_t task;
  int handle;
} TRACK_TASK;

typedef struct
{
  uint8_t task;
  xm_context_t *ctx;
} PLAYER_TASK;

extern QueueHandle_t xm_queue;
extern int master_volume;
extern int mix_volume;
extern int curr_xm_handle;

int xm_cmd(int argc, char **argv);
void xm_task(void *arg);
void initialize_xm();
void xm_console_register_system_commands();
int xm_load_play_file(const char *path, bool quiet = false);
int xm_play_cmd(int handle, bool quiet = false);
int xm_stop_cmd(bool quiet = false);
int xm_reset_cmd(int handle, bool quiet = false);
esp_err_t xm_host_stream_prepare_command(size_t module_size, size_t rx_size);
esp_err_t mod_host_stream_prepare_command(size_t module_size, size_t rx_size);
esp_err_t s3m_host_stream_prepare_command(size_t module_size, size_t rx_size);
void xm_host_stream_abort_current();
void xm_host_stream_process_rx_data(const uint8_t *data, size_t size);

#endif
