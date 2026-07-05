#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <sys/stat.h>


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_console.h"

#include "sfx.h"
#include "mem_obj.h"
#include "esp_spi_defs.h"
#include "sdmmc.h"

enum
{
  SFX_CMD_PLAY,
  SFX_CMD_STOP,
  SFX_CMD_STOP_GROUP,
  SFX_CMD_STOP_HANDLE,
  SFX_CMD_SET_PARAMS,
  SFX_CMD_SET_VOLUME,
  SFX_CMD_GET_STATE,
};

#define SFX_GLOBAL_VOLUME_DEFAULT 50
#define SFX_GLOBAL_VOLUME_BASE 50

typedef struct
{
  u8 cmd;
  int handle;
  int channel;
  u8 group;
  u8 volume;
  u8 pan;
  u16 pitch;
  SFX_CHANNEL_STATE *state;
} SFX_COMMAND;

typedef struct
{
  esp_err_t err;
  int channel;
} SFX_COMMAND_ACK;

EXT_RAM_BSS_ATTR SFX_CHANNEL sfx_channels[SFX_CHANNELS] = {};
EXT_RAM_BSS_ATTR volatile SFX_CHANNEL_STATE sfx_channel_states[SFX_CHANNELS] = {};
volatile u8 sfx_global_volume_state = SFX_GLOBAL_VOLUME_DEFAULT;
u8 sfx_global_volume = SFX_GLOBAL_VOLUME_DEFAULT;
QueueHandle_t sfx_command_queue = NULL;
QueueHandle_t sfx_command_ack_queue = NULL;
SemaphoreHandle_t sfx_command_mtx = NULL;

void sfx_store_state_snapshot(int channel, const SFX_CHANNEL_STATE *state)
{
  if (channel < 0 || channel >= SFX_CHANNELS || !state) return;

  volatile SFX_CHANNEL_STATE *dst = &sfx_channel_states[channel];
  dst->active = state->active;
  dst->handle = state->handle;
  dst->group = state->group;
  dst->group_order = state->group_order;
  dst->volume = state->volume;
  dst->pan = state->pan;
  dst->pitch = state->pitch;
  dst->sample_rate = state->sample_rate;
  dst->frame_count = state->frame_count;
  dst->position = state->position;
  dst->bits_per_sample = state->bits_per_sample;
  dst->channels = state->channels;
  dst->is_signed = state->is_signed;
}

void sfx_update_state_snapshots()
{
  sfx_global_volume_state = sfx_global_volume;

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    SFX_CHANNEL_STATE state = {};
    sfx_get_state(i, &state);
    sfx_store_state_snapshot(i, &state);
  }
}

void sfx_init()
{
  memset(sfx_channels, 0, sizeof(sfx_channels));
  memset((void *)sfx_channel_states, 0, sizeof(sfx_channel_states));
  sfx_global_volume = SFX_GLOBAL_VOLUME_DEFAULT;
  sfx_global_volume_state = SFX_GLOBAL_VOLUME_DEFAULT;

  if (!sfx_command_queue)
    sfx_command_queue = xQueueCreateWithCaps(8, sizeof(SFX_COMMAND), task_ram_type_non_critical);

  if (!sfx_command_ack_queue)
    sfx_command_ack_queue = xQueueCreateWithCaps(1, sizeof(SFX_COMMAND_ACK), task_ram_type_non_critical);

  if (!sfx_command_mtx)
    sfx_command_mtx = xSemaphoreCreateMutex();

  sfx_update_state_snapshots();
}

void sfx_compact_group(u8 group)
{
  u8 order = 1;

  while (1)
  {
    int oldest_channel = -1;
    u8 oldest_order = 0xFF;

    for (int i = 0; i < SFX_CHANNELS; i++)
    {
      if (!sfx_channels[i].active || sfx_channels[i].group != group) continue;
      if (sfx_channels[i].group_order >= order && sfx_channels[i].group_order < oldest_order)
      {
        oldest_channel = i;
        oldest_order = sfx_channels[i].group_order;
      }
    }

    if (oldest_channel < 0) break;

    sfx_channels[oldest_channel].group_order = order;
    order++;
  }
}

int sfx_wav_handle_has_users(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return 0;

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    if (sfx_channels[i].active && sfx_channels[i].handle == handle) return 1;
  }

  return 0;
}

void sfx_update_wav_obj_state(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return;
  if (!check_handle(handle) || mem_obj[handle].type != OBJ_TYPE_WAV) return;
  if (mem_obj[handle].state == OBJ_ST_DELETING) return;

  mem_obj[handle].state = sfx_wav_handle_has_users(handle) ? WAV_OBJ_ST_PLAYING : OBJ_ST_NONE;
}

void sfx_clear_channel(int channel)
{
  int handle = sfx_channels[channel].handle;
  u8 group = sfx_channels[channel].group;

  memset(&sfx_channels[channel], 0, sizeof(sfx_channels[channel]));
  sfx_compact_group(group);
  sfx_update_wav_obj_state(handle);
}

