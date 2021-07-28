
// FT812 VideoDAC2 wrapper

#include "std.h"
#include "vars.h"
#include "util.h"
#include "ft812.h"
#include "ft8xx/EVE_Platform.h"
#include "ft8xx/FT_Platform.h"

namespace vdac2
{
  HMODULE ft8xxemu_hdl = 0;

  typedef const char * (*BT8XXEMU_version_t)();
  typedef void (*BT8XXEMU_defaults_t)(uint32_t versionApi, BT8XXEMU_EmulatorParameters *params, BT8XXEMU_EmulatorMode mode);
  typedef void (*BT8XXEMU_run_t)(uint32_t versionApi, void **emulator, const BT8XXEMU_EmulatorParameters *params);
  typedef void (*BT8XXEMU_destroy_t)(void *emulator);
  typedef int (*BT8XXEMU_isRunning_t)(void *emulator);
  typedef void (*BT8XXEMU_chipSelect_t)(void *emulator, int cs);
  typedef uint8_t (*BT8XXEMU_transfer_t)(void *emulator, uint8_t data);
  typedef int (*BT8XXEMU_hasInterrupt_t)(void *emulator);

  BT8XXEMU_version_t BT8XXEMU_version;
  BT8XXEMU_defaults_t BT8XXEMU_defaults;
  BT8XXEMU_run_t BT8XXEMU_run;
  BT8XXEMU_destroy_t BT8XXEMU_destroy;
  BT8XXEMU_isRunning_t BT8XXEMU_isRunning;
  BT8XXEMU_chipSelect_t BT8XXEMU_chipSelect;
  BT8XXEMU_transfer_t BT8XXEMU_transfer;
  BT8XXEMU_hasInterrupt_t BT8XXEMU_hasInterrupt;

  void *pEmulator;

  // ---
  //  Since shitty FT8xx emulation lib has no interrupt support, we need to make a manual dusk.

  int addr_cnt = 0, data_cnt = 0;
  bool ss_int = false;
  bool has_irq = false;
  u32 addr_int = 0;
  int line_cnt = 0;

  u8 xfer_int(u8 r, u8 d)
  {
    if (ss_int)
    {
      if (addr_cnt < 3)
      {
        addr_int = (addr_int << 8) | d;
        addr_cnt++;
      }

      else if (data_cnt < 2)
      {
        if (data_cnt == 1)
        {
          if (addr_int == 0x3020A8) // Interrupt events
          {
            r = has_irq;
            has_irq = false;
          }
        }

        data_cnt++;
      }
    }

    return r;
  }

  void line()
  {
    line_cnt++;

    if (line_cnt == 260)  // 60Hz
      line_cnt = 0;

    if (line_cnt == 0)
      has_irq = true;
  }
  // ---

  void set_ss(bool ss)
  {
    BT8XXEMU_chipSelect(pEmulator, ss);

    ss_int = ss;
    if (!ss)
      addr_cnt = data_cnt = addr_int = 0;
  }

  bool is_interrupt()
  {
    bool rc = BT8XXEMU_hasInterrupt(pEmulator);

    static bool old_irq = false;
    rc = has_irq && !old_irq;
    old_irq = has_irq;

    return rc;
  }

  u8 transfer(u8 d)
  {
    u8 r = BT8XXEMU_transfer(pEmulator, d);
    r = xfer_int(r, d);

    return r;
  }

  void close_ft8xx()
  {
    if (ft8xxemu_hdl)
    {
      BT8XXEMU_destroy(pEmulator);
      FreeLibrary(ft8xxemu_hdl);
      ft8xxemu_hdl = 0;
    }
  }

  void log(BT8XXEMU_Emulator *sender, void *context, BT8XXEMU_LogType type, const char *message)
  {
    if (type == BT8XXEMU_LogMessage)
      return;

    else if (type == BT8XXEMU_LogWarning)
      color(CONSCLR_WARNING);

    else if (type == BT8XXEMU_LogError)
      color(CONSCLR_ERROR);

    printf("%s\n", message);
  }

  int graphics(BT8XXEMU_Emulator *sender, void *context, int output, const argb8888 *buffer, uint32_t hsize, uint32_t vsize, BT8XXEMU_FrameFlags flags)
  {
    return 1;
  }
  
  bool open_ft8xx(const char **ver)
  {
    if (ft8xxemu_hdl)
      return true;

    ft8xxemu_hdl = LoadLibrary("bt8xxemu.dll");
    if (!ft8xxemu_hdl)
      return false;

    BT8XXEMU_version = (BT8XXEMU_version_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_version");
    BT8XXEMU_defaults = (BT8XXEMU_defaults_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_defaults");
    BT8XXEMU_run = (BT8XXEMU_run_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_run");
    BT8XXEMU_destroy = (BT8XXEMU_destroy_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_destroy");
    BT8XXEMU_isRunning = (BT8XXEMU_isRunning_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_isRunning");
    BT8XXEMU_chipSelect = (BT8XXEMU_chipSelect_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_chipSelect");
    BT8XXEMU_transfer = (BT8XXEMU_transfer_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_transfer");
    BT8XXEMU_hasInterrupt = (BT8XXEMU_hasInterrupt_t)GetProcAddress(ft8xxemu_hdl, "BT8XXEMU_hasInterrupt");

    BT8XXEMU_EmulatorParameters emulatorParams;
    BT8XXEMU_defaults(BT8XXEMU_VERSION_API, &emulatorParams, BT8XXEMU_EmulatorFT812);
    
    emulatorParams.Flags = BT8XXEMU_EmulatorEnableAudio
                         | BT8XXEMU_EmulatorEnableCoprocessor
                         | BT8XXEMU_EmulatorEnableGraphicsMultithread;
    // emulatorParams.Graphics = graphics;
    emulatorParams.Log = log;
    
    BT8XXEMU_run(BT8XXEMU_VERSION_API, &pEmulator, &emulatorParams);

    if (!BT8XXEMU_isRunning(pEmulator))
    {
      close_ft8xx();
      return false;
    }

    *ver = BT8XXEMU_version();

    return true;
  }
}
