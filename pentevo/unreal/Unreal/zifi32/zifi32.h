#pragma once
#include "sysdefs.h"

namespace zf32
{
  void init();
  void set_ss(bool);
  u8 transfer(u8);
  u8 transfer_dma(u8);
  void dma_r_start(u32);
  void dma_w_start();
  void dma_w_end();
}
