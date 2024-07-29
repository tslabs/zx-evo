
#include <limits.h>
#include "stats.h"

namespace stats
{
  STATS_t _st = {};
  
  void init()
  {
    _st.drq_data_start_min_us = INT_MAX;
    _st.drq_data_end_min_us = INT_MAX;
    _st.xm_render_min_us = INT_MAX;
    _st.xm_render_min_cpu = INT_MAX;
  }
};
