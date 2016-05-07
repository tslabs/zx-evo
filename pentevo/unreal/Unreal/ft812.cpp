
// FT812 VideoDAC2 wrapper

#include "std.h"
#include "emul.h"
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

  void set_ss(bool ss)
  {
    (*FT8XXEMU_cs)(ss);
    
    // if (ss)
      // printf("FT812 SS asserted\n");
    // else
      // printf("FT812 SS deasserted\n");
  }
  
  u8 transfer(u8 d)
  {
    u8 r = (*FT8XXEMU_transfer)(d);
    // printf("FT812 data written %02x, read %02x\n", d, r);
    return r;
  }
  
  bool open_ft8xx()
  {
	if ( ft8xxemu_hdl )
      return true;

	ft8xxemu_hdl = LoadLibrary("ft8xxemu.dll");
	if ( !ft8xxemu_hdl )
	  return false;

	FT8XXEMU_defaults = (FT8XXEMU_defaults_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_defaults");
	FT8XXEMU_run = (FT8XXEMU_run_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_run");
	FT8XXEMU_stop = (FT8XXEMU_stop_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_stop");
	FT8XXEMU_transfer = (FT8XXEMU_transfer_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_transfer");
	FT8XXEMU_cs = (FT8XXEMU_cs_t)GetProcAddress(ft8xxemu_hdl, "FT8XXEMU_cs");

	if ( !FT8XXEMU_defaults || !FT8XXEMU_run )
	{
	  close_ft8xx();
      return false;
	}

    if ( _beginthread(ft8xx_tread, 0, 0) < 0 )
    {
      close_ft8xx();
      return false;
    }

    return true;
  }

  void close_ft8xx()
  {
    if ( ft8xxemu_hdl )
      FreeLibrary(ft8xxemu_hdl);
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
  