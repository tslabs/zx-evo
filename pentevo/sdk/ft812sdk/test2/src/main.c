
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>

u8 cmdl[0x0F00];

void main()
{
  ft_init(FT_MODE_800_600_60);
  ts_wreg(TS_VCONFIG, 4);

  ft_ccmd_start(cmdl);
  ft_ClearColorRGB(80, 0, 0);
  ft_Clear(1, 1, 1);
  ft_ColorA(255);
  ft_Begin(FT_POINTS);

  {
    u8 i;

    for (i = 0; i < 248; i += 8)
    {
      ft_PointSize(((((255 - i) * 10) / 12) << 3));

      ft_ColorRGB(i, i >> 1, i >> 1);
      ft_Vertex2f(150 << 4, 150 << 4);

      ft_ColorRGB(i >> 1, i, i >> 1);
      ft_Vertex2f(400 << 4, 150 << 4);

      ft_ColorRGB(i >> 1, i >> 1, i);
      ft_Vertex2f(650 << 4, 150 << 4);

      ft_ColorRGB(i, i, i >> 1);
      ft_Vertex2f(150 << 4, 450 << 4);

      ft_ColorRGB(i >> 1, i, i);
      ft_Vertex2f(400 << 4, 450 << 4);

      ft_ColorRGB(i, i >> 1, i);
      ft_Vertex2f(650 << 4, 450 << 4);

      ft_ColorRGB(i, i, i);
      ft_Vertex2f(280 << 4, 300 << 4);

      ft_ColorRGB(i, (i * 100) / 255, 0);
      ft_Vertex2f(520 << 4, 300 << 4);
    }
  }

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  while (1);
}
