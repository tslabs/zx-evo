#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "tsconf.h"

extern VCTR vid;

const u8 pwm[] = { 0,10,21,31,42,53,63,74,85,95,106,117,127,138,149,159,170,181,191,202,213,223,234,245,255 };

// convert CRAM data to precalculated renderer tables
void update_clut(u8 addr)
{
	u16 t = comp.ts.cram[addr];
	u8 r = (t >> 10) & 0x1F;
	u8 g = (t >> 5) & 0x1F;
	u8 b = t & 0x1F;

	// video PWM correction
	r = (r > 24) ? 24 : r;
	g = (g > 24) ? 24 : g;
	b = (b > 24) ? 24 : b;

	// coerce to TrueÚ Color values
	r = pwm[r];
	g = pwm[g];
	b = pwm[b];

	vid.clut[addr] = (r << 16) | (g <<8) | b;
}

// DMA procedures
void dma (u8 val)
{
	DMACTRL_t ctrl;
	ctrl.ctrl = val;

	u8 tmp;
	int i, j;
	u16 *s, *d;
	u32 ss, dd;
	u32 m1 = ctrl.asz ? 0x3FFE00 : 0x3FFF00;
	u32 m2 = ctrl.asz ? 0x1FF : 0xFF;

	for (i=0; i<(comp.ts.dmanum + 1); i++)
	{
		ss = comp.ts.dmasaddr;
		dd = comp.ts.dmadaddr;

		switch (ctrl.dev)
		{
			// RAM to RAM
			case DMA_RAM:
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
				s = (u16*)(ss + RAM_BASE_M);
				d = (u16*)(dd + RAM_BASE_M);
				*d = *s;
				ss_inc; dd_inc;
				}
				break;
			}
			// RAM to CRAM (palette)
			case DMA_CRAM:
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
					s = (u16*)(ss + RAM_BASE_M);
					tmp = (dd >> 1) & 0xFF;
					comp.ts.cram[tmp] = *s;
					update_clut(tmp);
					ss_inc; dd_inc;
				}
				break;
			}
		}

		if (ctrl.s_algn)
			comp.ts.dmasaddr += ctrl.asz ? 512 : 256;
		else
			comp.ts.dmasaddr = ss;

		if (ctrl.d_algn)
			comp.ts.dmadaddr += ctrl.asz ? 512 : 256;
		else
			comp.ts.dmadaddr = dd;
	}
}
