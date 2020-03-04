#include "defs.h"
#include "wc_h.h"
static const WC_HDR hdr =
{
  /* +00 -reserved-           */    "",
  /* +16 Magic string         */    {'W', 'i', 'l', 'd', 'C', 'o', 'm', 'm', 'a', 'n', 'd', 'e', 'r', 'M', 'D', 'L'},
  /* +32 Format version       */    0x0A,
  /* +33 -reserved-           */    "",
  /* +34 Number of pages      */    1,
  /* +35 Page at 0x8000       */    0,
  /* +36 Page definitions     */    {
  /*  +0 Page, +1 NBlocks     */        {0, 18}
  /*                          */    },
  /* +48 -reserved-           */    "",
  /* +64 Extensions           */    {
  /*                          */        {'T', 'R', 'D'},
  /*                          */        {0, 0, 0},
  /*                          */        {0, 0, 0},
  /*                          */        {0, 0, 0}
  /*                          */    },
  /* +160 0x00                */    "",
  /* +161 Max file size       */    0xFFFFFFFF,
  /* +165 Plugin name         */    "TRDispatcher v0.43b",
  /* +197 Starting condition  */    SC_MENU_EMENU,
  /* +198 Text for F4 menu    */    "",
  /* +204 Text for viewer menu*/    "Browse TRD"
};
