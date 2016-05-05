
// FT812 VideoDAC2 wrapper

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "ft812.h"
#include "ft8xx/FT_Platform.h"
#include "ft8xx/FT_Emulator.h"

namespace vdac2
{
  void set_ss(bool ss)
  {
    FT8XXEMU_cs(ss);
    
    // if (ss)
      // printf("FT812 SS asserted\n");
    // else
      // printf("FT812 SS deasserted\n");
  }
  
  u8 transfer(u8 d)
  {
    u8 r = FT8XXEMU_transfer(d);
    // printf("FT812 data written %02x, read %02x\n", d, r);
    return r;
  }
  
  bool open_ft8xx()
  {
    return _beginthread(ft8xx_tread, 0, 0) > 0;
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
  