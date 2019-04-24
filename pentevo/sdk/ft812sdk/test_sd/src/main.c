
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>
#include <pff.h>
#include <diskio.h>
#include "sdc.h"
#include "../res/font.h"

FATFS fs;
DIR dir;
FILINFO fil;
UINT num_read;
UINT num_write;
u8 menu;
bool req_unpress;
u8 sdbuf[64];
u16 bitmax;
u32 sd_size;

#include "func.c"
#include "sdc.c"
#include "print.c"
#include "menu.c"

void init_runtime()
{
  ts_wreg(TS_PAGE3, 0xF7);
  memcpy((void*)0xC000, code_866_fnt, sizeof(code_866_fnt));
  
  menu = M_MAIN;
}

void main()
{
  init_runtime();

  while (1)
  {
    req_unpress = true;
    menu_disp();

    while (1)
    {
      if (key_disp())
        break;
    }
  }
}
