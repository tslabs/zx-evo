#pragma once

#include "sysdefs.h"

enum
{
  VIS_MODE_OFF,
  VIS_MODE_SPRITES,
  VIS_MODE_SPRITES_BM,
  VIS_MODE_TILES0,
  VIS_MODE_TILES1,
  VIS_MODE_MAX
};

typedef struct
{
  int palette;
  u8 pal_map[3][4096];
} VISUALS_t;

extern HWND visuals_wnd;
extern VISUALS_t vis;

void init_visuals();
void visuals_on_off();
void draw_visuals();
void flip_visuals();

