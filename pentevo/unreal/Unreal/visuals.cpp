
#include "std.h"
#include "resource.h"
#include "vars.h"
#include "dx/dxrend.h"
#include "visuals.h"

/*
  To Do:
    - TSU shown:
      - all in one screen, scale 1x
      - scale 2x:
        - tiles 0
        - tiles 1
        - sprites
        - palette

    - Show TSU objects in palettes:
      - grayscale - generic for undefined palette
      - tiles: in palette selected for current layer
      - sprites: in selected palette for each sprite
      - user chosen palette (0..15) globally
      - tmap a tiles with indexes

    - Show CRAM
      + color
      - CRAM value
      - RGB value, in current VDAC rendering

    - Show hints when mouse is over an object

    - Editable properties of all objects

    - Analyze tmap to determine exact palette for each tile from tileset

    - Lower pane with current selected modes and hotkeys:
      F1 - TSU Pal: grey, F2 - Tiles 0, etc.

    - Add TSU (right panel) in Debugger
*/

constexpr auto VISUALS_WND_WIDTH = 840;
constexpr auto VISUALS_WND_HEIGHT = 716;
// #define BOX_BORDER_COLOR 0x606060
#define BOX_BORDER_COLOR 0

HWND visuals_wnd;
HMENU visuals_menu;
u32 bitmap[VISUALS_WND_HEIGHT][VISUALS_WND_WIDTH];
VISUALS_t vis;

int visuals_mode = VIS_MODE_OFF;

gdibmp_t visuals_gdibmp = {{{sizeof(BITMAPINFOHEADER), VISUALS_WND_WIDTH, -VISUALS_WND_HEIGHT, 1, 32, BI_RGB, 0}}};

static LRESULT APIENTRY VisualsWndProc(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
	PAINTSTRUCT ps;

	if (uMessage == WM_CLOSE)
	{
		return 0;
	}

	if (uMessage == WM_PAINT)
	{
		const auto bptr = bitmap;
		const auto hdc = BeginPaint(hwnd, &ps);
		SetDIBitsToDevice(hdc, 0, 0, VISUALS_WND_WIDTH, VISUALS_WND_HEIGHT, 0, 0, 0, VISUALS_WND_HEIGHT, bptr, &visuals_gdibmp.header, DIB_RGB_COLORS);
		EndPaint(hwnd, &ps);
		return 0;
	}

	if (uMessage == WM_COMMAND)
	{
		// switch (wparam)
    // {
    // }
	}

	return DefWindowProc(hwnd, uMessage, wparam, lparam);
}

void set_visuals_window_size()
{
  RECT cl_rect;
  const DWORD dw_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

  cl_rect.left = 0;
  cl_rect.top = 0;
  cl_rect.right = VISUALS_WND_WIDTH - 1;
  cl_rect.bottom = VISUALS_WND_HEIGHT - 1;
  AdjustWindowRect(&cl_rect, dw_style, GetMenu(visuals_wnd) != nullptr);
  SetWindowPos(visuals_wnd, nullptr, 0, 0, cl_rect.right - cl_rect.left + 1, cl_rect.bottom - cl_rect.top + 1, SWP_NOMOVE);
}

void init_visuals()
{
	WNDCLASS  wc{};
	const DWORD dw_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	wc.lpfnWndProc = WNDPROC(VisualsWndProc);
	wc.hInstance = hIn;
	wc.lpszClassName = "VISUALS_WND";
	wc.hIcon = LoadIcon(hIn, MAKEINTRESOURCE(IDI_MAIN));
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	// visuals_menu = LoadMenu(hIn, MAKEINTRESOURCE(IDR_DEBUGMENU)); // !!!

	visuals_wnd = CreateWindow("VISUALS_WND", "UnrealSpeccy visuals", dw_style, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, visuals_menu, hIn, NULL);

  set_visuals_window_size();

  memset(vis.pal_map, 0, sizeof(vis.pal_map));
}

void visuals_on_off()
{
  visuals_mode++;

  if (visuals_mode >= VIS_MODE_MAX)
    visuals_mode = VIS_MODE_OFF;

  if (visuals_mode == VIS_MODE_OFF)
  {
    ShowWindow(visuals_wnd, SW_HIDE);
  }
  else
  {
    ShowWindow(visuals_wnd, SW_SHOW);
    flip_visuals();
  }
}

