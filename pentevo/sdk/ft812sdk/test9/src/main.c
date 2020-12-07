
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>

void main()
{
  ft_init(FT_MODE_7);
  ts_wreg(TS_VCONFIG, 4);

  ft_ccmd_start((u8*)0x8000);
  ft_SetFont2(14, 0x202948, 32);
  ft_SetBase(10);
  ft_ccmd_write();
  ft_cp_wait();
    
  while (1)
  {
    ft_ccmd_start((u8*)0x8000);
    ft_Dlstart();
    ft_Gradient(0, 0, 0x008000, 767, 1023, 0xFFFFFF);
    ft_RomFont(14, 34);
    ft_ColorRGB(255, 0, 0);
    ft_Number(100, 80, 14, 0, ft_rreg32(FT_REG_CLOCK));
    ft_ColorRGB(0, 200, 240);
    ft_Number(100, 220, 14, 0, ft_rreg32(FT_REG_FRAMES));
    ft_ColorRGB(0, 0, 80);
    ft_Text(100, 50, 31, 0, "Clocks:");
    ft_Text(100, 190, 31, 0, "Frames:");
    ft_ColorRGB(0, 0, 255);
    ft_Text(220, 500, 31, 0, "All your base are belong to us!");
    // ft_Text(220, 500, 31, 0, "Drink party has started!!!");
    ft_Display();
    ft_Swap();
    ft_ccmd_write();
    ft_cp_wait();
  }
}
