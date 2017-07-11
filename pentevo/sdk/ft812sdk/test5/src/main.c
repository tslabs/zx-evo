
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>

void init_runtime()
{
}

u32 *dlist;
u16 dlp;

void dl(u32 a)
{
  dlist[dlp++] = a;
}

void main()
{
  init_runtime();
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
  ft_write(FT_RAM_G + 6912, (u32*)0x9000, 1024 >> 2);

  // prepare DL
  dlist = (u32*)0x8000;
  dlp = 0;

  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorRGB(255, 255, 255));
  dl(ft_VertexTranslateX(144 << 4));
  dl(ft_VertexTranslateY(108 << 4));
  dl(ft_Begin(FT_BITMAPS));

  // pixels
  {
    u8 i;

    dl(ft_BitmapHandle(0));
    dl(ft_BitmapLayout(FT_L1, 32, 1));
    dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 512, 2));
    dl(ft_BitmapSizeX(512, 2));
    dl(ft_BitmapTransformA(128));
    dl(ft_BitmapTransformE(128));

    dl(ft_BitmapSource(FT_RAM_G));
    for (i = 0; i < 128; i++)
      dl(ft_Vertex2ii(0, i << 1, 0, ((i & 7) << 3) + ((i >> 3) & 7) + (i & 64)));

    dl(ft_BitmapSource(FT_RAM_G + 4096));
    for (i = 128; i < 192; i++)
      dl(ft_Vertex2ii(0, i << 1, 0, ((i & 7) << 3) + ((i >> 3) & 7)));
  }

  // attributes
  dl(ft_ColorMask(1, 1, 1, 0));
  dl(ft_BitmapHandle(0));
  dl(ft_BitmapSource(FT_RAM_G + 6144));
  dl(ft_BitmapLayout(FT_PALETTED565, 32, 24));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 512, 384));
  // dl(ft_BitmapSize(FT_BILINEAR, FT_BORDER, FT_BORDER, 512, 384));
  dl(ft_BitmapSizeX(512, 384));

  dl(ft_BitmapTransformA(16));
  dl(ft_BitmapTransformE(16));
  dl(ft_PaletteSource(FT_RAM_G + 6912));
  dl(ft_BlendFunc(FT_DST_ALPHA, FT_ZERO));
  dl(ft_Vertex2ii(0, 0, 0, 0));
  dl(ft_PaletteSource(FT_RAM_G + 6912 + 512));
  dl(ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ONE));
  dl(ft_Vertex2ii(0, 0, 0, 0));

  dl(ft_Display());

  ft_write_dl(dlist, dlp);
  ft_wr8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);

  while (1)
  {
    u8 i;
    for (i = 0; i < 7; i++)
    {
      ts_wreg(TS_PAGE3, 16 + i);
      ft_write(FT_RAM_G, (u32*)0xC000, 6912 >> 2);
      anykey();
    }
  }
}
