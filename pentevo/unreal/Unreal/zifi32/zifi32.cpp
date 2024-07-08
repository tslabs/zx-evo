
// ESP32 VDAC3 wrapper

#include "std.h"
#include "vars.h"
#include "util.h"
#include "esp32_emul.h"

#define ESP32_EMUL 1

namespace zf32
{
  u8 transfer(u8 d)
  {
    u8 r = 0xFF;
    
#if ESP32_EMUL
    r = esp32_emul(d);
#endif

    // printf("ESP32 wr: %02X, rd: %02X\n", d, r);
    
    return r;
  }

  void set_ss(bool ss)
  {
#if ESP32_EMUL
    if (!ss) esp32_emul(-1);   // reset ESP32 emul state
#endif
  }
}
