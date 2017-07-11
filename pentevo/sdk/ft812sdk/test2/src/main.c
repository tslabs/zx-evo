
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
  ei();
  ft_init(FT_MODE_3);

  dlist = (u32*)0x4000;
  dlp = 0;
  
  dl(ft_ClearColorRGB(80, 0, 0));
  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorA(255));
  dl(ft_Begin(FT_POINTS));
  
  {
    u8 i;
    
    for (i = 0; i < 248; i += 8)
    {
      dl(ft_PointSize(((((255 - i) * 10) / 12) << 3)));
      
      dl(ft_ColorRGB(i, i >> 1, i >> 1));
      dl(ft_Vertex2f(150 << 4, 150 << 4));
    
      dl(ft_ColorRGB(i >> 1, i, i >> 1));
      dl(ft_Vertex2f(400 << 4, 150 << 4));
      
      dl(ft_ColorRGB(i >> 1, i >> 1, i));
      dl(ft_Vertex2f(650 << 4, 150 << 4));
      
      dl(ft_ColorRGB(i, i, i >> 1));
      dl(ft_Vertex2f(150 << 4, 450 << 4));
      
      dl(ft_ColorRGB(i >> 1, i, i));
      dl(ft_Vertex2f(400 << 4, 450 << 4));
      
      dl(ft_ColorRGB(i, i >> 1, i));
      dl(ft_Vertex2f(650 << 4, 450 << 4));
      
      dl(ft_ColorRGB(i, i, i));
      dl(ft_Vertex2f(280 << 4, 300 << 4));
      
      dl(ft_ColorRGB(i, (i * 100) / 255, 0));
      dl(ft_Vertex2f(520 << 4, 300 << 4));
    }
  }
  
  dl(ft_End());
  dl(ft_Display());

  ft_write_dl(dlist, dlp);
  ft_wr8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);

  __asm
    ld bc, #0x00AF
    ld a, #4
    out (c), a
  __endasm;

  while (1);
}
