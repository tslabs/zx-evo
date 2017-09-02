
#define USE_DMA

#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>

u8 cmdl[0x800];

void main()
{
  ft_init(FT_MODE_3);
  ts_wreg(TS_VCONFIG, 4);

  ft_ccmd_start(cmdl);
  ft_VertexFormat(0);
  ft_Clear(1, 1, 1);
  ft_ColorRGB(255, 255, 255);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_RAM_G + 0);
  ft_BitmapLayout(FT_L1, 100, 600);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 800, 600);

  ft_BitmapHandle(1);
  ft_BitmapSource(FT_RAM_G + 120000);
  ft_BitmapLayout(FT_RGB565, 400, 150);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 800, 600);

  ft_Begin(FT_BITMAPS);
  ft_BlendFunc(FT_ONE, FT_ZERO);
  ft_ColorA(85);
  ft_Vertex2ii(0, 0, 0, 0);
  ft_BlendFunc(FT_ONE, FT_ONE);
  ft_ColorA(170);
  ft_Vertex2ii(0, 0, 0, 1);

  ft_ColorMask(1, 1, 1, 0);
  ft_BitmapTransformA(64);
  ft_BitmapTransformE(64);
  ft_BlendFunc(FT_DST_ALPHA, FT_ZERO);
  ft_Vertex2ii(0, 0, 1, 1);
  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ONE);
  ft_Vertex2ii(0, 0, 1, 0);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  while (1)
  {
    u8 i;
    for (i = 16; i <= 64; i += 16)
    {
#ifdef USE_DMA
      ft_load_ram_dma((u8*)0xC000, i, FT_RAM_G, 240000);
#else
      ft_load_ram_p((u8*)0xC000, i, FT_RAM_G, 240000);
#endif
      anykey();
    }
  }
}
