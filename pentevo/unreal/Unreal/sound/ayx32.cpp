
#include "std.h"
#include "ayx32.h"

const char SNDAYX32::cpr_str[] = "AYX-32, (c)TSL";
const char SNDAYX32::bld_str[] = "Unreal Speccy";

SNDAYX32::SNDAYX32()
{
  reg = 0;
  r_ptr.nul();
  w_ptr.nul();
  rd_ptr.nul();
  wd_ptr.nul();
}

void SNDAYX32::write_addr(u8 val)
{
  switch (reg)
  {
    // Device signature
    case R_DEV_SIG:
      temp_16 = DEV_SIG;
      r_ptr.init((u8*)&temp_16, sizeof(temp_16));
      w_ptr.nul();
    break;

    // Chip Copyright String
    case R_CPR_STR:
      r_ptr.init((u8*)cpr_str, sizeof(cpr_str));
      w_ptr.nul();
    break;

    // Build String
    case R_BLD_STR:
      r_ptr.init((u8*)bld_str, sizeof(bld_str));
      w_ptr.nul();
    break;
  }
}

void SNDAYX32::write(unsigned timestamp, u8 val)
{
}

u8 SNDAYX32::read()
{
  u8 rc = 0xFF;

  switch (reg)
  {
    case R_DEV_SIG:
    case R_CPR_STR:
    case R_BLD_STR:
      rc = r_ptr.read();
    break;
  }

  return rc;
}
