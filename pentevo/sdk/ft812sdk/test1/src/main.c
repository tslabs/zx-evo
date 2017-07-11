
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

  ts_wreg(TS_VCONFIG, 4);
  // ft_init(FT_MODE_0);
  // ft_init(FT_MODE_3);
  ft_init(FT_MODE_7);

  dlist = (u32*)0x3200;
  dlp = 0;

  dl(ft_VertexFormat(0));
  dl(ft_ClearColorRGB(0, 0, 0));
  dl(ft_Clear(1, 1, 1));

  dl(ft_ColorRGB(255, 255, 255));
  // dl(ft_ColorRGB(255, 0, 0));
  dl(ft_LineWidth(24));
  
  dl(ft_Begin(FT_LINE_STRIP));
  dl(ft_Vertex2f(0, 0));
  dl(ft_Vertex2f(1023, 0));
  dl(ft_Vertex2f(1023, 767));
  dl(ft_Vertex2f(0, 767));
  dl(ft_Vertex2f(0, 0));

  dl(ft_Begin(FT_LINE_STRIP));
  dl(ft_Vertex2f(0, 0));
  dl(ft_Vertex2f(799, 0));
  dl(ft_Vertex2f(799, 599));
  dl(ft_Vertex2f(0, 599));
  dl(ft_Vertex2f(0, 0));

  dl(ft_Begin(FT_LINE_STRIP));
  dl(ft_Vertex2f(0, 0));
  dl(ft_Vertex2f(639, 0));
  dl(ft_Vertex2f(639, 479));
  dl(ft_Vertex2f(0, 479));
  dl(ft_Vertex2f(0, 0));

  dl(ft_Begin(FT_POINTS));
  dl(ft_PointSize(100 << 4));
  dl(ft_BlendFunc(FT_SRC_ALPHA, FT_ONE));
  dl(ft_ColorRGB(255, 0, 0));
  dl(ft_Vertex2f(rsin(80, 32768) + 320, rcos(80, 32768) + 240));
  dl(ft_ColorRGB(0, 255, 0));
  dl(ft_Vertex2f(rsin(80, 21845 + 32768) + 320, rcos(80, 21845 + 32768) + 240));
  dl(ft_ColorRGB(0, 0, 255));
  dl(ft_Vertex2f(rsin(80, 43690 - 32768) + 320, rcos(80, 43690 - 32768) + 240));

  dl(ft_Display());

  ft_write_dl(dlist, dlp);
  ft_swap();

  while (1);
}
