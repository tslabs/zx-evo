
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

  // prepare palette for ZX
  {
    u16 a;
    u16 *pal = (u16*)0x9000;

    for (a = 0; a < 256; a++)
    {
      u8 b = a & 64;
      u8 i = a;
      u16 ir = (i & 2) ? (b ? (31 << 11) : (21 << 11)) : 0;
      u16 ig = (i & 4) ? (b ? (63 << 5) : (42 << 5)) : 0;
      u16 ib = (i & 1) ? (b ? 31 : 21) : 0;
      pal[a] = ir + ig + ib;

      i = a >> 3;
      ir = (i & 2) ? (b ? (31 << 11) : (21 << 11)) : 0;
      ig = (i & 4) ? (b ? (63 << 5) : (42 << 5)) : 0;
      ib = (i & 1) ? (b ? 31 : 21) : 0;
      pal[a + 256] = ir + ig + ib;
    }
  }
  ft_write((u8*)0x9000, FT_RAM_G + 6912, 1024);

  // prepare DL
  ft_ccmd_start(cmdl);
  ft_Clear(1, 1, 1);
  ft_ColorRGB(255, 255, 255);
  ft_VertexTranslateX(144 << 4);
  ft_VertexTranslateY(108 << 4);
  ft_Begin(FT_BITMAPS);

  // pixels
  {
    u8 i;

    ft_BitmapHandle(0);
    ft_BitmapLayout(FT_L1, 32, 1);
    ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 512, 2);
    ft_BitmapTransformA(128);
    ft_BitmapTransformE(128);

    ft_BitmapSource(FT_RAM_G);
    for (i = 0; i < 128; i++)
      ft_Vertex2ii(0, i << 1, 0, ((i & 7) << 3) + ((i >> 3) & 7) + (i & 64));

    ft_BitmapSource(FT_RAM_G + 4096);
    for (i = 128; i < 192; i++)
      ft_Vertex2ii(0, i << 1, 0, ((i & 7) << 3) + ((i >> 3) & 7));
  }

  // attributes
  ft_ColorMask(1, 1, 1, 0);
  ft_BitmapHandle(0);
  ft_BitmapSource(FT_RAM_G + 6144);
  ft_BitmapLayout(FT_PALETTED565, 32, 24);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 512, 384);
  // ft_BitmapSize(FT_BILINEAR, FT_BORDER, FT_BORDER, 512, 384);

  ft_BitmapTransformA(16);
  ft_BitmapTransformE(16);
  ft_PaletteSource(FT_RAM_G + 6912);
  ft_BlendFunc(FT_DST_ALPHA, FT_ZERO);
  ft_Vertex2ii(0, 0, 0, 0);
  ft_PaletteSource(FT_RAM_G + 6912 + 512);
  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ONE);
  ft_Vertex2ii(0, 0, 0, 0);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  while (1)
  {
    u8 i;
    for (i = 0; i < 7; i++)
    {
      ts_wreg(TS_PAGE3, 16 + i);
      ft_write((u8*)0xC000, FT_RAM_G, 6912);
      anykey();
    }
  }
}
