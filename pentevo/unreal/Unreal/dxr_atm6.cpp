#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_atmf.h"
#include "dxr_atm6.h"
#include "fontatm2.h"

const int text0_ofs = 0x0;
const int text1_ofs = 0x2000;
const int text2_ofs = -4*int(PAGE)+1;
const int text3_ofs = -4*int(PAGE)+0x2000;

void line_atm6_8(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font, int src_offset)
{
    for (unsigned x = 0; x < 640; x += 0x40) {
        src_offset &= 0x1FFF;
        unsigned p0 = *(unsigned*)(src + src_offset + text0_ofs),
                 p1 = *(unsigned*)(src + src_offset + text1_ofs),
                 a0 = *(unsigned*)(src + src_offset + text2_ofs),
                 a1 = *(unsigned*)(src + src_offset + text3_ofs);
        unsigned c, *tab;
        tab = tab0 + ((a1 << 4) & 0xFF0), c = font[p0 & 0xFF];
        *(unsigned*)(dst+x+0x00) = tab[((c >> 4)  & 0xF)];
        *(unsigned*)(dst+x+0x04) = tab[c & 0xF];
        tab = tab0 + ((a0 << 4) & 0xFF0); c = font[p1 & 0xFF];
        *(unsigned*)(dst+x+0x08) = tab[((c >> 4)  & 0xF)];
        *(unsigned*)(dst+x+0x0C) = tab[c & 0xF];
        tab = tab0 + ((a1 >> 4) & 0xFF0); c = font[(p0 >> 8) & 0xFF];
        *(unsigned*)(dst+x+0x10) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x14) = tab[c & 0xF];
        tab = tab0 + ((a0 >> 4) & 0xFF0); c = font[(p1 >> 8) & 0xFF];
        *(unsigned*)(dst+x+0x18) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x1C) = tab[c & 0xF];
        tab = tab0 + ((a1 >> 12) & 0xFF0); c = font[(p0 >> 16) & 0xFF];
        *(unsigned*)(dst+x+0x20) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x24) = tab[c & 0xF];
        tab = tab0 + ((a0 >> 12) & 0xFF0); c = font[(p1 >> 16) & 0xFF];
        *(unsigned*)(dst+x+0x28) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x2C) = tab[c & 0xF];
        tab = tab0 + ((a1 >> 20) & 0xFF0); c = font[p0 >> 24];
        *(unsigned*)(dst+x+0x30) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x34) = tab[c & 0xF];
        tab = tab0 + ((a0 >> 20) & 0xFF0); c = font[p1 >> 24];
        *(unsigned*)(dst+x+0x38) = tab[((c >> 4) & 0xF)];
        *(unsigned*)(dst+x+0x3C) = tab[c & 0xF];
        src_offset += 4;
    }
}

void line_atm6_16(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font, int src_offset)
{
    for (unsigned x = 0; x < 640*2; x += 0x80) {
        src_offset &= 0x1FFF;
        unsigned p0 = *(unsigned*)(src + src_offset + text0_ofs),
                 p1 = *(unsigned*)(src + src_offset + text1_ofs),
                 a0 = *(unsigned*)(src + src_offset + text2_ofs),
                 a1 = *(unsigned*)(src + src_offset + text3_ofs);
        unsigned c, *tab;
        tab = tab0 + ((a1 << 2) & 0x3FC), c = font[p0 & 0xFF];
        *(unsigned*)(dst+x+0x00) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x04) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x08) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x0C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a0 << 2) & 0x3FC); c = font[p1 & 0xFF];
        *(unsigned*)(dst+x+0x10) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x14) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x18) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x1C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a1 >> 6) & 0x3FC); c = font[(p0 >> 8) & 0xFF];
        *(unsigned*)(dst+x+0x20) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x24) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x28) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x2C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a0 >> 6) & 0x3FC); c = font[(p1 >> 8) & 0xFF];
        *(unsigned*)(dst+x+0x30) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x34) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x38) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x3C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a1 >> 14) & 0x3FC); c = font[(p0 >> 16) & 0xFF];
        *(unsigned*)(dst+x+0x40) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x44) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x48) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x4C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a0 >> 14) & 0x3FC); c = font[(p1 >> 16) & 0xFF];
        *(unsigned*)(dst+x+0x50) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x54) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x58) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x5C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a1 >> 22) & 0x3FC); c = font[p0 >> 24];
        *(unsigned*)(dst+x+0x60) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x64) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x68) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x6C) = tab[((c >> 0)  & 0x03)];
        tab = tab0 + ((a0 >> 22) & 0x3FC); c = font[p1 >> 24];
        *(unsigned*)(dst+x+0x70) = tab[((c >> 6)  & 0x03)];
        *(unsigned*)(dst+x+0x74) = tab[((c >> 4)  & 0x03)];
        *(unsigned*)(dst+x+0x78) = tab[((c >> 2)  & 0x03)];
        *(unsigned*)(dst+x+0x7C) = tab[((c >> 0)  & 0x03)];
        src_offset += 4;
    }
}

