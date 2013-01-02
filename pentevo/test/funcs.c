
// ASM functions prototypes
void drawc(u8);
void ram_fill_p(u8);
void ram_fill_inc(void);
u16 ram_check_p(u8);
u16 ram_check_inc(void);


// Screen functions
#define color(a) cc = (u8)(a)
#define xy(x, y) cx = (x); cy = (y)
#define border(a) output8(0xFE,(a))

void cls()
{
	memset(vm, 0, 6144);
	memset(vm + 6144, 0, 768);
}

void putc(u8 c)
{
	if((c == '\r') || (c == '\n'))
	{
		xy(0, cy + 1);
		return;
	}
	drawc(c);
	xy(cx + 6, cy);
}

void msg(u8 *adr)
{
	u8 c;
	while(c = *adr++)
		putc(c);
}

void hex(u8 dig)
{
	putc((dig > 9) ? (dig - 10 + 'A') : (dig + '0'));
}

void hex8(u8 num)
{
	hex((num >> 4) & 0xF);
	hex(num & 0xF);
}

void hex16(u16 num)
{
	hex((num >> 12) & 0xF);
	hex((num >> 8) & 0xF);
	hex((num >> 4) & 0xF);
	hex(num & 0xF);
}

// Key functions
u8 getkey()
{
	union
	{
		u16 w;
		struct
		{
			u8 l;
			u8 h;
		} b;
	} row;
	u8 inp, inpm;
	s8 key, keyc;
	u8 i, j;

	while(~input(0xFE) & 0x1F);		// wait for ALL keys to release

	while(1)
	{
		row.w = 0xFEFE;
		key = keyc = -1;
		for (i=0; i<8; i++)
		{
			inp = ~input(row.w);
			for (j=0, inpm = 1; j<5; j++, inpm <<= 1)
			{
				key++;
				if (inp & inpm)
				{
					keyc = key; break;
				}
			}
			if (keyc != -1)
				return keyc;
			row.b.h = (row.b.h << 1) | 1;
		}
	}
}

u8 key_disp()
{
	u8 key;

	while(1)
	{
		key = getkey();
		switch(key)
		{
			case K_1: { mode = 1; return 1; }
			case K_2: { mode = 2; return 1; }
			case K_3: { mode = 0; return 1; }
			case K_0: { z80_lp ^= 1; return 0; }
		}
	}
}

// H/W functions

	// variables
	u8 page0;
	u8 page1;
	u8 page2;
	u8 page3;
	u8 tsconf;

	// inline functions
#define page0(a)	page0 = (a); output(PAGE0,(a))
#define page1(a)	page1 = (a); output(PAGE1,(a))
#define page2(a)	page2 = (a); output(PAGE2,(a))
#define page3(a)	page3 = (a); output(PAGE3,(a))
#define tscfg(a)	output(TSCONF,(a))
#define fmaps(a)	output(FMADDR, ((a)>>12) | FM_EN)
#define fmaps_off()	output(FMADDR, 0)

#define dma_busy	(input(DSTATUS) & DMA_BSY)
#define dma_mem()	output(DMACTR, DMA_DEV_MEM);
#define dma_set(saddr, spage, daddr, dpage, len, num)	\
					output(DMASAL, saddr & 0xFF);		\
					output(DMASAH, saddr >> 8);			\
					output(DMASAX, spage);				\
					output(DMADAL, daddr & 0xFF);		\
					output(DMADAH, daddr >> 8);			\
					output(DMADAX, dpage);				\
					output(DMALEN, len);				\
					output(DMANUM, num);

	// functions
void sfile_null(u16 addr)
{
	fmaps(addr);
	memset((u8*)addr + SFILE, 0, 512);
	fmaps_off();
}
