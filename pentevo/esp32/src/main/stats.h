
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

  int xm_render_last_us;
  int xm_render_min_us;
  int xm_render_max_us;

  int xm_render_last_cpu;
  int xm_render_min_cpu;
  int xm_render_max_cpu;

  float xm_samp_min;
  float xm_samp_max;

  char runtime_stats_buffer[2048];
} STATS_t;

namespace stats
{
  extern STATS_t _st;
  
  void init();
  void set_start();
  void set_end();
  u32 get_time();
};
