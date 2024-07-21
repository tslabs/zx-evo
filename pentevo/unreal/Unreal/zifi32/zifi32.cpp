
// ESP32 VDAC3 wrapper

#include "std.h"
#include "vars.h"
#include "util.h"
#include "esp32_emul.h"
#include "zifi32.h"
#include "ft232h.h"

namespace zf32
{
  bool is_init = false;

  u8 dma_buf[256 * 512];
  u32 dma_buf_ptr;

  void init()
  {
    if (comp.ts.zifi32 == 1)
    {
      esp32_emul_init();
      is_init = true;
    }
    else
      is_init = !spi::open();
  }

  void dma_r_start(u32 size)
  {
    if (comp.ts.zifi32 == 2)
    {
      dma_buf_ptr = 0;
      spi::xfer(dma_buf, dma_buf, size);
    }
  }

  void dma_w_start()
  {
    if (comp.ts.zifi32 == 2)
      dma_buf_ptr = 0;
  }

  void dma_w_end()
  {
    if (comp.ts.zifi32 == 2)
      spi::xfer(dma_buf, dma_buf, dma_buf_ptr);
  }

  u8 transfer_dma(u8 d)
  {
    if (comp.ts.zifi32 == 1)
      return transfer(d);
    else
    {
      u8 r = 255;

      if (dma_buf_ptr < sizeof(dma_buf))
      {
        r = dma_buf[dma_buf_ptr];
        dma_buf[dma_buf_ptr] = d;
        dma_buf_ptr++;
      }

      return r;
    }
  }

  u8 transfer(u8 d)
  {
    u8 r = 0xFF;
    
    if (comp.ts.zifi32 == 1)
      r = esp32_emul(d);
    else
      spi::xfer_byte(d, r);

    // printf("ESP32 wr: %02X, rd: %02X\n", d, r);
    
    return r;
  }

  void set_ss(bool ss)
  {
    if (comp.ts.zifi32 == 1)
    {
      if (!ss) esp32_emul(-1);   // reset ESP32 emul state
    }
    else
      spi::set_ss(ss);

    // printf("SS %s\n", ss ? "ON" : "OFF");
  }
}
