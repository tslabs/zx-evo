
#define color(a) cc = (u8)(a)
#define border(a) output8(0xFE,(a))
#define page3(a) output(0x13AF,(a))
#define xy(x, y) cx = (x); cy = (y)

u8 ram_check_0(void);
u8 ram_check_inc(void);
void dma_copy(u8);

// Screen functions
void cls()
{
	memset(vm, 0, 6144);
	memset(vm + 6144, 0, 768);
}

void fade()
{
	memset(vm + 6144, 1, 768);
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

int putchar(int c)
{
    putc(c);
    return c;
}

void msg(u8 *adr)
{
	u8 c;
	while(c = *adr++)
		putc(c);
}

void frame(u8 xx, u8 yy, u8 sx, u8 sy, u8 cf)
{
	s8 i, j;
	xy(xx, yy);
	
	color(cf);
	
	putc(201);
	for (i=0; i<sx; i++)
		putc(205);
	putc(187);
	xy(xx, cy + 1);
	
	for(j=0; j<sy; j++)
	{
		putc(186);
		for (i=0; i<sx; i++)
			putc(32);
		putc(186);
		xy(xx, cy + 1);
	}
	
	putc(200);
	for (i=0; i<sx; i++)
		putc(205);
	putc(188);
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
