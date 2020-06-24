
#include "std.h"
#include "ayx32.h"

const char SNDAYX32::cpr_str[] = "AYX-32, (c)TSL";
const char SNDAYX32::bld_str[] = "Unreal Speccy";

SNDAYX32::SNDAYX32()
{
  status.b = 0;
  r_ptr.nul();
  w_ptr.nul();
  rd_ptr.nul();
  wd_ptr.nul();
}

void SNDAYX32::write_addr(REG val)
{
  reg = val;

  switch (reg)
  {
    // Params/response
    case R_PARAM: // = R_RESP
      r_ptr.init(resp, sizeof(resp));
      w_ptr.init(param, sizeof(param));
    break;

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

    case R_CORE_FRQ:
      temp_32 = CORE_FRQ;
      r_ptr.init((u8*)&temp_32, sizeof(temp_32));
      w_ptr.nul();
    break;
  }
}

void SNDAYX32::write(unsigned timestamp, u8 val)
{
  switch (reg)
  {
    case R_PARAM:
      w_ptr.write(val);
    break;

    // Command
    case R_CMD:
    {
      CMD cmd = (CMD)val;

      if (cmd == C_BREAK)
      {
        // if (!bg_task)
          // terminate(E_BREAK);
        // else
          ;   // +++ set BREAK flag for BG task
      }
      // else if (!status.busy)
      else
      {
        // status.b = STATUS_CLR;
        error = E_NONE;
        rd_ptr.nul();
        wd_ptr.nul();

        switch (cmd)
        {
          case C_LOCK:
            is_locked = (*(u32*)param != MAGIC_LCK);
          break;
        };
      }
    }
    break;
  };
}

u8 SNDAYX32::read()
{
  u8 rc = 0xFF;

  switch (reg)
  {
    case R_STATUS:
      rc = status.b;
    break;

    case R_VER:
      rc = 0;
    break;

    case R_RESP:
    case R_DEV_SIG:
    case R_CPR_STR:
    case R_BLD_STR:
    case R_CORE_FRQ:
      rc = r_ptr.read();
    break;
  }

  return rc;
}
