
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
  u16 i, j;

  ft_init(FT_MODE_800_600_60);
  ts_wreg(TS_VCONFIG, 4);

  ft_ccmd_start(cmdl);
  ft_VertexFormat(0);
  ft_ClearColorRGB(0, 0, 0);
  ft_Clear(1, 1, 1);

  ft_ColorA(255);
  ft_Begin(FT_POINTS);
  ft_PointSize(100 << 4);
  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);
  ft_ColorRGB(255, 0, 0);
  ft_Vertex2f(rsin(80, 0) + 400, rcos(80, 0) + 300);
  ft_ColorRGB(0, 255, 0);
  ft_Vertex2f(rsin(80, 21845) + 400, rcos(80, 21845) + 300);
  ft_ColorRGB(0, 0, 255);
  ft_Vertex2f(rsin(80, 43690) + 400, rcos(80, 43690) + 300);

  ft_ClearColorA(32);
  ft_ColorMask(0, 0, 0, 1);
  ft_Clear(1, 1, 1);

  ft_BlendFunc(FT_ONE, FT_ONE);
  ft_Begin(FT_POINTS);
  ft_ColorA(20);

  for (i = 120, j = 0; j <= 255; i -= 5, j += 20)
  {
    ft_PointSize(i << 4);
    ft_Vertex2f(400, 300);
  }

  ft_ColorRGB(0, 0, 0);
  ft_ColorMask(1, 1, 1, 1);
  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA);
  ft_Begin(FT_RECTS);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(799, 599);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  while (1);
}
