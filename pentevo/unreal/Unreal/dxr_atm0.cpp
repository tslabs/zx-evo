#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_atmf.h"
#include "dxr_atm0.h"

static const int ega0_ofs = -4*int(PAGE);
static const int ega1_ofs = 0;
static const int ega2_ofs = -4*int(PAGE)+0x2000;
static const int ega3_ofs = 0x2000;

static void line_atm0_8(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u8 *d = (u8 *)dst;
    for (unsigned x = 0; x < 320; x += 8, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0] = tab[src[ega0_ofs + src_offset]];
        d[x+1] = tab[src[ega0_ofs + src_offset]] >> 16;
        d[x+2] = tab[src[ega1_ofs + src_offset]];
        d[x+3] = tab[src[ega1_ofs + src_offset]] >> 16;
        d[x+4] = tab[src[ega2_ofs + src_offset]];
        d[x+5] = tab[src[ega2_ofs + src_offset]] >> 16;
        d[x+6] = tab[src[ega3_ofs + src_offset]];
        d[x+7] = tab[src[ega3_ofs + src_offset]] >> 16;
    }
}

static void line_atm0_8d(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u32 *d = (u32 *)dst;
    for (unsigned x = 0; x < 640/4; x += 4, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0] = tab[src[ega0_ofs + src_offset]];
        d[x+1] = tab[src[ega1_ofs + src_offset]];
        d[x+2] = tab[src[ega2_ofs + src_offset]];
        d[x+3] = tab[src[ega3_ofs + src_offset]];
    }
}

static void line_atm0_16(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u16 *d = (u16 *)dst;
    for (unsigned x = 0; x < 320; x += 8, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0] = tab[0+2*src[ega0_ofs + src_offset]];
        d[x+1] = tab[1+2*src[ega0_ofs + src_offset]];
        d[x+2] = tab[0+2*src[ega1_ofs + src_offset]];
        d[x+3] = tab[1+2*src[ega1_ofs + src_offset]];
        d[x+4] = tab[0+2*src[ega2_ofs + src_offset]];
        d[x+5] = tab[1+2*src[ega2_ofs + src_offset]];
        d[x+6] = tab[0+2*src[ega3_ofs + src_offset]];
        d[x+7] = tab[1+2*src[ega3_ofs + src_offset]];
    }
}

static void line_atm0_16d(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u32 *d = (u32 *)dst;
    for (unsigned x = 0; x < 640/2; x += 8, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0] = tab[0+2*src[ega0_ofs + src_offset]];
        d[x+1] = tab[1+2*src[ega0_ofs + src_offset]];
        d[x+2] = tab[0+2*src[ega1_ofs + src_offset]];
        d[x+3] = tab[1+2*src[ega1_ofs + src_offset]];
        d[x+4] = tab[0+2*src[ega2_ofs + src_offset]];
        d[x+5] = tab[1+2*src[ega2_ofs + src_offset]];
        d[x+6] = tab[0+2*src[ega3_ofs + src_offset]];
        d[x+7] = tab[1+2*src[ega3_ofs + src_offset]];
    }
}

static void line_atm0_32(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u32 *d = (u32 *)dst;
    for (unsigned x = 0; x < 320; x += 8, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0] = tab[0+2*src[ega0_ofs + src_offset]];
        d[x+1] = tab[1+2*src[ega0_ofs + src_offset]];
        d[x+2] = tab[0+2*src[ega1_ofs + src_offset]];
        d[x+3] = tab[1+2*src[ega1_ofs + src_offset]];
        d[x+4] = tab[0+2*src[ega2_ofs + src_offset]];
        d[x+5] = tab[1+2*src[ega2_ofs + src_offset]];
        d[x+6] = tab[0+2*src[ega3_ofs + src_offset]];
        d[x+7] = tab[1+2*src[ega3_ofs + src_offset]];
    }
}

static void line_atm0_32d(unsigned char *dst, unsigned char *src, unsigned *tab, int src_offset)
{
    u32 *d = (u32 *)dst;
    for (unsigned x = 0; x < 640; x += 16, ++src_offset)
    {
        src_offset &= 0x1FFF;
        d[x+0]  = d[x+1]  = tab[0+2*src[ega0_ofs + src_offset]];
        d[x+2]  = d[x+3]  = tab[1+2*src[ega0_ofs + src_offset]];
        d[x+4]  = d[x+5]  = tab[0+2*src[ega1_ofs + src_offset]];
        d[x+6]  = d[x+7]  = tab[1+2*src[ega1_ofs + src_offset]];
        d[x+8]  = d[x+9]  = tab[0+2*src[ega2_ofs + src_offset]];
        d[x+10] = d[x+11] = tab[1+2*src[ega2_ofs + src_offset]];
        d[x+12] = d[x+13] = tab[0+2*src[ega3_ofs + src_offset]];
        d[x+14] = d[x+15] = tab[1+2*src[ega3_ofs + src_offset]];
    }
}

void rend_atm0_small(unsigned char *dst, unsigned pitch, int y, int Offset)
{
    unsigned char *dst2 = dst + (temp.ox-320)*temp.obpp/16;
    if (temp.scy > 200)
        dst2 += (temp.scy-200)/2*pitch * ((temp.oy > temp.scy)?2:1);

    dst2 += y*pitch;
    switch(temp.obpp)
    {
    case 8:
        line_atm0_8(dst2, temp.base, t.p4bpp8[0], Offset);
        dst2 += pitch;
        break;
    case 16:
        line_atm0_16(dst2, temp.base, t.p4bpp16[0], Offset);
        dst2 += pitch;
        break;
    case 32:
        line_atm0_32(dst2, temp.base, t.p4bpp32[0], Offset);
        dst2 += pitch;
        break;
    }
}

// EGA 320x200, double
void rend_atm0(unsigned char *dst, unsigned pitch, int y, int Offset)
{
    unsigned char *dst2 = dst + (temp.ox-640)*temp.obpp/16;
    if (temp.scy > 200)
        dst2 += (temp.scy-200)/2*pitch * ((temp.oy > temp.scy)?2:1);

    if (conf.fast_sl)
    {
        dst2 += y*pitch;
        switch(temp.obpp)
        {
        case 8:
            line_atm0_8d(dst2, temp.base, t.p4bpp8[0], Offset);
            break;
        case 16:
            line_atm0_16d(dst2, temp.base, t.p4bpp16[0], Offset);
            break;
        case 32:
            line_atm0_32d(dst2, temp.base, t.p4bpp32[0], Offset);
            break;
        }
    }
    else
    {
        dst2 += 2*y*pitch;
        switch(temp.obpp)
        {
        case 8:
            line_atm0_8d(dst2, temp.base, t.p4bpp8[0], Offset);
            dst2 += pitch;
            line_atm0_8d(dst2, temp.base, t.p4bpp8[1], Offset);
            break;
        case 16:
            line_atm0_16d(dst2, temp.base, t.p4bpp16[0], Offset);
            dst2 += pitch;
            line_atm0_16d(dst2, temp.base, t.p4bpp16[1], Offset);
            break;
        case 32:
            line_atm0_32d(dst2, temp.base, t.p4bpp32[0], Offset);
            dst2 += pitch;
            line_atm0_32d(dst2, temp.base, t.p4bpp32[1], Offset);
            break;
        }
    }
}
