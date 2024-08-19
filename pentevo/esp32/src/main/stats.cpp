
#include <limits.h>
#include "esp_timer.h"
#include "stats.h"

namespace stats
{
  STATS_t _st = {};
  int64_t start_t;
  u32 last_cmd_exec_time;

  void init()
  {
    _st.drq_data_start_min_us = INT_MAX;
    _st.drq_data_end_min_us = INT_MAX;
    _st.xm_render_min_us = INT_MAX;
    _st.xm_render_min_cpu = INT_MAX;
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