esp_err_t sfx_parse_wav_object(int handle, SFX_WAV_INFO *info)
{
  if (!info) return ESP_ERR_INVALID_ARG;
  memset(info, 0, sizeof(SFX_WAV_INFO));

  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return ESP_ERR_INVALID_ARG;
  if (!check_handle(handle)) return ESP_ERR_NOT_FOUND;
  if (mem_obj[handle].type != OBJ_TYPE_WAV) return ESP_ERR_INVALID_ARG;
  if (mem_obj[handle].state == OBJ_ST_DELETING) return ESP_ERR_INVALID_STATE;
  if (!mem_obj[handle].addr || mem_obj[handle].size < 12) return ESP_ERR_INVALID_SIZE;

  const u8 *wav = (const u8 *)mem_obj[handle].addr;
  u32 size = (u32)mem_obj[handle].size;

  if (memcmp(wav, "RIFF", 4) != 0 || memcmp(wav + 8, "WAVE", 4) != 0) return ESP_ERR_INVALID_RESPONSE;

  bool have_fmt = false;
  bool have_data = false;
  u16 format_tag = 0;
  u16 channels = 0;
  u32 sample_rate = 0;
  u16 block_align = 0;
  u16 bits_per_sample = 0;
  u32 data_offset = 0;
  u32 data_size = 0;
  u32 pos = 12;

  while (pos + 8 <= size)
  {
    const u8 *chunk = wav + pos;
    u32 chunk_size = (u32)chunk[4] | ((u32)chunk[5] << 8) | ((u32)chunk[6] << 16) | ((u32)chunk[7] << 24);
    u32 chunk_data = pos + 8;

    if (chunk_data > size || chunk_size > size - chunk_data) return ESP_ERR_INVALID_SIZE;

    if (memcmp(chunk, "fmt ", 4) == 0)
    {
      if (chunk_size < 16) return ESP_ERR_INVALID_SIZE;

      format_tag = (u16)wav[chunk_data] | ((u16)wav[chunk_data + 1] << 8);
      channels = (u16)wav[chunk_data + 2] | ((u16)wav[chunk_data + 3] << 8);
      sample_rate = (u32)wav[chunk_data + 4] | ((u32)wav[chunk_data + 5] << 8) | ((u32)wav[chunk_data + 6] << 16) | ((u32)wav[chunk_data + 7] << 24);
      block_align = (u16)wav[chunk_data + 12] | ((u16)wav[chunk_data + 13] << 8);
      bits_per_sample = (u16)wav[chunk_data + 14] | ((u16)wav[chunk_data + 15] << 8);
      have_fmt = true;
    }
    else if (memcmp(chunk, "data", 4) == 0)
    {
      data_offset = chunk_data;
      data_size = chunk_size;
      have_data = true;
      break;
    }

    pos = chunk_data + chunk_size + (chunk_size & 1);
  }

  if (!have_fmt || !have_data) return ESP_ERR_NOT_FOUND;
  if (format_tag != 1) return ESP_ERR_NOT_SUPPORTED;
  if (channels != 1 && channels != 2) return ESP_ERR_NOT_SUPPORTED;
  if (sample_rate == 0) return ESP_ERR_INVALID_ARG;
  if (bits_per_sample != 8 && bits_per_sample != 16) return ESP_ERR_NOT_SUPPORTED;

  u16 expected_block_align = channels * (bits_per_sample / 8);
  if (block_align != expected_block_align) return ESP_ERR_INVALID_SIZE;
  if (data_size < block_align) return ESP_ERR_INVALID_SIZE;

  info->data = wav + data_offset;
  info->data_size = data_size;
  info->frame_count = data_size / block_align;
  info->sample_rate = sample_rate;
  info->block_align = block_align;
  info->bits_per_sample = bits_per_sample;
  info->channels = channels;
  info->is_signed = bits_per_sample != 8;

  if (info->frame_count == 0) return ESP_ERR_INVALID_SIZE;

  return ESP_OK;
}

esp_err_t sfx_play(int handle, u8 group, u8 volume, u8 pan, u16 pitch, int *out_channel)
{
  if (!out_channel) return ESP_ERR_INVALID_ARG;
  *out_channel = -1;
  if (pitch == 0) return ESP_ERR_INVALID_ARG;

  SFX_WAV_INFO wav = {};
  esp_err_t err = sfx_parse_wav_object(handle, &wav);
  if (err != ESP_OK) return err;

  int channel = -1;

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    if (!sfx_channels[i].active)
    {
      channel = i;
      break;
    }
  }

  if (channel < 0)
  {
    u8 oldest_order = 0xFF;

    for (int i = 0; i < SFX_CHANNELS; i++)
    {
      if (sfx_channels[i].group != group) continue;
      if (sfx_channels[i].group_order < oldest_order)
      {
        channel = i;
        oldest_order = sfx_channels[i].group_order;
      }
    }

    if (channel < 0) return ESP_ERR_NO_MEM;
  }

  if (sfx_channels[channel].active) sfx_clear_channel(channel);

  u8 group_order = 1;

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    if (!sfx_channels[i].active || sfx_channels[i].group != group) continue;
    if (sfx_channels[i].group_order >= group_order) group_order = sfx_channels[i].group_order + 1;
  }

  sfx_channels[channel].active = 1;
  sfx_channels[channel].handle = handle;
  sfx_channels[channel].group = group;
  sfx_channels[channel].group_order = group_order;
  sfx_channels[channel].volume = volume;
  sfx_channels[channel].pan = pan;
  sfx_channels[channel].pitch = pitch;
  sfx_channels[channel].position = 0.0f;
  sfx_channels[channel].wav = wav;

  sfx_compact_group(group);
  sfx_update_wav_obj_state(handle);
  *out_channel = channel;

  return ESP_OK;
}

