#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrend.h"
#include "dxr_ts.h"
#include "dxr_ts1.h"
#include "dxr_ts2.h"
#include "dxr_ts3.h"

typedef void (*TTsRendFunc)(u8 *dst, unsigned pitch);

static void rend_ts_frame_small(u8 *dst, unsigned pitch)
{
     // static const TTsRendFunc TsRendFunc[] =
     // { rend_tsframe8, rend_tsframe16, 0, rend_tsframe32 };

     // TsRendFunc[temp.obpp/8-1](dst, pitch);
}

static void rend_ts_frame(u8 *dst, unsigned pitch)
{
    // if (conf.fast_sl) {
        // switch(temp.obpp)
        // {
        // case 8:
            // rend_tsframe_8d1(dst, pitch);
            // break;
        // case 16:
            // rend_tsframe_16d1(dst, pitch);
            // break;
        // case 32:
            // rend_tsframe_32d1(dst, pitch);
            // break;
        // }
    // } else {
        // switch(temp.obpp)
        // {
        // case 8:
            // rend_tsframe_8d(dst, pitch);
            // break;
        // case 16:
            // rend_tsframe_16d(dst, pitch);
            // break;
        // case 32:
            // rend_tsframe_32d(dst, pitch);
            // break;
        // }
    // }

}

void rend_ts_small(u8 *dst, unsigned pitch)
{
   // if (temp.comp_pal_changed) {
      // pixel_tables();
      // temp.comp_pal_changed = 0;
   // }

    switch (comp.ts.vconf.i.vmode)
    {
		case 0:		// Sinclair
		{
			rend_small(dst, pitch);
			return;
		}
    }
}

void rend_ts(u8 *dst, unsigned pitch)
{
//	u8 *dst1 = dst;
	u32 *src = &vbuf[0];
	src += conf.framex * 2;
	src += conf.framey * VID_WIDTH * 2;
	for (u32 i=0; i<conf.frameysize; i++)
	{
		memcpy (dst, src, pitch); dst += pitch;
		memcpy (dst, src, pitch); dst += pitch;
		src += VID_WIDTH * 2;
	}
//	dst = dst1;
	
	return;

    switch (comp.ts.vconf.i.vmode)
    {
		case 0:		// Sinclair
		{
			rend_dbl(dst, pitch);
			return;
		}
		case 1:		// 16c
		{
			rend_ts1(dst, pitch);
			return;
		}
		case 2:		// 256c
		{
			rend_ts2(dst, pitch);
			return;
		}
		case 3:		// Text
		{
			rend_ts3(dst, pitch);
			return;
		}
    }
}
