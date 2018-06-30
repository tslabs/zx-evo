
#include "defs.h"
#include "sdklib.h"

// Key functions
u8 getkey() __naked
{
  __asm
    ld bc, #0xFEFE
    ld l, #0
  k1:
    in a, (c)
    ld h, #5
  k2:
    rra
    ret nc
    inc l
    dec h
    jr nz, k2
    rlc b
    jr c, k1
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
