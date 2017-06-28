
#include "defs.h"
#include "ts.h"
#include "tslib.h"

void ts_wreg(u8 a, u8 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld b, (hl)
    inc hl
    ld c, #0xAF
    ld a, (hl)
    out (c), a
    ret
  __endasm;
}
