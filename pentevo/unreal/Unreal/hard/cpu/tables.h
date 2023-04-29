#pragma once
#include "sysdefs.h"

extern const u8 rl0[];
extern const u8 rl1[];

extern const u8 rr0[];
extern const u8 rr1[];

extern u8 rol[];
extern u8 ror[];

extern u8 rlcaf[];
extern u8 rrcaf[];
extern const u8 rlcf[];
extern const u8 rrcf[];

extern const u8 sraf[];
extern u8 log_f[];

extern const u8 incf[];
extern const u8 decf[];
extern u8 adcf[];
extern u8 sbcf[];

extern u8 cpf[];
extern u8 cpf8b[];

Z80INLINE void and8(Z80 *cpu, u8 src)
{
   cpu->a &= src;
   cpu->f = log_f[cpu->a] | HF;
}

Z80INLINE void or8(Z80 *cpu, u8 src)
{
   cpu->a |= src;
   cpu->f = log_f[cpu->a];
}

Z80INLINE void xor8(Z80 *cpu, u8 src)
{
   cpu->a ^= src;
   cpu->f = log_f[cpu->a];
}

Z80INLINE void bitmem(Z80 *cpu, u8 src, u8 bit)
{
   cpu->f = log_f[src & (1 << bit)] | HF | (cpu->f & CF);
   cpu->f = (cpu->f & ~(F3|F5)) | (cpu->memh & (F3|F5));
}

Z80INLINE void set(u8 &src, u8 bit)
{
   src |= (1 << bit);
}

Z80INLINE void res(u8 &src, u8 bit)
{
   src &= ~(1 << bit);
}

Z80INLINE void bit(Z80 *cpu, u8 src, u8 bit)
{
   cpu->f = log_f[src & (1 << bit)] | HF | (cpu->f & CF) | (src & (F3|F5));
}

Z80INLINE u8 resbyte(u8 src, u8 bit)
{
   return src & ~(1 << bit);
}

Z80INLINE u8 setbyte(u8 src, u8 bit)
{
   return src | (1 << bit);
}

Z80INLINE void inc8(Z80 *cpu, u8 &x)
{
   cpu->f = incf[x] | (cpu->f & CF);
   x++;
}

Z80INLINE void dec8(Z80 *cpu, u8 &x)
{
   cpu->f = decf[x] | (cpu->f & CF);
   x--;
}

Z80INLINE void add8(Z80 *cpu, u8 src)
{
   cpu->f = adcf[cpu->a + src*0x100];
   cpu->a += src;
}

Z80INLINE void sub8(Z80 *cpu, u8 src)
{
   cpu->f = sbcf[cpu->a*0x100 + src];
   cpu->a -= src;
}

Z80INLINE void adc8(Z80 *cpu, u8 src)
{
   u8 carry = ((cpu->f) & CF);
   cpu->f = adcf[cpu->a + src*0x100 + 0x10000*carry];
   cpu->a += src + carry;
}

Z80INLINE void sbc8(Z80 *cpu, u8 src)
{
   u8 carry = ((cpu->f) & CF);
   cpu->f = sbcf[cpu->a*0x100 + src + 0x10000*carry];
   cpu->a -= src + carry;
}

Z80INLINE void cp8(Z80 *cpu, u8 src)
{
   cpu->f = cpf[cpu->a*0x100 + src];
}

void init_z80tables();
