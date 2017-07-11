
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

void load_pic_dma(u8 p)
{
  u32 s = 393216;
  ts_set_dma_saddr_p(0, p);
  ts_set_dma_size(256, 64);
  ft_start_write(FT_RAM_G);

  while (s)
  {
    ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
    ts_dma_wait();
    s -= min(16384, s);
  }

  __asm;
    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
  __endasm;
}

void main()
{
  init_runtime();
  ft_init(FT_MODE_7);
  ts_wreg(TS_VCONFIG, 4);

  dlist = (u32*)0x8000;
  dlp = 0;

  dl(ft_VertexFormat(0));
  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorRGB(255, 255, 255));

  dl(ft_BitmapHandle(0));
  dl(ft_BitmapSource(FT_RAM_G + 0));
  dl(ft_BitmapLayout(FT_L1, 128, 768));
  dl(ft_BitmapLayoutX(128, 768));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 1024, 768));
  dl(ft_BitmapSizeX(1024, 768));

  dl(ft_BitmapHandle(1));
  dl(ft_BitmapSource(FT_RAM_G + 196608));
  dl(ft_BitmapLayout(FT_RGB565, 512, 192));
  dl(ft_BitmapLayoutX(512, 192));
  dl(ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 1024, 768));
  dl(ft_BitmapSizeX(1024, 768));

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
    for (i = 10; i <= 202; i += 24)
    {
      load_pic_dma(i);
      anykey();
    }
  }
}
