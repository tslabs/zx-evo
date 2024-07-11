
#include <defs.h>
#include <wc_h.h>

static const WC_HDR hdr =
{
  /* +00 -reserved-          */   "",
  /* +16 Magic string        */   {'W', 'i', 'l', 'd', 'C', 'o', 'm', 'm', 'a', 'n', 'd', 'e', 'r', 'M', 'D', 'L'},
  /* +32 Format version      */   7,
  /* +33 -reserved-          */   "",
  /* +34 Number of pages     */   1,
  /* +35 Page at 0x8000      */   0,
  /* +36 Page definitions    */   {
  /*  +0 Page, +1 NBlocks    */     {0, N_BLK0}
  /*                         */   },
  /* +48 -reserved-          */   "",
  /* +64 Extensions          */   {
  /*                         */     {'J', 'P', 'G'},
  /*                         */     {'P', 'N', 'G'},
  /*                         */     {'D', 'L', 'S'},
  /*                         */     {'D', 'X', 'P'},
  /*                         */     {'A', 'V', 'I'}
  /*                         */   },
  /* +160 0x00               */   "",
  /* +161 Max file size      */   0xFFFFFFFF,
  /* +165 Plugin name        */   "FT812 Viewer",
  /* +197 Starting condition */   SC_MENU   // also run by extension
};
