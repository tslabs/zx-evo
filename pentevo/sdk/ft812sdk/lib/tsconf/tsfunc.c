
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

u8 ts_rreg(u8 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld b, (hl)
    ld c, #0xAF
    in l, (c)
    ret
  __endasm;
}

void ts_set_dma_saddr(u32 a)
{
  ts_wreg(TS_DMASADDRL, (u8)a);
  ts_wreg(TS_DMASADDRH, (u8)(a >> 8));
  ts_wreg(TS_DMASADDRX, (u8)(a >> 14));
}

void ts_set_dma_daddr(u32 a)
{
  ts_wreg(TS_DMADADDRL, (u8)a);
  ts_wreg(TS_DMADADDRH, (u8)(a >> 8));
  ts_wreg(TS_DMADADDRX, (u8)(a >> 14));
}

void ts_set_dma_saddr_p(u16 a, u8 p)
{
  ts_wreg(TS_DMASADDRL, (u8)a);
  ts_wreg(TS_DMASADDRH, (u8)(a >> 8));
  ts_wreg(TS_DMASADDRX, p);
}

void ts_set_dma_daddr_p(u16 a, u8 p)
{
  ts_wreg(TS_DMADADDRL, (u8)a);
  ts_wreg(TS_DMADADDRH, (u8)(a >> 8));
  ts_wreg(TS_DMADADDRX, p);
}

void ts_set_dma_size(u16 l, u16 n)
{
  ts_wreg(TS_DMALEN, (u8)((l >> 1) - 1));
  ts_wreg(TS_DMANUM, (u8)(n - 1));
}

void ts_dma_start(u8 mode)
{
  ts_wreg(TS_DMACTR, mode);
}

void ts_dma_wait() __naked
{
  __asm
    ld b, #TS_DMASTATUS
    ld c, #0xAF
1$: 
    .db 0xED, 0x70   // INF
    jp m, 1$
    ret
  __endasm;
}
