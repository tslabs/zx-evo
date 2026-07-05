
#pragma once

#include "main.h"

typedef struct
{
  int drq_data_t;
  int drq_data_start_last_us;
  int drq_data_start_min_us;
  int drq_data_start_max_us;
  int drq_data_end_last_us;
  int drq_data_end_min_us;
  int drq_data_end_max_us;

  int audio_tracker_last_us;
  int audio_tracker_min_us;
  int audio_tracker_max_us;
  int audio_tracker_last_cpu_x10;
  int audio_tracker_min_cpu_x10;
  int audio_tracker_max_cpu_x10;

  int audio_opl_last_us;
  int audio_opl_min_us;
  int audio_opl_max_us;
  int audio_opl_last_cpu_x10;
  int audio_opl_min_cpu_x10;
  int audio_opl_max_cpu_x10;

  int audio_sfx_last_us;
  int audio_sfx_min_us;
  int audio_sfx_max_us;
  int audio_sfx_last_cpu_x10;
  int audio_sfx_min_cpu_x10;
  int audio_sfx_max_cpu_x10;

  int audio_total_last_us;
  int audio_total_min_us;
  int audio_total_max_us;
  int audio_total_last_cpu_x10;
  int audio_total_min_cpu_x10;
  int audio_total_max_cpu_x10;

  float xm_samp_min;
  float xm_samp_max;

  char runtime_stats_buffer[2048];
} STATS_t;

namespace stats
{
  extern STATS_t _st;
  
  void init();
  void reset_audio_render();
  void update_audio_render(int *last_us, int *min_us, int *max_us, int *last_cpu_x10, int *min_cpu_x10, int *max_cpu_x10, int us, int sample_rate, int sample_count);
  void set_start();
  void set_end();
  u32 get_time();
};
