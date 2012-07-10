#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_ts3.h"


void line_ts3_32(u8 *dst, u8 *src, u8 *font, int src_offset)
{
   for (unsigned x = 0; x < 640*4; x += 0x20)
   {
		src_offset &= 0x3FFF;
		
		u8 p = font[src[src_offset] << 3];	// pixels
		u8 a = src[src_offset + 128];		// attributes
		
		u32 p0 = temp.tspal_32[(comp.ts.palsel.i.gpal << 4) | ((a >> 4) & 0x0F)];	// color for 'PAPER'
		u32 p1 = temp.tspal_32[(comp.ts.palsel.i.gpal << 4) | (a & 0x0F)];			// color for 'INK'
		
		*(unsigned*)(dst+x+0x00) = ((p << 1) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x04) = ((p << 2) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x08) = ((p << 3) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x0C) = ((p << 4) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x10) = ((p << 5) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x14) = ((p << 6) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x18) = ((p << 7) & 0x100) ? p1 : p0;
		*(unsigned*)(dst+x+0x1C) = ((p << 8) & 0x100) ? p1 : p0;
		
		src_offset += 1;
   }
}

void rend1_ts3(u8 *dst, unsigned pitch, int y)
{
   u8 *dst2 = dst + (temp.ox - 640) * temp.obpp / 16;
   // !!! temporary dirty hack
   // int offs = (temp.scy == 300) ? 60 : (temp.scy == 240) ? 20 : 0;
   int offs = (temp.scy == 300) ? 40 : (temp.scy == 240) ? 0 : 0;
   int mul = (temp.oy > temp.scy) ? 2 : 1;

   if (temp.scy > 200)
       dst2 += pitch * offs * mul;

	int src_offset = ((y >> 3) & 0x1F) << 8;
	u8 text_page = comp.ts.vpage;
	u8 font_page = comp.ts.vpage ^ 0x01;
	u8 *font = RAM_BASE_M + PAGE * font_page + (y & 7);

    if (conf.fast_sl) {
        dst2 += y * pitch;
        switch(temp.obpp)
        {
        case 8:
            break;
        case 16:
            break;
        case 32:
            line_ts3_32(dst2, temp.base, font, src_offset);
            break;
        }
    }

	else
	{
        dst2 += 2*y*pitch;
        switch(temp.obpp)
        {
        case 8:
            break;
        case 16:
            break;
        case 32:
            line_ts3_32(dst2, temp.base, font, src_offset);
            dst2 += pitch;
            line_ts3_32(dst2, temp.base, font, src_offset);
            break;
        }
    }
}

void rend_ts3(u8 *dst, unsigned pitch)
{
	// for (int y=0; y<200; y++)
	for (int y=0; y<240; y++)
		rend1_ts3(dst, pitch, y);
}