esp_err_t sfx_stop(int channel)
{
  if (channel < 0 || channel >= SFX_CHANNELS) return ESP_ERR_INVALID_ARG;
  if (!sfx_channels[channel].active) return ESP_ERR_NOT_FOUND;

  sfx_clear_channel(channel);

  return ESP_OK;
}

esp_err_t sfx_stop_group(u8 group)
{
  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    if (!sfx_channels[i].active || sfx_channels[i].group != group) continue;
    sfx_clear_channel(i);
  }

  return ESP_OK;
}

esp_err_t sfx_stop_handle(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return ESP_ERR_INVALID_ARG;

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    if (!sfx_channels[i].active || sfx_channels[i].handle != handle) continue;
    sfx_clear_channel(i);
  }

  return ESP_OK;
}

esp_err_t sfx_set_params(int channel, u8 volume, u8 pan, u16 pitch)
{
  if (channel < 0 || channel >= SFX_CHANNELS) return ESP_ERR_INVALID_ARG;
  if (!sfx_channels[channel].active) return ESP_ERR_NOT_FOUND;
  if (pitch == 0) return ESP_ERR_INVALID_ARG;

  sfx_channels[channel].volume = volume;
  sfx_channels[channel].pan = pan;
  sfx_channels[channel].pitch = pitch;

  return ESP_OK;
}

esp_err_t sfx_set_volume(u8 volume)
{
  sfx_global_volume = volume;

  return ESP_OK;
}

esp_err_t sfx_command_sync(SFX_COMMAND *cmd, SFX_COMMAND_ACK *ack)
{
  if (!cmd || !ack) return ESP_ERR_INVALID_ARG;
  if (!sfx_command_queue || !sfx_command_ack_queue || !sfx_command_mtx) return ESP_ERR_INVALID_STATE;

  if (xSemaphoreTake(sfx_command_mtx, portMAX_DELAY) != pdTRUE)
    return ESP_ERR_TIMEOUT;

  while (xQueueReceive(sfx_command_ack_queue, ack, 0) == pdTRUE);

  if (xQueueSend(sfx_command_queue, cmd, portMAX_DELAY) != pdTRUE)
  {
    xSemaphoreGive(sfx_command_mtx);
    return ESP_ERR_TIMEOUT;
  }

  if (xQueueReceive(sfx_command_ack_queue, ack, portMAX_DELAY) != pdTRUE)
  {
    xSemaphoreGive(sfx_command_mtx);
    return ESP_ERR_TIMEOUT;
  }

  xSemaphoreGive(sfx_command_mtx);

  return ack->err;
}

esp_err_t sfx_play_sync(int handle, u8 group, u8 volume, u8 pan, u16 pitch, int *out_channel)
{
  if (!out_channel) return ESP_ERR_INVALID_ARG;
  *out_channel = -1;

  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_PLAY;
  cmd.handle = handle;
  cmd.group = group;
  cmd.volume = volume;
  cmd.pan = pan;
  cmd.pitch = pitch;

  esp_err_t err = sfx_command_sync(&cmd, &ack);
  if (err == ESP_OK) *out_channel = ack.channel;
  return err;
}

esp_err_t sfx_stop_sync(int channel)
{
  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_STOP;
  cmd.channel = channel;

  return sfx_command_sync(&cmd, &ack);
}

esp_err_t sfx_stop_group_sync(u8 group)
{
  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_STOP_GROUP;
  cmd.group = group;

  return sfx_command_sync(&cmd, &ack);
}

esp_err_t sfx_stop_handle_sync(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return ESP_ERR_INVALID_ARG;
  if (!sfx_command_queue || !sfx_command_ack_queue || !sfx_command_mtx) return ESP_OK;

  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_STOP_HANDLE;
  cmd.handle = handle;

  return sfx_command_sync(&cmd, &ack);
}

esp_err_t sfx_set_params_sync(int channel, u8 volume, u8 pan, u16 pitch)
{
  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_SET_PARAMS;
  cmd.channel = channel;
  cmd.volume = volume;
  cmd.pan = pan;
  cmd.pitch = pitch;

  return sfx_command_sync(&cmd, &ack);
}

esp_err_t sfx_set_volume_sync(u8 volume)
{
  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_SET_VOLUME;
  cmd.volume = volume;

  return sfx_command_sync(&cmd, &ack);
}

esp_err_t sfx_get_state_sync(int channel, SFX_CHANNEL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;

  SFX_COMMAND cmd = {};
  SFX_COMMAND_ACK ack = {};

  cmd.cmd = SFX_CMD_GET_STATE;
  cmd.channel = channel;
  cmd.state = state;

  return sfx_command_sync(&cmd, &ack);
}

