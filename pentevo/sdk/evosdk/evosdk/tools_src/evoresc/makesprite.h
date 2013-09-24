const int sprhgt=16;
const int sprwid=16;

int sprnr,curaddr,spraddr,sprpg;
unsigned char sprite[16][16];
unsigned char tspraddr[16384];
unsigned char foutbuf[16385];
int foutbufpos;
unsigned char *mempos;


inline void foutflush(void)
{
	int off;

	//таблица кусками по 256 спрайтов

	off=256+(sprnr&255)+(sprnr>>8)*768;

	tspraddr[off+0  ]=spraddr;
	tspraddr[off+256]=spraddr>>8;
	tspraddr[off+512]=sprpg^0xff; //pg

	memcpy(mempos,foutbuf,foutbufpos);
	mempos+=foutbufpos;

	foutbufpos=0;
}



inline void wrbyte(unsigned char b)
{
	//BlockWrite(fout,b,1);
	foutbuf[foutbufpos++]=b;
	++curaddr;
}



inline bool sprisempty(void)
{
	int i,j;

	for(i=0;i<16;++i)
	{
		for(j=0;j<16;++j)
		{
			if(sprite[i][j]<16) return false;//transparent_color
		}
	}

	return true;
}



inline bool colisempty(unsigned char col1,unsigned char col2)
{
	return (col1>=16)&&(col2>=16); //transparent_color
}



inline unsigned char coltopixel(unsigned char col1,unsigned char col2)
{
	//transparent_color and $0f = 0!!!
	return ((col1&0x08)<<3) + (col1&0x07) + ((col2&0x08)<<4) + ((col2&0x07)<<3);
}



inline unsigned char coltomask(unsigned char col1,unsigned char col2)
{
	unsigned char mask;

	mask=0;

	if(col1>=16) mask =0x47;
	if(col2>=16) mask|=0xb8;

	return mask;
}



void mksprite(void)
{
	int x,y;
	unsigned char mask,pixel;
	int curscraddr,oldscraddr,scraddrdelta,oldde;

	x=6;

	oldde=65536; //false value

	while(x>=0) //4 layers
	{
		//start of the layer
		wrbyte(225); //pop hl
		oldscraddr=0;
		curscraddr=0;

		while(x<sprwid) //layer columns
		{
			for(y=0;y<16;++y)
			{
				scraddrdelta=curscraddr-oldscraddr;

				if(!colisempty(sprite[y][x],sprite[y][x+1]))
				{
					if(scraddrdelta)
					{
						if(scraddrdelta!=40)
						{
							if(scraddrdelta!=oldde)
							{
								if((oldde!=65536) && ((scraddrdelta&0xff00)==(oldde&0xff00)))
								{//ld e,N
									wrbyte(30); //ld e,N
									wrbyte(scraddrdelta);
								}
								else
								{//ld de,N
									wrbyte(17); //ld de,N
									wrbyte(scraddrdelta);
									wrbyte(scraddrdelta>>8);
								}

								oldde=scraddrdelta;
							}

							wrbyte(25); //add hl,de
						}
						else
						{//scraddrdelta=40
							wrbyte(9); //add hl,bc
						}
					}

					oldscraddr=curscraddr;
					mask =coltomask (sprite[y][x],sprite[y][x+1]);
					pixel=coltopixel(sprite[y][x],sprite[y][x+1]);

					if(!mask)
					{
						if(!pixel)
						{
							wrbyte(112); //ld (hl),b
						}
						else
						{
							wrbyte(54); //ld (hl),N
							wrbyte(pixel);
						}
					}
					else
					{//mask<>0
						wrbyte(126); //ld a,(hl)
						wrbyte(230); //and N
						wrbyte(mask);

						if(pixel)
						{
							wrbyte(246); //or N
							wrbyte(pixel);
						}

						wrbyte(119); //ld (hl),a
					}

				} //nonempty

				curscraddr+=40;
			} //y

			curscraddr=curscraddr-(40*sprhgt)+1;
			x+=8;
		} //x in layer

		x=x-sprwid-2;
	} //layers

	wrbyte(0xfd); //$fd
	wrbyte(233); //jp (iy)
}



void mkspr_init(int page)
{
	int i,off;

	sprnr=0;
	curaddr=0; //todo +$0200?
	sprpg=page; //todo $10?
	foutbufpos=0;
	mempos=mem;

	memset(tspraddr,0,sizeof(tspraddr));

	for(i=0;i<256*21;++i)
	{
		off=256+(i&255)+(i>>8)*768;
		tspraddr[off+0  ]=0;
		tspraddr[off+256]=0;    //todo $02?
		tspraddr[off+512]=0xeb; //pg
	}
}


int mkspr_add(const char *filename)
{
	FILE *fin;
	int i,j,x,y,pp,size,bshift,wdt,hgt,bpp,rle;
	unsigned char *data;

	fin=fopen(filename,"rb");//256c 256x256

	if(!fin)
	{
		printf("ERR: Can't open sprite sheet BMP (%s)\n",filename);
		return -1;
	}

	fseek(fin,0,SEEK_END);
	size=ftell(fin);
	fseek(fin,0,SEEK_SET);
	data=(unsigned char*)malloc(size);
	fread(data,size,1,fin);
	fclose(fin);

	bshift=read_dword(&data[10]);
	wdt=read_dword(&data[18]);
	hgt=read_dword(&data[22]);
	bpp=read_word (&data[28]);
	rle=read_dword(&data[30]);
	
	if((wdt&15)||(hgt&15))
	{
		printf("ERR: Width and height should be 16px aligned (%s)\n",filename);
		free(data);
		return -1;
	}

	if(rle!=0||bpp!=8)
	{
		printf("ERR: Sprite sheet should be 256 colors uncompressed BMP (%s)\n",filename);
		free(data);
		return -1;
	}

	/* Code to generate:
	dup 4
	pop hl
	dup ?
	dup ?
	[ld de,N:add hl,de]/[add hl,bc]
	ld (hl),N / ld a,(hl):and N:or N:ld (hl),a
	edup ;bytes
	edup ;columns
	edup ;layers
	jp (iy)
	*/

	for(y=0;y<hgt;y+=16)
	{
		for(x=0;x<wdt;x+=16)
		{
			if(sprnr>=5376) break;//округлённое (16384/768) *256

			//копирование спрайта из изображения в буфер

			for(i=0;i<16;++i)
			{
				pp=bshift+((hgt-1-(y+i))*wdt)+x;

				for(j=0;j<16;++j)
				{
					sprite[i][j]=data[pp++];
				}
			}

			//sprite 16 x 16 (sprwid x sprhgt)

			spraddr=curaddr;

			mksprite();

			if(curaddr<16384)
			{
				foutflush();
			}
			else
			{//page overflow
				page_save(0,sprpg);
				mempos=mem;

				++sprpg;
				curaddr=0;//-=spraddr; //todo +$0200?
				spraddr=0;

				foutbufpos=0;

				mksprite();
				foutflush();
			}

			++sprnr;
		}
	}

	free(data);

	return sprpg;
}
