
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
u16 dla[32];
u16 dlap;

void dl(u32 a)
{
  dlist[dlp++] = a;
}

void main()
{
  init_runtime();
  ts_wreg(TS_VCONFIG, 4);
  ft_init(FT_MODE_7);
  
  dlist = (u32*)0x8000;
  dlp = 0;
  dlap = 0;

  dla[dlap++] = dlp;
  dl(ft_VertexTranslateX(0));
  dl(ft_VertexFormat(0));
  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorRGB(255, 255, 255));

  dl(ft_BitmapHandle(0));
  dl(ft_BitmapSource(FT_RAM_G + 0));
  dl(ft_BitmapLayout(FT_L1, 243, 768));
  dl(ft_BitmapLayoutX(243, 768));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 1940, 768));
  dl(ft_BitmapSizeX(1940, 768));

  dl(ft_BitmapHandle(1));
  dl(ft_BitmapSource(FT_RAM_G + 373248));
  dl(ft_BitmapLayout(FT_RGB565, 970, 192));
  dl(ft_BitmapLayoutX(970, 192));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 1940, 768));
  dl(ft_BitmapSizeX(1940, 768));

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

  {
    u8 p = 16;
    u32 s = 745728;
    
    ts_set_dma_saddr_p(0, p);
    ts_set_dma_size(256, 64);
    ft_start_write(FT_RAM_G);

    while (s)
    {
      ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
      ts_dma_wait();
      s -= min(16384, s);
    }

    ft_spi_unsel();
  }

  {
    u16 ang = 16384;
    
    while (1)
    {
      dlist[dla[0]] = ft_VertexTranslateX(rsin(458 << 4, ang) - (458 << 4));
      ang += 64;
      ft_write_dl(dlist, dlp);
      ft_swap();
      ft_wait_int();
    }
  }
}