void sfx_process_commands()
{
  if (!sfx_command_queue || !sfx_command_ack_queue) return;

  SFX_COMMAND cmd;

  while (xQueueReceive(sfx_command_queue, &cmd, 0) == pdTRUE)
  {
    SFX_COMMAND_ACK ack = {};
    ack.channel = -1;

    switch (cmd.cmd)
    {
      case SFX_CMD_PLAY:
        ack.err = sfx_play(cmd.handle, cmd.group, cmd.volume, cmd.pan, cmd.pitch, &ack.channel);
      break;

      case SFX_CMD_STOP:
        ack.err = sfx_stop(cmd.channel);
      break;

      case SFX_CMD_STOP_GROUP:
        ack.err = sfx_stop_group(cmd.group);
      break;

      case SFX_CMD_STOP_HANDLE:
        ack.err = sfx_stop_handle(cmd.handle);
      break;

      case SFX_CMD_SET_PARAMS:
        ack.err = sfx_set_params(cmd.channel, cmd.volume, cmd.pan, cmd.pitch);
      break;

      case SFX_CMD_SET_VOLUME:
        ack.err = sfx_set_volume(cmd.volume);
      break;

      case SFX_CMD_GET_STATE:
        ack.err = sfx_get_state(cmd.channel, cmd.state);
      break;

      default:
        ack.err = ESP_ERR_INVALID_ARG;
      break;
    }

    sfx_update_state_snapshots();
    xQueueOverwrite(sfx_command_ack_queue, &ack);
  }
}