void clear()
{
  memset(bitmap, 0x30, sizeof(bitmap));
}

void box(int x, int y, int w, int h, int c)
{
  for (int i = 0; i <= w; i++)
  {
    bitmap[y][x + i] = c;
    bitmap[y + h][x + i] = c;
  }

  for (int i = 0; i <= h; i++)
  {
    bitmap[y + i][x] = c;
    bitmap[y + i][x + w] = c;
  }
}

void draw_palette_cell(int i, int x, int y)
{
  box(x, y, 15, 12, BOX_BORDER_COLOR);

  for (int b = 1; b < 12; b++)
    for (int a = 1; a < 15; a++)
      bitmap[y + b][x + a] = vid.clut[i];
}

void draw_palette(int x0, int y0)
{
  int i = 0;
  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
      draw_palette_cell(i++, x * 15 + x0, y * 12 + y0);
  }
}

void draw_tile_bitmap(int x0, int y0, int n, int pg, int pal, bool flip = false)
// x, y - screen coords
// n - tile number [0..4095]
// pg - page [0..255]
// pal - palette selector [0..15]
{
  u8 *bm = page_ram(pg);
  int nx = n & 63;
  int ny = n >> 6;
  bm = &bm[nx * 4 + ny * 2048];

  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 4; x++)
    {
      int d = bm[x + y * 256];
      int d0 = (d >> 4) & 15;
      int d1 = d & 15;
      if (d0) bitmap[y + y0][x * 2 + x0] = vid.clut[pal * 16 + d0];
      if (d1) bitmap[y + y0][x * 2 + x0 + 1] = vid.clut[pal * 16 + d1];
    }
}

void draw_bitmap(int x0, int y0, int plane)
{
  int g, p;

  if (plane < 2)  // tiles
  {
    p = (plane ? comp.ts.t1pal : comp.ts.t0pal) << 2;
    g = plane ? comp.ts.t1gpage[2] : comp.ts.t0gpage[2];
  }
  else  // sprites
  {
    g = comp.ts.sgpage;
    p = 0;
  }

  for (int y = 0; y < 64; y++)
  {
    for (int x = 0; x < 64; x++)
    {
      int n = x + y * 64;

      if (vis.palette < 0)
        p = vis.pal_map[plane][n];

      box(x * 9 + x0, y * 9 + y0, 9, 9, BOX_BORDER_COLOR);
      draw_tile_bitmap(x * 9 + x0 + 1, y * 9 + y0 + 1, n, g, p);
    }
  }
}

void draw_sprite(int i, int x0, int y0)
{
  SPRITE_t *spr = (SPRITE_t*)comp.sfile;
  int w = spr[i].xs + 1;
  int h = spr[i].ys + 1;
  int n = spr[i].tnum;
  int g = comp.ts.sgpage;
  int p = spr[i].pal;

  box(x0, y0, 65, 65, BOX_BORDER_COLOR);
  box(x0, y0, (spr[i].xs + 1) * 8 + 1, (spr[i].ys + 1) * 8 + 1, BOX_BORDER_COLOR);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      draw_tile_bitmap(x * 8 + x0 + 1, y * 8 + y0 + 1, n + x + y * 64, g, p);  // !!! add flips
}

void draw_sprites(int x0, int y0)
{
  int i = 0;

  for (int y = 0; y < 11; y++)
  {
    for (int x = 0; x < 8; x++)
    {
      if (i < 85)
        draw_sprite(i++, x * 65 + x0, y * 65 + y0);
    }
  }
}

void draw_visuals()
{
  clear();
  draw_palette(599, 0);

  switch (visuals_mode)
  {
    case VIS_MODE_SPRITES:
      draw_sprites(0, 0);
    break;

    case VIS_MODE_SPRITES_BM:
      draw_bitmap(0, 0, 2);
    break;

    case VIS_MODE_TILES0:
      draw_bitmap(0, 0, 0);
    break;

    case VIS_MODE_TILES1:
      draw_bitmap(0, 0, 1);
    break;
  }
}

void flip_visuals()
{
  draw_visuals();
  InvalidateRect(visuals_wnd, nullptr, FALSE);
}
