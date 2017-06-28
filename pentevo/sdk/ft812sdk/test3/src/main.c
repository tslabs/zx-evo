
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

void main()
{
  u16 i, j;
  
  init_runtime();
  ei();
  ft_init(FT_MODE_3);

  dlist = (u32*)0x4000;
  dlp = 0;

  dl(ft_VertexFormat(0));
  dl(ft_ClearColorRGB(0, 0, 0));
  dl(ft_Clear(1, 1, 1));

  dl(ft_ColorA(255));
  dl(ft_Begin(FT_POINTS));
  dl(ft_PointSize(100 << 4));
  dl(ft_BlendFunc(FT_SRC_ALPHA, FT_ONE));
  dl(ft_ColorRGB(255, 0, 0));
  dl(ft_Vertex2f(rsin(80, 0) + 400, rcos(80, 0) + 300));
  dl(ft_ColorRGB(0, 255, 0));
  dl(ft_Vertex2f(rsin(80, 21845) + 400, rcos(80, 21845) + 300));
  dl(ft_ColorRGB(0, 0, 255));
  dl(ft_Vertex2f(rsin(80, 43690) + 400, rcos(80, 43690) + 300));
  
  dl(ft_ClearColorA(32));
  dl(ft_ColorMask(0, 0, 0, 1));
  dl(ft_Clear(1, 1, 1));
  
  dl(ft_BlendFunc(FT_ONE, FT_ONE));
  dl(ft_Begin(FT_POINTS));
  dl(ft_ColorA(20));
  
  for (i = 120, j = 0; j <= 255; i -= 5, j += 20)
  {
    dl(ft_PointSize(i << 4));
    dl(ft_Vertex2f(400, 300));
  }

  dl(ft_ColorRGB(0, 0, 0));
  dl(ft_ColorMask(1, 1, 1, 1));
  dl(ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA));
  dl(ft_Begin(FT_RECTS));
  dl(ft_Vertex2f(0, 0));
  dl(ft_Vertex2f(799, 599));
  
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