void sfx_render_pcm_u8_mono_unsigned(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    float frac = pos - (float)frame;
    float sample0 = ((float)src[frame] - 128.0f) / 128.0f;
    float sample1 = ((float)src[next_frame] - 128.0f) / 128.0f;
    float sample = sample0 + (sample1 - sample0) * frac;

    mix[2 * i] += sample * left_gain;
    mix[2 * i + 1] += sample * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_u8_mono_signed(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const i8 *src = (const i8 *)ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    float frac = pos - (float)frame;
    float sample0 = (float)src[frame] / 128.0f;
    float sample1 = (float)src[next_frame] / 128.0f;
    float sample = sample0 + (sample1 - sample0) * frac;

    mix[2 * i] += sample * left_gain;
    mix[2 * i + 1] += sample * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_u8_stereo_unsigned(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const u8 *sample0 = src + frame * 2;
    const u8 *sample1 = src + next_frame * 2;
    float frac = pos - (float)frame;
    float left0 = ((float)sample0[0] - 128.0f) / 128.0f;
    float right0 = ((float)sample0[1] - 128.0f) / 128.0f;
    float left1 = ((float)sample1[0] - 128.0f) / 128.0f;
    float right1 = ((float)sample1[1] - 128.0f) / 128.0f;
    float left = left0 + (left1 - left0) * frac;
    float right = right0 + (right1 - right0) * frac;

    mix[2 * i] += left * left_gain;
    mix[2 * i + 1] += right * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_u8_stereo_signed(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const i8 *src = (const i8 *)ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const i8 *sample0 = src + frame * 2;
    const i8 *sample1 = src + next_frame * 2;
    float frac = pos - (float)frame;
    float left0 = (float)sample0[0] / 128.0f;
    float right0 = (float)sample0[1] / 128.0f;
    float left1 = (float)sample1[0] / 128.0f;
    float right1 = (float)sample1[1] / 128.0f;
    float left = left0 + (left1 - left0) * frac;
    float right = right0 + (right1 - right0) * frac;

    mix[2 * i] += left * left_gain;
    mix[2 * i + 1] += right * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_u16_mono_unsigned(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const u8 *sample0 = src + frame * 2;
    const u8 *sample1 = src + next_frame * 2;
    u16 value0 = (u16)sample0[0] | ((u16)sample0[1] << 8);
    u16 value1 = (u16)sample1[0] | ((u16)sample1[1] << 8);
    float frac = pos - (float)frame;
    float s0 = ((float)value0 - 32768.0f) / 32768.0f;
    float s1 = ((float)value1 - 32768.0f) / 32768.0f;
    float sample = s0 + (s1 - s0) * frac;

    mix[2 * i] += sample * left_gain;
    mix[2 * i + 1] += sample * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_s16_mono_signed(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const u8 *sample0 = src + frame * 2;
    const u8 *sample1 = src + next_frame * 2;
    u16 value0 = (u16)sample0[0] | ((u16)sample0[1] << 8);
    u16 value1 = (u16)sample1[0] | ((u16)sample1[1] << 8);
    float frac = pos - (float)frame;
    float s0 = (float)(i16)value0 / 32768.0f;
    float s1 = (float)(i16)value1 / 32768.0f;
    float sample = s0 + (s1 - s0) * frac;

    mix[2 * i] += sample * left_gain;
    mix[2 * i + 1] += sample * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_u16_stereo_unsigned(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const u8 *sample0 = src + frame * 4;
    const u8 *sample1 = src + next_frame * 4;
    u16 left_value0 = (u16)sample0[0] | ((u16)sample0[1] << 8);
    u16 right_value0 = (u16)sample0[2] | ((u16)sample0[3] << 8);
    u16 left_value1 = (u16)sample1[0] | ((u16)sample1[1] << 8);
    u16 right_value1 = (u16)sample1[2] | ((u16)sample1[3] << 8);
    float frac = pos - (float)frame;
    float left0 = ((float)left_value0 - 32768.0f) / 32768.0f;
    float right0 = ((float)right_value0 - 32768.0f) / 32768.0f;
    float left1 = ((float)left_value1 - 32768.0f) / 32768.0f;
    float right1 = ((float)right_value1 - 32768.0f) / 32768.0f;
    float left = left0 + (left1 - left0) * frac;
    float right = right0 + (right1 - right0) * frac;

    mix[2 * i] += left * left_gain;
    mix[2 * i + 1] += right * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render_pcm_s16_stereo_signed(SFX_CHANNEL *ch, float *mix, int sample_count, float step, float left_gain, float right_gain)
{
  const u8 *src = ch->wav.data;
  float pos = ch->position;

  for (int i = 0; i < sample_count; i++)
  {
    u32 frame = (u32)pos;
    if (frame >= ch->wav.frame_count) break;

    u32 next_frame = frame + 1;
    if (next_frame >= ch->wav.frame_count) next_frame = frame;

    const u8 *sample0 = src + frame * 4;
    const u8 *sample1 = src + next_frame * 4;
    u16 left_value0 = (u16)sample0[0] | ((u16)sample0[1] << 8);
    u16 right_value0 = (u16)sample0[2] | ((u16)sample0[3] << 8);
    u16 left_value1 = (u16)sample1[0] | ((u16)sample1[1] << 8);
    u16 right_value1 = (u16)sample1[2] | ((u16)sample1[3] << 8);
    float frac = pos - (float)frame;
    float left0 = (float)(i16)left_value0 / 32768.0f;
    float right0 = (float)(i16)right_value0 / 32768.0f;
    float left1 = (float)(i16)left_value1 / 32768.0f;
    float right1 = (float)(i16)right_value1 / 32768.0f;
    float left = left0 + (left1 - left0) * frac;
    float right = right0 + (right1 - right0) * frac;

    mix[2 * i] += left * left_gain;
    mix[2 * i + 1] += right * right_gain;
    pos += step;
  }

  ch->position = pos;
}

void sfx_render(float *mix, int sample_count, int output_sample_rate)
{
  if (!mix || sample_count <= 0 || output_sample_rate <= 0) return;

  sfx_process_commands();

  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    SFX_CHANNEL *ch = &sfx_channels[i];
    if (!ch->active) continue;

    if (!ch->wav.data || ch->wav.frame_count == 0 || ch->wav.sample_rate == 0 || ch->pitch == 0)
    {
      sfx_clear_channel(i);
      continue;
    }

    float step = ((float)ch->wav.sample_rate * (float)ch->pitch) / ((float)output_sample_rate * 256.0f);
    float gain = 32767.0f * (float)sfx_global_volume * (float)ch->volume / ((float)SFX_GLOBAL_VOLUME_BASE * 255.0f);
    float left_gain = gain;
    float right_gain = gain;

    if (ch->pan < SFX_PAN_CENTER)
    {
      right_gain *= (float)ch->pan / (float)SFX_PAN_CENTER;
    }
    else if (ch->pan > SFX_PAN_CENTER)
    {
      left_gain *= (float)(255 - ch->pan) / (float)(255 - SFX_PAN_CENTER);
    }

    if (ch->wav.bits_per_sample == 8)
    {
      if (ch->wav.channels == 1)
      {
        if (ch->wav.is_signed)
        {
          sfx_render_pcm_u8_mono_signed(ch, mix, sample_count, step, left_gain, right_gain);
        }
        else
        {
          sfx_render_pcm_u8_mono_unsigned(ch, mix, sample_count, step, left_gain, right_gain);
        }
      }
      else
      {
        if (ch->wav.is_signed)
        {
          sfx_render_pcm_u8_stereo_signed(ch, mix, sample_count, step, left_gain, right_gain);
        }
        else
        {
          sfx_render_pcm_u8_stereo_unsigned(ch, mix, sample_count, step, left_gain, right_gain);
        }
      }
    }
    else if (ch->wav.bits_per_sample == 16)
    {
      if (ch->wav.channels == 1)
      {
        if (ch->wav.is_signed)
        {
          sfx_render_pcm_s16_mono_signed(ch, mix, sample_count, step, left_gain, right_gain);
        }
        else
        {
          sfx_render_pcm_u16_mono_unsigned(ch, mix, sample_count, step, left_gain, right_gain);
        }
      }
      else
      {
        if (ch->wav.is_signed)
        {
          sfx_render_pcm_s16_stereo_signed(ch, mix, sample_count, step, left_gain, right_gain);
        }
        else
        {
          sfx_render_pcm_u16_stereo_unsigned(ch, mix, sample_count, step, left_gain, right_gain);
        }
      }
    }
    else
    {
      sfx_clear_channel(i);
      continue;
    }

    if (ch->position >= (float)ch->wav.frame_count) sfx_clear_channel(i);
  }

  sfx_update_state_snapshots();
}

esp_err_t sfx_get_state(int channel, SFX_CHANNEL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  memset(state, 0, sizeof(SFX_CHANNEL_STATE));

  if (channel < 0 || channel >= SFX_CHANNELS) return ESP_ERR_INVALID_ARG;

  SFX_CHANNEL *ch = &sfx_channels[channel];

  state->active = ch->active;
  state->handle = ch->handle;
  state->group = ch->group;
  state->group_order = ch->group_order;
  state->volume = ch->volume;
  state->pan = ch->pan;
  state->pitch = ch->pitch;
  state->sample_rate = ch->wav.sample_rate;
  state->frame_count = ch->wav.frame_count;
  state->position = (u32)ch->position;
  state->bits_per_sample = ch->wav.bits_per_sample;
  state->channels = ch->wav.channels;
  state->is_signed = ch->wav.is_signed;

  return ESP_OK;
}

esp_err_t sfx_get_state_snapshot(int channel, SFX_CHANNEL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  memset(state, 0, sizeof(SFX_CHANNEL_STATE));

  if (channel < 0 || channel >= SFX_CHANNELS) return ESP_ERR_INVALID_ARG;

  volatile SFX_CHANNEL_STATE *src = &sfx_channel_states[channel];
  state->active = src->active;
  state->handle = src->handle;
  state->group = src->group;
  state->group_order = src->group_order;
  state->volume = src->volume;
  state->pan = src->pan;
  state->pitch = src->pitch;
  state->sample_rate = src->sample_rate;
  state->frame_count = src->frame_count;
  state->position = src->position;
  state->bits_per_sample = src->bits_per_sample;
  state->channels = src->channels;
  state->is_signed = src->is_signed;

  return ESP_OK;
}

// -------------------- Console --------------------

bool sfx_parse_ulong_arg(const char *s, unsigned long max_value, unsigned long *out_value)
{
  if (!s || !s[0] || !out_value) return false;

  char *endp = NULL;
  unsigned long value = strtoul(s, &endp, 0);
  if (!endp || *endp || value > max_value) return false;

  *out_value = value;
  return true;
}

const char *sfx_signedness_str(u8 is_signed)
{
  return is_signed ? "signed" : "unsigned";
}

void sfx_print_wav_info(int handle, const SFX_WAV_INFO *info)
{
  if (!info) return;

  printf("WAV %02X: %u Hz, %u-bit, %s, %s, frames=%u, data=%u\r\n",
    handle,
    (unsigned)info->sample_rate,
    (unsigned)info->bits_per_sample,
    info->channels == 1 ? "mono" : "stereo",
    sfx_signedness_str(info->is_signed),
    (unsigned)info->frame_count,
    (unsigned)info->data_size);
}

esp_err_t sfx_load_file(const char *path, int *out_handle, SFX_WAV_INFO *out_info)
{
  if (!path || !path[0] || !out_handle) return ESP_ERR_INVALID_ARG;

  *out_handle = -1;
  if (out_info) memset(out_info, 0, sizeof(SFX_WAV_INFO));

  const char *open_path = path;
  char full[256];
  bool is_sd_path = !strncmp(path, "/sd", 3) && (path[3] == 0 || path[3] == '/');
  bool tried_sd_mount = false;

  struct stat st = {};
  if (stat(open_path, &st) != 0)
  {
    esp_err_t err = ESP_FAIL;

    if (is_sd_path || path[0] != '/')
    {
      tried_sd_mount = true;
      err = sd_fs_mount("/sd", NULL);
      if (err == ESP_OK && !is_sd_path)
      {
        if (!sd_fs_build_full_path("/sd", path, full, sizeof(full))) return ESP_ERR_INVALID_SIZE;
        open_path = full;
      }
    }

    if (tried_sd_mount && err != ESP_OK) return err;
    if (stat(open_path, &st) != 0) return ESP_ERR_NOT_FOUND;
  }

  if (st.st_size <= 0 || st.st_size > INT_MAX) return ESP_ERR_INVALID_SIZE;

  int handle = make_obj((int)st.st_size, OBJ_TYPE_WAV);
  if (handle < 0) return ESP_ERR_NO_MEM;

  FILE *fp = fopen(open_path, "rb");
  if (!fp)
  {
    delete_obj(handle);
    return ESP_ERR_NOT_FOUND;
  }

  size_t read_size = fread(mem_obj[handle].addr, 1, (size_t)st.st_size, fp);
  fclose(fp);

  if (read_size != (size_t)st.st_size)
  {
    delete_obj(handle);
    return ESP_FAIL;
  }

  SFX_WAV_INFO info = {};
  esp_err_t err = sfx_parse_wav_object(handle, &info);
  if (err != ESP_OK)
  {
    delete_obj(handle);
    return err;
  }

  *out_handle = handle;
  if (out_info) *out_info = info;
  return ESP_OK;
}

int sfx_load_cmd(const char *path)
{
  if (!path || !path[0])
  {
    printf("Usage: sfx load <file.wav>\r\n");
    return 1;
  }

  int handle = -1;
  SFX_WAV_INFO info = {};
  esp_err_t err = sfx_load_file(path, &handle, &info);
  if (err != ESP_OK)
  {
    printf("E: WAV load failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("WAV loaded: handle=%02X size=%u\r\n", handle, (unsigned)mem_obj[handle].size);
  sfx_print_wav_info(handle, &info);
  return 0;
}

int sfx_play_cmd(int argc, char **argv)
{
  unsigned long value = 0;

  if (argc < 3 || !sfx_parse_ulong_arg(argv[2], OBJ_HANDLES_MAX - 1, &value))
  {
    printf("Usage: sfx play <handle> [group] [volume] [pan] [pitch]\r\n");
    return 1;
  }

  int handle = (int)value;
  u8 group = 0;
  u8 volume = SFX_VOLUME_MAX;
  u8 pan = SFX_PAN_CENTER;
  u16 pitch = SFX_PITCH_ONE;

  if (argc >= 4)
  {
    if (!sfx_parse_ulong_arg(argv[3], 255, &value))
    {
      printf("Bad <group>: %s\r\n", argv[3]);
      return 1;
    }
    group = (u8)value;
  }

  if (argc >= 5)
  {
    if (!sfx_parse_ulong_arg(argv[4], SFX_VOLUME_MAX, &value))
    {
      printf("Bad <volume>: %s (expected 0..255)\r\n", argv[4]);
      return 1;
    }
    volume = (u8)value;
  }

  if (argc >= 6)
  {
    if (!sfx_parse_ulong_arg(argv[5], 255, &value))
    {
      printf("Bad <pan>: %s (expected 0..255)\r\n", argv[5]);
      return 1;
    }
    pan = (u8)value;
  }

  if (argc >= 7)
  {
    const char *p = argv[6];
    unsigned long integer = 0;
    unsigned long fraction = 0;
    unsigned long scale = 1;
    bool got_digit = false;

    while (*p >= '0' && *p <= '9')
    {
      got_digit = true;
      integer = integer * 10 + (unsigned long)(*p - '0');
      if (integer > 15) break;
      p++;
    }

    if (*p == '.')
    {
      p++;
      while (*p >= '0' && *p <= '9')
      {
        got_digit = true;
        if (scale < 1000000)
        {
          fraction = fraction * 10 + (unsigned long)(*p - '0');
          scale *= 10;
        }
        p++;
      }
    }

    unsigned long pitch_value = integer * 256 + (fraction * 256 + scale / 2) / scale;
    if (!got_digit || *p || pitch_value == 0 || pitch_value > 0x0FFF)
    {
      printf("Bad <pitch>: %s (expected 0.004..15.996, example: 1.0 or 1.5)\r\n", argv[6]);
      return 1;
    }
    pitch = (u16)pitch_value;
  }

  int channel = -1;
  esp_err_t err = sfx_play_sync(handle, group, volume, pan, pitch, &channel);
  if (err != ESP_OK)
  {
    printf("SFX play failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  unsigned pitch_integer = (unsigned)(pitch >> 8);
  unsigned pitch_fraction = ((unsigned)(pitch & 0xFF) * 1000 + 128) >> 8;

  printf("SFX playing: handle=%02X channel=%u group=%u volume=%u pan=%u pitch=%u.%03u (0x%04X)\r\n",
    handle,
    (unsigned)channel,
    (unsigned)group,
    (unsigned)volume,
    (unsigned)pan,
    pitch_integer,
    pitch_fraction,
    (unsigned)pitch);
  return 0;
}

int sfx_stop_cmd(int argc, char **argv)
{
  unsigned long value = 0;

  if (argc >= 3 && !strcmp(argv[2], "group"))
  {
    if (argc < 4 || !sfx_parse_ulong_arg(argv[3], 255, &value))
    {
      printf("Usage: sfx stop group <group>\r\n");
      return 1;
    }

    esp_err_t err = sfx_stop_group_sync((u8)value);
    if (err != ESP_OK)
    {
      printf("SFX stop group failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    printf("SFX group stopped: %u\r\n", (unsigned)value);
    return 0;
  }

  if (argc < 3 || !sfx_parse_ulong_arg(argv[2], SFX_CHANNELS - 1, &value))
  {
    printf("Usage: sfx stop <channel>\r\n");
    printf("       sfx stop group <group>\r\n");
    return 1;
  }

  esp_err_t err = sfx_stop_sync((int)value);
  if (err != ESP_OK)
  {
    printf("SFX stop failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("SFX stopped: channel=%u\r\n", (unsigned)value);
  return 0;
}

void sfx_print_channel_state(int channel, const SFX_CHANNEL_STATE *state)
{
  if (!state) return;

  if (!state->active)
  {
    printf("%02u  free\r\n", (unsigned)channel);
    return;
  }

  unsigned pitch_integer = (unsigned)(state->pitch >> 8);
  unsigned pitch_fraction = ((unsigned)(state->pitch & 0xFF) * 1000 + 128) >> 8;

  printf("%02u  handle=%02X group=%u order=%u vol=%u pan=%u pitch=%u.%03u (0x%04X) pos=%u/%u %uHz %u-bit %s %s\r\n",
    (unsigned)channel,
    (unsigned)state->handle,
    (unsigned)state->group,
    (unsigned)state->group_order,
    (unsigned)state->volume,
    (unsigned)state->pan,
    pitch_integer,
    pitch_fraction,
    (unsigned)state->pitch,
    (unsigned)state->position,
    (unsigned)state->frame_count,
    (unsigned)state->sample_rate,
    (unsigned)state->bits_per_sample,
    state->channels == 1 ? "mono" : "stereo",
    sfx_signedness_str(state->is_signed));
}

int sfx_vol_cmd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("SFX global volume: %u\r\n", (unsigned)sfx_global_volume_state);
    return 0;
  }

  unsigned long value = 0;
  if (!sfx_parse_ulong_arg(argv[2], SFX_VOLUME_MAX, &value))
  {
    printf("Bad <volume>: %s (expected 0..255)\r\n", argv[2]);
    return 1;
  }

  esp_err_t err = sfx_set_volume_sync((u8)value);
  if (err != ESP_OK)
  {
    printf("SFX volume failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("SFX global volume: %u\r\n", (unsigned)value);
  return 0;
}

int sfx_info_cmd(int argc, char **argv)
{
  unsigned long value = 0;

  if (argc >= 3)
  {
    if (!sfx_parse_ulong_arg(argv[2], SFX_CHANNELS - 1, &value))
    {
      printf("Usage: sfx info [channel]\r\n");
      return 1;
    }

    SFX_CHANNEL_STATE state = {};
    esp_err_t err = sfx_get_state_snapshot((int)value, &state);
    if (err != ESP_OK)
    {
      printf("SFX info failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    sfx_print_channel_state((int)value, &state);
    return 0;
  }

  printf("\r\nSFX global volume: %u\r\n", (unsigned)sfx_global_volume_state);
  printf("\r\nSFX channels\r\n");

  int active = 0;
  for (int i = 0; i < SFX_CHANNELS; i++)
  {
    SFX_CHANNEL_STATE state = {};
    esp_err_t err = sfx_get_state_snapshot(i, &state);
    if (err != ESP_OK)
    {
      printf("%02u  state error: %s\r\n", (unsigned)i, esp_err_to_name(err));
      continue;
    }

    if (!state.active) continue;
    sfx_print_channel_state(i, &state);
    active++;
  }

  if (!active) printf("(none)\r\n");

  printf("\r\nWAV objects\r\n");
  int objects = 0;
  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
  {
    if (!mem_obj[i].addr || mem_obj[i].type != OBJ_TYPE_WAV) continue;

    SFX_WAV_INFO info = {};
    esp_err_t err = sfx_parse_wav_object(i, &info);
    if (err == ESP_OK)
      sfx_print_wav_info(i, &info);
    else
      printf("WAV %02X: invalid: %s\r\n", i, esp_err_to_name(err));
    objects++;
  }

  if (!objects) printf("(none)\r\n");
  printf("\r\n");
  return 0;
}

int sfx_del_cmd(int argc, char **argv)
{
  unsigned long value = 0;

  if (argc < 3 || !sfx_parse_ulong_arg(argv[2], OBJ_HANDLES_MAX - 1, &value))
  {
    printf("Usage: sfx del <handle>\r\n");
    return 1;
  }

  int handle = (int)value;
  if (!check_handle(handle) || mem_obj[handle].type != OBJ_TYPE_WAV)
  {
    printf("Handle %02X is not a WAV object\r\n", handle);
    return 1;
  }

  if (!delete_obj(handle))
  {
    printf("Delete failed for handle %02X\r\n", handle);
    return 1;
  }

  printf("WAV deleted: %02X\r\n", handle);
  return 0;
}

int sfx_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  sfx load <file.wav>\r\n");
    printf("  sfx play <handle> [group] [volume] [pan] [pitch]     pitch example: 1.0, 1.5\r\n");
    printf("  sfx stop <channel>\r\n");
    printf("  sfx stop group <group>\r\n");
    printf("  sfx info [channel]\r\n");
    printf("  sfx vol [0..255]\r\n");
    printf("  sfx del <handle>\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "load"))
    return sfx_load_cmd(argc >= 3 ? argv[2] : NULL);

  if (!strcmp(op, "play"))
    return sfx_play_cmd(argc, argv);

  if (!strcmp(op, "stop"))
    return sfx_stop_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return sfx_info_cmd(argc, argv);

  if (!strcmp(op, "vol"))
    return sfx_vol_cmd(argc, argv);

  if (!strcmp(op, "del"))
    return sfx_del_cmd(argc, argv);

  printf("Unknown sfx subcommand: %s\r\n", op);
  return 1;
}

void sfx_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "sfx",
    .help     = "SFX commands: load/play/stop/info/vol/del",
    .hint     = NULL,
    .func     = &sfx_cmd,
    .argtable = NULL,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
