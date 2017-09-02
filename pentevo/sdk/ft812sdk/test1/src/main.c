
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>

u8 cmdl[0x180];

void print_string(const char *s, u16 x, u16 y)
{
  int i = 0;
  char c;

  while (c = s[i++])
  {
    ft_Cell(c);
    ft_Vertex2f(x, y);
    x += 9;
  }
}

void main()
{
  ts_wreg(TS_VCONFIG, 4);
  ft_init(FT_MODE_7);

  ft_ccmd_start(cmdl);
  ft_VertexFormat(0);
  ft_ClearColorRGB(0, 0, 0);
  ft_Clear(1, 1, 1);

  ft_ColorRGB(255, 255, 255);
  // ft_ColorRGB(255, 0, 0);
  ft_LineWidth(24);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(1023, 0);
  ft_Vertex2f(1023, 767);
  ft_Vertex2f(0, 767);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(799, 0);
  ft_Vertex2f(799, 599);
  ft_Vertex2f(0, 599);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(639, 0);
  ft_Vertex2f(639, 479);
  ft_Vertex2f(0, 479);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_POINTS);
  ft_PointSize(100 << 4);
  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);
  ft_ColorRGB(255, 0, 0);
  ft_Vertex2f(rsin(80, 32768) + 320, rcos(80, 32768) + 240);
  ft_ColorRGB(0, 255, 0);
  ft_Vertex2f(rsin(80, 21845 + 32768) + 320, rcos(80, 21845 + 32768) + 240);
  ft_ColorRGB(0, 0, 255);
  ft_Vertex2f(rsin(80, 43690 - 32768) + 320, rcos(80, 43690 - 32768) + 240);

  ft_Begin(FT_BITMAPS);
  ft_BitmapHandle(18);
  ft_ColorRGB(120, 100, 255);
  print_string("640x480", 572, 461);
  print_string("800x600", 732, 581);
  print_string("1024x768", 947, 749);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  while (1);
}
