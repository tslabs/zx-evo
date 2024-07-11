
#include "defs.h"
#include "sdklib.h"

// Key functions
u8 getkey() __naked
{
  __asm
    ld bc, #0xFEFE
    ld l, #40
  k1:
    in e, (c)
    ld h, #5
  k2:
    rr e
    jr c, k3
    
    ld a, l
    and #63
    cp #KEY_CS_
    jr z, k3
    cp #KEY_SS_
    jr nz, k4
    
  k3:
    dec l
    dec h
    jr nz, k2
    rlc b
    jr c, k1
    
  k4:  
    ld a, #0xFE
    in a, (#0xFE)
    rrca
    jr c, k5
    set 6, l  // set CS flag
  k5:
    ld a, #0x7F
    in a, (#0xFE)
    rrca
    rrca
    ret c
    set 7, l  // set SS flag
    ret
  __endasm;
}

void anykey_press() __naked
{
  __asm
  ak1: 
    xor a
    in a, (254)
    cpl
    and #0x1F
    jr z, ak1
    ret
  __endasm;
}

void anykey_release() __naked
{
  __asm
  ak2: 
    xor a
    in a, (254)
    cpl
    and #0x1F
    jr nz, ak2
    ret
  __endasm;
}

void anykey()
{
  anykey_release();
  anykey_press();
}

// Screen functions
void putc(u8 c)
{
  if((c == '\r') || (c == '\n'))
  {
    // +++ CR
    return;
  }
  else
  {
    // +++ print char
  }
}

_PUTCHAR_PROTO
{
  putc(c);
  return 0;
}

// CPU functions
void di() __naked
{
  __asm
    di
    ret
  __endasm;
}

void ei() __naked
{
  __asm
    ei
    ret
  __endasm;
}
