
bool wc_api__bool(u8 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld a, (hl)
    call _WCAPI
    ld l, #0
    ret z
    inc l
    ret
  __endasm;
}

void wc_api_u16(u8 a, u16 b) __naked
{
  a;  // to avoid SDCC warning
  b;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld a, (hl)
    inc hl
    ld c, (hl)
    inc hl
    ld b, (hl)
    push bc
    pop ix
    jp _WCAPI
  __endasm;
}

bool wc_vmode(u8 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld a, (hl)
    ex af, af
    ld a, #_GVmod
    jp _WCAPI
  __endasm;
}

u16 wc_load512(u8 *a, u8 b) __naked
{
  a;  // to avoid SDCC warning
  b;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld b, (hl)
    ex de, hl
    ld a, #_LOAD512
    jp _WCAPI
  __endasm;
}

void wc_exit(u8 rc)
{
  rc;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld a, (hl)
    ld sp, (_ret_sp)
    ret
  __endasm;
}
