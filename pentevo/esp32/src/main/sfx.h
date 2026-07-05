#pragma once

#include "esp_err.h"
#include "main.h"

#define SFX_CHANNELS    32
#define SFX_VOLUME_MAX  255
#define SFX_PAN_CENTER  128
#define SFX_PITCH_ONE   0x0100

typedef struct
{
  const u8 *data;
  u32 data_size;
  u32 frame_count;
  u32 sample_rate;
  u16 block_align;
  u16 bits_per_sample;
  u8 channels;
  u8 is_signed;
} SFX_WAV_INFO;

typedef struct
{
  u8 active;
  u8 handle;
  u8 group;
  u8 group_order;
  u8 volume;
  u8 pan;
  u16 pitch;
  float position;
  SFX_WAV_INFO wav;
} SFX_CHANNEL;

typedef struct
{
  u8 active;
  u8 handle;
  u8 group;
  u8 group_order;
  u8 volume;
  u8 pan;
  u16 pitch;
  u32 sample_rate;
  u32 frame_count;
  u32 position;
  u16 bits_per_sample;
  u8 channels;
  u8 is_signed;
} SFX_CHANNEL_STATE;

extern SFX_CHANNEL sfx_channels[SFX_CHANNELS];
extern u8 sfx_global_volume;

void sfx_init();
esp_err_t sfx_parse_wav_object(int handle, SFX_WAV_INFO *info);
esp_err_t sfx_load_file(const char *path, int *out_handle, SFX_WAV_INFO *out_info);
esp_err_t sfx_play(int handle, u8 group, u8 volume, u8 pan, u16 pitch, int *out_channel);
esp_err_t sfx_stop(int channel);
esp_err_t sfx_stop_group(u8 group);
esp_err_t sfx_stop_handle(int handle);
esp_err_t sfx_set_params(int channel, u8 volume, u8 pan, u16 pitch);
esp_err_t sfx_set_volume(u8 volume);
esp_err_t sfx_get_state(int channel, SFX_CHANNEL_STATE *state);
esp_err_t sfx_get_state_snapshot(int channel, SFX_CHANNEL_STATE *state);
esp_err_t sfx_play_sync(int handle, u8 group, u8 volume, u8 pan, u16 pitch, int *out_channel);
esp_err_t sfx_stop_sync(int channel);
esp_err_t sfx_stop_group_sync(u8 group);
esp_err_t sfx_stop_handle_sync(int handle);
esp_err_t sfx_set_params_sync(int channel, u8 volume, u8 pan, u16 pitch);
esp_err_t sfx_set_volume_sync(u8 volume);
esp_err_t sfx_get_state_sync(int channel, SFX_CHANNEL_STATE *state);
void sfx_render(float *mix, int sample_count, int output_sample_rate);
void sfx_console_register_system_commands();
