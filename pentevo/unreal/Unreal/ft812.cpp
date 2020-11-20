
// FT812 VideoDAC2 wrapper

#include "std.h"
#include "vars.h"
#include "ft812.h"
#include "ft8xx/FT_Platform.h"
#include "ft8xx/FT_Emulator.h"

namespace vdac2
{
  typedef void (*FT8XXEMU_defaults_t)(uint32_t versionApi, FT8XXEMU_EmulatorParameters *params, FT8XXEMU_EmulatorMode mode);
  typedef void (*FT8XXEMU_run_t)(uint32_t versionApi, const FT8XXEMU_EmulatorParameters *params);
  typedef void (*FT8XXEMU_stop_t)();
  typedef uint8_t (**FT8XXEMU_transfer_t)(uint8_t data);
  typedef void (**FT8XXEMU_cs_t)(int cs);

  static FT8XXEMU_defaults_t FT8XXEMU_defaults;
  static FT8XXEMU_run_t FT8XXEMU_run;
  static FT8XXEMU_stop_t FT8XXEMU_stop;
  static FT8XXEMU_transfer_t FT8XXEMU_transfer;
  static FT8XXEMU_cs_t FT8XXEMU_cs;

  static HMODULE ft8xxemu_hdl = 0;

  u32 line_cnt;  // line counter
  u32 line_eof;  // end of frame limit

  enum STATE
  {
    ST_OFF,
    ST_START,
    ST_ADDR,
    ST_READ,
    ST_WRITE,
  };

  STATE state;            // SPI FSM state
  u32 st_cnt;             // SPI FSM addr/data counter
  u8 st_addr_b[3];        // SPI FSM address array
  u32 st_addr;            // SPI FSM address
  u32 st_wreg;            // SPI FSM write register
  u32 st_rreg;            // SPI FSM read register
  bool st_read_override;  // SPI FSM read override

  u32 reg_int_flags;

  void init_runtime()
  {
    state = ST_OFF;
    line_cnt = 0;
    reg_int_flags = 0;
    // line_eof = -1;
    line_eof = 260;   // !!! only 60Hz yet
  }

  bool process_line()
  {
    bool rc = false;
    line_cnt++;
    if (line_cnt >= line_eof)
    {
      line_cnt = 0;
      reg_int_flags = 1;
      rc = true;
    }

    return rc;
  }

  void execute_command()
  {
    // +++
  }

  void process_write()
  {
    // +++
  }

  void prepare_read()
  {
    switch (st_addr)
    {
      case 0x3020A8:    // REG_INT_FLAGS
        st_rreg = reg_int_flags;
        reg_int_flags = 0;
        st_read_override = true;
      break;

      default:
        st_read_override = false;
    }
  }

  void set_ss(bool ss)
  {
    (*FT8XXEMU_cs)(ss);

    static bool ss_old = false;

    if (ss)
    {
      if (!ss_old)
        state = ST_START;
    }
    else
      state = ST_OFF;

    ss_old = ss;

    // if (ss)
      // printf("FT812 SS asserted\n");
    // else
      // printf("FT812 SS deasserted\n");
  }

  u8 transfer(u8 d)
  {
    u8 r = (*FT8XXEMU_transfer)(d);

    switch (state)
    {
      case ST_START:
        st_addr_b[0] = d;
        st_cnt = 1;
        st_read_override = false;
        state = ST_ADDR;
      break;

      case ST_ADDR:
        st_addr_b[st_cnt++] = d;

        if (st_cnt == sizeof(st_addr_b))
        {
          st_addr = (st_addr_b[0] << 16) | (st_addr_b[1] << 8) | st_addr_b[2];

          switch(st_addr_b[0] & 0xC0)
          {
            case 0x00:  // Memory Read
              if (st_addr)
              {
                st_cnt = -1;
                prepare_read();
                state = ST_READ;
              }
              else  // CMD_ACTIVE
              {
                execute_command();
                state = ST_OFF;
              }
            break;

            case 0x80:  // Memory Write
              st_cnt = 0;
              state = ST_WRITE;
            break;

            case 0x40:  // Command
              execute_command();
              state = ST_OFF;
            break;

            default:
              state = ST_OFF;
          }
        }
      break;

      case ST_WRITE:
        {
          u8 *wptr = (u8*)&st_wreg;
          wptr[st_cnt++] = d;
        }

        if (++st_cnt == 4)
        {
          st_cnt = 0;
          process_write();
        }
      break;

      case ST_READ:
        if (st_read_override && (st_cnt != -1))
        {
          u8 *rptr = (u8*)&st_rreg;
          r = rptr[st_cnt];
        }

        if (++st_cnt == 4)
        {
          st_cnt = 0;
          prepare_read();
        }
      break;
    }

    // printf("FT812 data written %02x, read %02x\n", d, r);

    return r;
  }

  bool open_ft8xx()
  {
    if (ft8xxemu_hdl)
      return true;

    ft8xxemu_hdl = LoadLibrary("ft8xxemu.dll");
    if ( !ft8xxemu_hdl )
      return false;

    FT8XXEMU_defaults = (FT8XXEMU_defaults_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_defaults");
    FT8XXEMU_run = (FT8XXEMU_run_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_run");
    FT8XXEMU_stop = (FT8XXEMU_stop_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_stop");
    FT8XXEMU_transfer = (FT8XXEMU_transfer_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_transfer");
    FT8XXEMU_cs = (FT8XXEMU_cs_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_cs");

    if (!FT8XXEMU_defaults || !FT8XXEMU_run)
    {
      close_ft8xx();
      return false;
    }

    if (_beginthread(ft8xx_tread, 0, 0) < 0)
    {
      close_ft8xx();
      return false;
    }

    init_runtime();
    return true;
  }

  void close_ft8xx()
  {
    if ( ft8xxemu_hdl )
      //FreeLibrary(ft8xxemu_hdl);
    ft8xxemu_hdl = 0;
  }

  void ft8xx_tread(void *args)
  {
    FT8XXEMU_EmulatorParameters params;
    FT8XXEMU_defaults(FT8XXEMU_VERSION_API, &params, FT8XXEMU_EmulatorFT812);
    params.Flags &= (~FT8XXEMU_EmulatorEnableDynamicDegrade & ~FT8XXEMU_EmulatorEnableRegPwmDutyEmulation);
    params.Setup = ft8xx_setup;
    params.Loop = ft8xx_loop;
    FT8XXEMU_run(FT8XXEMU_VERSION_API, &params);
  }

  Ft_Gpu_Hal_Context_t host, *phost;

  void ft8xx_setup()
  {
    host.hal_config.channel_no = 0;
    host.hal_config.pdn_pin_no = FT800_PD_N;
    host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;
    host.ft_cmd_fifo_wp = host.ft_dl_buff_wp = 0;
    host.spinumdummy = 1;
    host.spichannel = 0;
    host.status = FT_GPU_HAL_OPENED;
    phost = &host;
  }

  void ft8xx_loop()
  {
    Sleep(1000);
  }
}
