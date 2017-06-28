
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <gamelib.h>
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

void load_pic(u8 p)
{
  u32 a = FT_RAM_G;
  u32 s = 240000;

  while (s)
  {
    ts_wreg(TS_PAGE3, p++);
    ft_write(a, (u8*)0xC000, 4096);
    a += 16384;
    s -= min(16384, s);
  }
}

void main()
{
  init_runtime();
  ft_init(FT_MODE_3);
  ts_wreg(TS_VCONFIG, 4);

  dlist = (u32*)0x8000;
  dlp = 0;

  dl(ft_VertexFormat(0));
  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorRGB(255, 255, 255));

  dl(ft_BitmapHandle(0));
  dl(ft_BitmapSource(FT_RAM_G + 0));
  dl(ft_BitmapLayout(FT_L1, 100, 600));
  dl(ft_BitmapLayoutX(100, 600));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 800, 600));
  dl(ft_BitmapSizeX(800, 600));

  dl(ft_BitmapHandle(1));
  dl(ft_BitmapSource(FT_RAM_G + 120000));
  dl(ft_BitmapLayout(FT_RGB565, 400, 150));
  dl(ft_BitmapLayoutX(400, 150));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 800, 600));
  dl(ft_BitmapSizeX(800, 600));

  dl(ft_Begin(FT_BITMAPS));
  dl(ft_BlendFunc(FT_ONE, FT_ZERO));
  dl(ft_ColorA(85));
  dl(ft_Vertex2ii(0, 0, 0, 0));
  dl(ft_BlendFunc(FT_ONE, FT_ONE));
  dl(ft_ColorA(170));
  dl(ft_Vertex2ii(0, 0, 0, 1));

  dl(ft_ColorMask(1, 1, 1, 0));
  dl(ft_BitmapTransformA(64));
  dl(ft_BitmapTransformE(64));
  dl(ft_BlendFunc(FT_DST_ALPHA, FT_ZERO));
  dl(ft_Vertex2ii(0, 0, 1, 1));
  dl(ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ONE));
  dl(ft_Vertex2ii(0, 0, 1, 0));

  dl(ft_Display());

  ft_write_dl(dlist, dlp);
  ft_wr8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);

  while (1)
  {
    u8 i;
    for (i = 16; i <= 64; i += 16)
    {
      load_pic(i);
      anykey();
    }
  }
}
