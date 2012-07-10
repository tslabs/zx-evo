#include "std.h"
#include "emul.h"
#include "vars.h"
#include "tsconf.h"

const u8 pwm[] = { 0,10,21,31,42,53,63,74,85,95,106,117,127,138,149,159,170,181,191,202,213,223,234,245,255 };

// convert CRAM data to precalculated renderer tables
void update_tspal(u8 addr)
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

	temp.tspal_32[addr] = (r << 16) | (g <<8) | b;
}

// DMA procedures
void dma (u8 val)
{
	DMACTRL_t ctrl;
	ctrl.b = val;

	u8 tmp;
	int i, j;
	u16 *s, *d;
	u32 ss, dd;

	for (i=0; i<(comp.ts.dmanum + 1); i++)
	{
		ss = comp.ts.dmasaddr.w;
		dd = comp.ts.dmadaddr.w;

		switch (ctrl.i.dev)
		{
			// RAM to RAM
			case DMA_RAM:
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
				s = (u16*)(ss + RAM_BASE_M);
				d = (u16*)(dd + RAM_BASE_M);
				*d = *s;
				ss = (ss + 2) & 0x3FFFFF;
				dd = (dd + 2) & 0x3FFFFF;
				}
				break;
			}
			case DMA_CRAM:
			// RAM to CRAM (palette)
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
					s = (u16*)(ss + RAM_BASE_M);
					tmp = (dd >> 1) & 0xFF;
					comp.ts.cram[tmp] = *s;
					update_tspal(tmp);
					ss = (ss + 2) & 0x3FFFFF;
					dd = (dd + 2) & 0x3FFFFF;
				}
				break;
			}
		}

		if (ctrl.i.s_algn)
			comp.ts.dmasaddr.w += ctrl.i.asz ? 512 : 256;
		else
			comp.ts.dmasaddr.w = ss;

		if (ctrl.i.d_algn)
			comp.ts.dmadaddr.w += ctrl.i.asz ? 512 : 256;
		else
			comp.ts.dmadaddr.w = dd;
	}
}