void line_atm6_32(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font, int src_offset)
{
   for (unsigned x = 0; x < 640*4; x += 0x80) {
      unsigned c, *tab;
      src_offset &= 0x1FFF;
      tab = tab0 + src[src_offset + text3_ofs]; 
      c = font[src[src_offset + text0_ofs]];
      *(unsigned*)(dst+x+0x00) = tab[((c << 1) & 0x100)];
      *(unsigned*)(dst+x+0x04) = tab[((c << 2) & 0x100)];
      *(unsigned*)(dst+x+0x08) = tab[((c << 3) & 0x100)];
      *(unsigned*)(dst+x+0x0C) = tab[((c << 4) & 0x100)];
      *(unsigned*)(dst+x+0x10) = tab[((c << 5) & 0x100)];
      *(unsigned*)(dst+x+0x14) = tab[((c << 6) & 0x100)];
      *(unsigned*)(dst+x+0x18) = tab[((c << 7) & 0x100)];
      *(unsigned*)(dst+x+0x1C) = tab[((c << 8) & 0x100)];

      tab = tab0 + src[src_offset + text2_ofs]; 
      c = font[src[src_offset + text1_ofs]];
      *(unsigned*)(dst+x+0x20) = tab[((c << 1) & 0x100)];
      *(unsigned*)(dst+x+0x24) = tab[((c << 2) & 0x100)];
      *(unsigned*)(dst+x+0x28) = tab[((c << 3) & 0x100)];
      *(unsigned*)(dst+x+0x2C) = tab[((c << 4) & 0x100)];
      *(unsigned*)(dst+x+0x30) = tab[((c << 5) & 0x100)];
      *(unsigned*)(dst+x+0x34) = tab[((c << 6) & 0x100)];
      *(unsigned*)(dst+x+0x38) = tab[((c << 7) & 0x100)];
      *(unsigned*)(dst+x+0x3C) = tab[((c << 8) & 0x100)];

      tab = tab0 + src[src_offset + text3_ofs+1]; 
      c = font[src[src_offset + text0_ofs+1]];
      *(unsigned*)(dst+x+0x40) = tab[((c << 1) & 0x100)];
      *(unsigned*)(dst+x+0x44) = tab[((c << 2) & 0x100)];
      *(unsigned*)(dst+x+0x48) = tab[((c << 3) & 0x100)];
      *(unsigned*)(dst+x+0x4C) = tab[((c << 4) & 0x100)];
      *(unsigned*)(dst+x+0x50) = tab[((c << 5) & 0x100)];
      *(unsigned*)(dst+x+0x54) = tab[((c << 6) & 0x100)];
      *(unsigned*)(dst+x+0x58) = tab[((c << 7) & 0x100)];
      *(unsigned*)(dst+x+0x5C) = tab[((c << 8) & 0x100)];

      tab = tab0 + src[src_offset + text2_ofs+1]; 
      c = font[src[src_offset + text1_ofs+1]];
      *(unsigned*)(dst+x+0x60) = tab[((c << 1) & 0x100)];
      *(unsigned*)(dst+x+0x64) = tab[((c << 2) & 0x100)];
      *(unsigned*)(dst+x+0x68) = tab[((c << 3) & 0x100)];
      *(unsigned*)(dst+x+0x6C) = tab[((c << 4) & 0x100)];
      *(unsigned*)(dst+x+0x70) = tab[((c << 5) & 0x100)];
      *(unsigned*)(dst+x+0x74) = tab[((c << 6) & 0x100)];
      *(unsigned*)(dst+x+0x78) = tab[((c << 7) & 0x100)];
      *(unsigned*)(dst+x+0x7C) = tab[((c << 8) & 0x100)];

      src_offset += 2;
   }
}

// Textmode
void rend_atm6(unsigned char *dst, unsigned pitch, int y, int Offset)
{
   unsigned char *dst2 = dst + (temp.ox-640)*temp.obpp/16;
   if (temp.scy > 200) 
       dst2 += (temp.scy-200)/2*pitch * ((temp.oy > temp.scy)?2:1);

    int v = y%8;
    if (conf.fast_sl) {
        dst2 += y*pitch;
        switch(temp.obpp)
        {
        case 8:
            line_atm6_8 (dst2, temp.base, t.zctab8 [0], fontatm2 + v*0x100, Offset); 
            break;
        case 16:
            line_atm6_16(dst2, temp.base, t.zctab16[0], fontatm2 + v*0x100, Offset); 
            break;
        case 32:
            line_atm6_32(dst2, temp.base, t.zctab32[0], fontatm2 + v*0x100, Offset); 
            break;
        }
    } else {
        dst2 += 2*y*pitch;
        switch(temp.obpp)
        {
        case 8:
            line_atm6_8 (dst2, temp.base, t.zctab8 [0], fontatm2 + v*0x100, Offset); 
            dst2 += pitch;
            line_atm6_8 (dst2, temp.base, t.zctab8 [1], fontatm2 + v*0x100, Offset);
            break;
        case 16:
            line_atm6_16(dst2, temp.base, t.zctab16[0], fontatm2 + v*0x100, Offset); 
            dst2 += pitch;
            line_atm6_16(dst2, temp.base, t.zctab16[1], fontatm2 + v*0x100, Offset);
            break;
        case 32:
            line_atm6_32(dst2, temp.base, t.zctab32[0], fontatm2 + v*0x100, Offset); 
            dst2 += pitch;
            line_atm6_32(dst2, temp.base, t.zctab32[1], fontatm2 + v*0x100, Offset);
            break;
        }
    }
}
