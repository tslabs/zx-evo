#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_atmf.h"
#include "dxr_atm2.h"

const int atmh0_ofs = 0;
const int atmh1_ofs = 0x2000;
const int atmh2_ofs = -4*int(PAGE);
const int atmh3_ofs = -4*int(PAGE)+0x2000;

void line_atm2_8(unsigned char *dst, unsigned char *src, unsigned *tab0, int src_offset)
{
    for (unsigned x = 0; x < 640; x += 0x40) {
        src_offset &= 0x1FFF;
        unsigned s  = *(unsigned*)(src+atmh0_ofs+src_offset);
        unsigned t  = *(unsigned*)(src+atmh1_ofs+src_offset);
        unsigned as = *(unsigned*)(src+atmh2_ofs+src_offset);
        unsigned at = *(unsigned*)(src+atmh3_ofs+src_offset);
        unsigned *tab = tab0 + ((as << 4) & 0xFF0);
        *(unsigned*)(dst+x+0x00) = tab[((s >> 4)  & 0xF)];
        *(unsigned*)(dst+x+0x04) = tab[((s >> 0)  & 0xF)];
        tab = tab0 + ((at << 4) & 0xFF0);
        *(unsigned*)(dst+x+0x08) = tab[((t >> 4)  & 0xF)];
        *(unsigned*)(dst+x+0x0C) = tab[((t >> 0)  & 0xF)];
        tab = tab0 + ((as >> 4) & 0xFF0);
        *(unsigned*)(dst+x+0x10) = tab[((s >> 12) & 0xF)];
        *(unsigned*)(dst+x+0x14) = tab[((s >> 8)  & 0xF)];
        tab = tab0 + ((at >> 4) & 0xFF0);
        *(unsigned*)(dst+x+0x18) = tab[((t >> 12) & 0xF)];
        *(unsigned*)(dst+x+0x1C) = tab[((t >> 8)  & 0xF)];
        tab = tab0 + ((as >> 12) & 0xFF0);
        *(unsigned*)(dst+x+0x20) = tab[((s >> 20) & 0xF)];
        *(unsigned*)(dst+x+0x24) = tab[((s >> 16) & 0xF)];
        tab = tab0 + ((at >> 12) & 0xFF0);
        *(unsigned*)(dst+x+0x28) = tab[((t >> 20) & 0xF)];
        *(unsigned*)(dst+x+0x2C) = tab[((t >> 16) & 0xF)];
        tab = tab0 + ((as >> 20) & 0xFF0);
        *(unsigned*)(dst+x+0x30) = tab[((s >> 28) & 0xF)];
        *(unsigned*)(dst+x+0x34) = tab[((s >> 24) & 0xF)];
        tab = tab0 + ((at >> 20) & 0xFF0);
        *(unsigned*)(dst+x+0x38) = tab[((t >> 28) & 0xF)];
        *(unsigned*)(dst+x+0x3C) = tab[((t >> 24) & 0xF)];
        src_offset+=4;
    }
}


void line_atm2_16(unsigned char *dst, unsigned char *src, unsigned *tab0, int src_offset)
{
    for (unsigned x = 0; x < 640*2; x += 0x80) {
        src_offset &= 0x1FFF;
        unsigned s  = *(unsigned*)(src+atmh0_ofs+src_offset);
        unsigned t  = *(unsigned*)(src+atmh1_ofs+src_offset);
        unsigned as = *(unsigned*)(src+atmh2_ofs+src_offset);
        unsigned at = *(unsigned*)(src+atmh3_ofs+src_offset);
        unsigned *tab = tab0 + ((as << 2) & 0x3FC);
        *(unsigned*)(dst+x+0x00) = tab[((s >> 6)  & 3)];
        *(unsigned*)(dst+x+0x04) = tab[((s >> 4)  & 3)];
        *(unsigned*)(dst+x+0x08) = tab[((s >> 2)  & 3)];
        *(unsigned*)(dst+x+0x0C) = tab[((s >> 0)  & 3)];
        tab = tab0 + ((at << 2) & 0x3FC);
        *(unsigned*)(dst+x+0x10) = tab[((t >> 6)  & 3)];
        *(unsigned*)(dst+x+0x14) = tab[((t >> 4)  & 3)];
        *(unsigned*)(dst+x+0x18) = tab[((t >> 2)  & 3)];
        *(unsigned*)(dst+x+0x1C) = tab[((t >> 0)  & 3)];
        tab = tab0 + ((as >> 6) & 0x3FC);
        *(unsigned*)(dst+x+0x20) = tab[((s >>14)  & 3)];
        *(unsigned*)(dst+x+0x24) = tab[((s >>12)  & 3)];
        *(unsigned*)(dst+x+0x28) = tab[((s >>10)  & 3)];
        *(unsigned*)(dst+x+0x2C) = tab[((s >> 8)  & 3)];
        tab = tab0 + ((at >> 6) & 0x3FC);
        *(unsigned*)(dst+x+0x30) = tab[((t >>14)  & 3)];
        *(unsigned*)(dst+x+0x34) = tab[((t >>12)  & 3)];
        *(unsigned*)(dst+x+0x38) = tab[((t >>10)  & 3)];
        *(unsigned*)(dst+x+0x3C) = tab[((t >> 8)  & 3)];
        tab = tab0 + ((as >> 14) & 0x3FC);
        *(unsigned*)(dst+x+0x40) = tab[((s >>22)  & 3)];
        *(unsigned*)(dst+x+0x44) = tab[((s >>20)  & 3)];
        *(unsigned*)(dst+x+0x48) = tab[((s >>18)  & 3)];
        *(unsigned*)(dst+x+0x4C) = tab[((s >>16)  & 3)];
        tab = tab0 + ((at >> 14) & 0x3FC);
        *(unsigned*)(dst+x+0x50) = tab[((t >>22)  & 3)];
        *(unsigned*)(dst+x+0x54) = tab[((t >>20)  & 3)];
        *(unsigned*)(dst+x+0x58) = tab[((t >>18)  & 3)];
        *(unsigned*)(dst+x+0x5C) = tab[((t >>16)  & 3)];
        tab = tab0 + ((as >> 22) & 0x3FC);
        *(unsigned*)(dst+x+0x60) = tab[((s >>30)  & 3)];
        *(unsigned*)(dst+x+0x64) = tab[((s >>28)  & 3)];
        *(unsigned*)(dst+x+0x68) = tab[((s >>26)  & 3)];
        *(unsigned*)(dst+x+0x6C) = tab[((s >>24)  & 3)];
        tab = tab0 + ((at >> 22) & 0x3FC);
        *(unsigned*)(dst+x+0x70) = tab[((t >>30)  & 3)];
        *(unsigned*)(dst+x+0x74) = tab[((t >>28)  & 3)];
        *(unsigned*)(dst+x+0x78) = tab[((t >>26)  & 3)];
        *(unsigned*)(dst+x+0x7C) = tab[((t >>24)  & 3)];
        src_offset+=4;
    }
}



