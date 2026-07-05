
#include <limits.h>
#include "esp_timer.h"
#include "stats.h"

namespace stats
{
  STATS_t _st = {};
  int64_t start_t;
  u32 last_cmd_exec_time;

  void reset_audio_render()
  {
    _st.audio_tracker_last_us = 0;
    _st.audio_tracker_min_us = INT_MAX;
    _st.audio_tracker_max_us = 0;
    _st.audio_tracker_last_cpu_x10 = 0;
    _st.audio_tracker_min_cpu_x10 = INT_MAX;
    _st.audio_tracker_max_cpu_x10 = 0;

    _st.audio_opl_last_us = 0;
    _st.audio_opl_min_us = INT_MAX;
    _st.audio_opl_max_us = 0;
    _st.audio_opl_last_cpu_x10 = 0;
    _st.audio_opl_min_cpu_x10 = INT_MAX;
    _st.audio_opl_max_cpu_x10 = 0;

    _st.audio_sfx_last_us = 0;
    _st.audio_sfx_min_us = INT_MAX;
    _st.audio_sfx_max_us = 0;
    _st.audio_sfx_last_cpu_x10 = 0;
    _st.audio_sfx_min_cpu_x10 = INT_MAX;
    _st.audio_sfx_max_cpu_x10 = 0;

    _st.audio_total_last_us = 0;
    _st.audio_total_min_us = INT_MAX;
    _st.audio_total_max_us = 0;
    _st.audio_total_last_cpu_x10 = 0;
    _st.audio_total_min_cpu_x10 = INT_MAX;
    _st.audio_total_max_cpu_x10 = 0;
  }

  void init()
  {
    _st.drq_data_start_min_us = INT_MAX;
    _st.drq_data_end_min_us = INT_MAX;
    reset_audio_render();
  }

  void update_audio_render(int *last_us, int *min_us, int *max_us, int *last_cpu_x10, int *min_cpu_x10, int *max_cpu_x10, int us, int sample_rate, int sample_count)
  {
    int cpu_x10 = 0;

    if (sample_count > 0)
      cpu_x10 = (int)((int64_t)us * sample_rate * 10 / sample_count / 10000);

    *last_us = us;
    if (us < *min_us) *min_us = us;
    if (us > *max_us) *max_us = us;

    *last_cpu_x10 = cpu_x10;
    if (cpu_x10 < *min_cpu_x10) *min_cpu_x10 = cpu_x10;
    if (cpu_x10 > *max_cpu_x10) *max_cpu_x10 = cpu_x10;
  }

  void set_start()
  {
    start_t = esp_timer_get_time();
  }

  void set_end()
  {
    last_cmd_exec_time = esp_timer_get_time() - start_t;
  }
  
  u32 get_time()
  {
    return last_cmd_exec_time;
  }
};
