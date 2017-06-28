
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
  init_runtime();
  ei();
  ft_init(FT_MODE_3);

  dlist = (u32*)0x4000;
  dlp = 0;

  dl(ft_ClearColorRGB(0, 0, 0));
  dl(ft_Clear(1, 1, 1));
  dl(ft_ColorA(255));
  dl(ft_Begin(FT_POINTS));

  dl(ft_PointSize(100 << 4));
  dl(ft_BlendFunc(FT_SRC_ALPHA, FT_ONE));
  dl(ft_ColorRGB(255, 0, 0));
  dl(ft_Vertex2f(300 << 4, 250 << 4));
  dl(ft_ColorRGB(0, 255, 0));
  dl(ft_Vertex2f(300 << 4, 350 << 4));
  dl(ft_ColorRGB(0, 0, 255));
  dl(ft_Vertex2f(400 << 4, 300 << 4));

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
