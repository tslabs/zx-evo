#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_ts1.h"



void line_ts1_32(u8 *dst, u8 *src)
{
   int src_offset = 0;
   
   for (unsigned x = 0; x < 720*4; x += 0x10)
   {
		
		u8 tmp = src[src_offset];
		
		u32 p = temp.tspal_32[(comp.ts.palsel.i.gpal << 4) | (tmp >> 4)];
		*(unsigned*)(dst+x+0x00) = p;
		*(unsigned*)(dst+x+0x04) = p;
		
		p = temp.tspal_32[(comp.ts.palsel.i.gpal << 4) | (tmp & 0x0F)];
		*(unsigned*)(dst+x+0x08) = p;
		*(unsigned*)(dst+x+0x0C) = p;
		
		src_offset += 1;
   }
}

void rend1_ts1(u8 *dst, unsigned pitch, int y)
{
   u8 *dst2 = dst + (temp.ox - 720) * temp.obpp / 16;
   // !!! temporary dirty hack
   int offs = 16;
   int mul = (temp.oy > temp.scy) ? 2 : 1;

   dst2 += pitch * offs * mul;

	u8 gfx_page = (comp.ts.vpage & 0xF8);
	u8 *gfx = RAM_BASE_M + PAGE * (gfx_page + ((y >> 6) & 7)) + ((y & 0x3F) << 8);


    if (conf.fast_sl) {
        dst2 += y * pitch;
        switch(temp.obpp)
        {
        case 8:
            break;
        case 16:
            break;
        case 32:
            line_ts1_32(dst2, gfx);
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
            line_ts1_32(dst2, gfx);
            dst2 += pitch;
            line_ts1_32(dst2, gfx);
            break;
        }
    }
}

void rend_ts1(u8 *dst, unsigned pitch)
{
	// for (int y=0; y<288; y++)
	for (int y=0; y<284; y++)
		rend1_ts1(dst, pitch, y);
}