void line_atm2_32(unsigned char *dst, unsigned char *src, unsigned *tab0, int src_offset)
{
    unsigned *tab; unsigned c;
    for (unsigned x = 0; x < 640*4; x += 0x40) {
        src_offset &= 0x1FFF;

        tab = tab0 + src[atmh2_ofs+src_offset]; 
        c = src[atmh0_ofs+src_offset];
        *(unsigned*)(dst+x+0x00) = tab[((c << 1) & 0x100)];
        *(unsigned*)(dst+x+0x04) = tab[((c << 2) & 0x100)];
        *(unsigned*)(dst+x+0x08) = tab[((c << 3) & 0x100)];
        *(unsigned*)(dst+x+0x0C) = tab[((c << 4) & 0x100)];
        *(unsigned*)(dst+x+0x10) = tab[((c << 5) & 0x100)];
        *(unsigned*)(dst+x+0x14) = tab[((c << 6) & 0x100)];
        *(unsigned*)(dst+x+0x18) = tab[((c << 7) & 0x100)];
        *(unsigned*)(dst+x+0x1C) = tab[((c << 8) & 0x100)];

        tab = tab0 + src[atmh3_ofs+src_offset]; 
        c = src[atmh1_ofs+src_offset];
        *(unsigned*)(dst+x+0x20) = tab[((c << 1) & 0x100)];
        *(unsigned*)(dst+x+0x24) = tab[((c << 2) & 0x100)];
        *(unsigned*)(dst+x+0x28) = tab[((c << 3) & 0x100)];
        *(unsigned*)(dst+x+0x2C) = tab[((c << 4) & 0x100)];
        *(unsigned*)(dst+x+0x30) = tab[((c << 5) & 0x100)];
        *(unsigned*)(dst+x+0x34) = tab[((c << 6) & 0x100)];
        *(unsigned*)(dst+x+0x38) = tab[((c << 7) & 0x100)];
        *(unsigned*)(dst+x+0x3C) = tab[((c << 8) & 0x100)];

        ++src_offset;
    }
}

// Hardware Multicolor
void rend_atm2(unsigned char *dst, unsigned pitch, int y, int Offset)
{
    unsigned char *dst2 = dst + (temp.ox-640)*temp.obpp/16;
    if (temp.scy > 200) 
        dst2 += (temp.scy-200)/2*pitch * ((temp.oy > temp.scy)?2:1);

    if (conf.fast_sl) {
        dst2 += y*pitch;
        switch (temp.obpp)
        {
        case 8:
            line_atm2_8 (dst2, temp.base, t.zctab8 [0], Offset);
            break;
        case 16:
            line_atm2_16(dst2, temp.base, t.zctab16[0], Offset);
            break;
        case 32:
            line_atm2_32(dst2, temp.base, t.zctab32[0],  Offset);
            break;
        }
    } else {
        dst2 += 2*y*pitch;
        switch (temp.obpp)
        {
        case 8:
            line_atm2_8 (dst2, temp.base, t.zctab8 [0], Offset);
            dst2 += pitch;
            line_atm2_8 (dst2, temp.base, t.zctab8 [1], Offset);
            break;
        case 16:
            line_atm2_16(dst2, temp.base, t.zctab16[0], Offset);
            dst2 += pitch;
            line_atm2_16(dst2, temp.base, t.zctab16[1], Offset);
            break;
        case 32:
            line_atm2_32(dst2, temp.base, t.zctab32[0], Offset);
            dst2 += pitch;
            line_atm2_32(dst2, temp.base, t.zctab32[1], Offset);
            break;
        }
    }
}
