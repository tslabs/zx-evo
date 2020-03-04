
#include "defs.h"
#include "wc_api.h"

bool wc_api__bool(u8 a) __naked __z88dk_fastcall
{
    a;          // to avoid SDCC warning

  __asm
    ld a,l
    call _WCAPI
    ld l,#0
    ret z
    inc l
    ret
  __endasm;
}

void wc_api_u8(u8 a) __naked __z88dk_fastcall
{
    a;          // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld a,l
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_api_u16(u8 a, u16 b) __naked
{
    a;          // to avoid SDCC warning
    b;          // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld hl,#6
    add hl,sp
    ld a,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    push bc
    pop ix
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_vmode(u8 vmode) __naked __z88dk_fastcall
{
    vmode;          // to avoid SDCC warning

  __asm
    ld a,l
    ex af,af
    ld a,#_GVmod
    jp _WCAPI
  __endasm;
}

void wc_page3(u8 pg) __naked __z88dk_fastcall
{
    pg;             // to avoid SDCC warning

  __asm
    ld a,l
    ex af,af
    ld a,#_MNGC_PL
    jp _WCAPI
  __endasm;
}

void wc_vpage3(u8 pg) __naked __z88dk_fastcall
{
    pg;             // to avoid SDCC warning

  __asm
    ld a,l
    ex af,af
    ld a,#_MNGCVPL
    jp _WCAPI
  __endasm;
}

void wc_print_window(void *win) __naked __z88dk_fastcall
{
    win;            // to avoid SDCC warning

  __asm
    push ix
    push iy
    push hl
    pop ix
    ld a,#_PRWOW
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_restore_window(void *win) __naked __z88dk_fastcall
{
    win;            // to avoid SDCC warning

  __asm
    push ix
    push iy
    push hl
    pop ix
    ld a,#_RRESB
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_print_cursor(WC_MNU_WINDOW *win) __naked __z88dk_fastcall
{
    win;            // to avoid SDCC warning

  __asm
    push ix
    push iy
    push hl
    pop ix
    ld a,#_CURSOR
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_restore_cursor(WC_MNU_WINDOW *win) __naked __z88dk_fastcall
{
    win;            // to avoid SDCC warning

  __asm
    push ix
    push iy
    push hl
    pop ix
    ld a,#_CURSER
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_print(void *win, u8 x, u8 y, char *str) __naked
{
    win, x, y;      // to avoid SDCC warning
    str;            // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld ix,#6
    add ix,sp
    exx
    ld l,0 (ix)
    ld h,1 (ix)
    push hl
    exx
    ld e,2 (ix)
    ld d,3 (ix)
    ld l,4 (ix)
    ld h,5 (ix)
    pop ix
    ld a,#_TXTPR
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

void wc_print_w(void *win, u8 x, u8 y, char *str, u16 length) __naked
{
    win, x, y;      // to avoid SDCC warning
    str;            // to avoid SDCC warning
    length;         // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld ix,#6
    add ix,sp
    exx
    ld l,0 (ix)
    ld h,1 (ix)
    push hl
    exx
    ld e,2 (ix)
    ld d,3 (ix)
    ld l,4 (ix)
    ld h,5 (ix)
    ld c,6 (ix)
    ld b,7 (ix)
    pop ix
    ld a,#_PRSRW
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

u8 wc_keyscan() __naked
{
  __asm
    ld a,#0x01
    ex af,af
    ld a,#_KBSCN
    call _WCAPI
    ld l,a
    ret
  __endasm;
}

u16 wc_load256(u8 *a, u8 b) __naked
{
    a, b;           // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld hl,#6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    ld a,#_LOAD256
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

u16 wc_load512(u8 *a, u8 b) __naked
{
    a, b;           // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld hl,#6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    ld a,#_LOAD512
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

u16 wc_save512(u8 *a, u8 b) __naked
{
    a;              // to avoid SDCC warning
    b;              // to avoid SDCC warning

  __asm
    push ix
    push iy
    ld hl,#6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    ld a,#_SAVE512
    call _WCAPI
    pop iy
    pop ix
    ret
  __endasm;
}

u8 wc_mkfile(WC_FILENAME *filename) __naked __z88dk_fastcall
{
    filename;       // to avoid SDCC warning

  __asm
    push ix
    ld a,#_MKfilenew
    call _WCAPI
    ld l,a
    pop ix
    ret
  __endasm;
}

u16 wc_get_marked_file(u16 num, char *buff) __naked
{
    num;            // to avoid SDCC warning
    buff;           // to avoid SDCC warning

  __asm
    push ix
    ld ix,#4
    add ix,sp
    ld l,0 (ix)
    ld h,1 (ix)
    ld e,2 (ix)
    ld d,3 (ix)
    ld ix,(_wc_actpanel)
    ld a,#_TMRKDFL
    call _WCAPI
    pop ix
    ld l,c
    ld h,b
    ret
  __endasm;
}

u32 wc_ffind(u8 *filename) __naked __z88dk_fastcall
{
    filename;       // to avoid SDCC warning

  __asm
    push ix
    ld a,#_FENTRY
    call _WCAPI
    pop ix
    ret
  __endasm;
}

void wc_exit(u8 rc)
{
    rc;             // to avoid SDCC warning

  __asm
    ld hl,#2
    add hl,sp
    ld a,(hl)
    ld sp,(_wc_ret_sp)
    ret
  __endasm;
}
