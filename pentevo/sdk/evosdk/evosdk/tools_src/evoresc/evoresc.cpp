#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


struct {
	unsigned char* data;
	int offset;
	int bpp;
	int wdt,hgt;
	bool flip;
} BMP;

unsigned char mem[65536];

FILE *fileBatMLZ;
FILE *fileBatSCL;

unsigned char *tileData;

int fileCnt;

struct {
	int tile;
	int wdt,hgt;
} IMGLIST[256];

#define STARTUP_ADR	0xe000
#define STACK_SIZE	0x0400

#define MUS_COUNT	0x49fe
#define SMP_COUNT	0x49ff
#define MUSLIST_ADR	0x4a00	//3*256
#define SMPLIST_ADR	0x4d00	//4*256
#define SFX_ADR		0x5100

#define PAL_ADR		0x0000
#define IMGLIST_ADR	0x1000

#define TSPR_ADR	0xfa00

//фиксированные номера страниц

#define SPRBUF_PAGE	8	//4 страницы
#define SPRTBL_PAGE	6	//параметры спрайтов

#define CC_PAGE0	12	//код
#define CC_PAGE1	13
#define CC_PAGE2	14
#define CC_PAGE3	15

#define SND_PAGE	0	//код звука и эффекты
#define PAL_PAGE	4	//палитры

#define GFX_PAGE	16	//графика начинается с фиксированной страницы

#define PACK_PIXELS(col1,col2) (((col1)&7)|(((col1)&8)<<3)|(((col2)&7)<<3)|(((col2)&8)<<4))



void clear_mem(void)
{
	memset(mem,0,65536);
}



int get_hex(char n)
{
	if(n>='0'&&n<='9') return n-'0';
	if(n>='a'&&n<='f') return n-'a'+10;
	if(n>='A'&&n<='F') return n-'A'+10;

	return -1;
}



int get_hex_byte(char *str)
{
	return (get_hex(str[0])<<4)+get_hex(str[1]);
}



//чтение Intel HEX файла в память, без всяких проверок

int load_ihx(const char* name)
{
	FILE *file;
	char line[1024];
	int i,off,ptr,len,size;

	file=fopen(name,"rt");

	if(!file) return 0;

	size=0;

	while(fgets(line,sizeof(line),file)!=NULL)
	{
		len=get_hex_byte(&line[1]);
		ptr=(get_hex_byte(&line[3])<<8)+get_hex_byte(&line[5]);
		off=9;

		for(i=0;i<len;i++)
		{
			mem[ptr++]=get_hex_byte(&line[off]);
			off+=2;
		}
		
		if(ptr>size) size=ptr;
	}

	fclose(file);

	return size;
}



bool load_bin(int adr,const char* name)
{
	FILE *file;
	int size;

	file=fopen(name,"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	if(adr+size>0x10000)
	{
		fclose(file);
		return false;
	}

	fread(mem+adr,size,1,file);
	fclose(file);

	return true;
}



//проверка страницы на наличие ненулевых данных

bool page_is_empty(int page)
{
	int i;

	page<<=14;

	for(i=0;i<16384;i++)
	{
		if(mem[i+page]) return false;
	}

	return true;
}



bool page_save(int slot,int page)
{
	char name[1024];
	FILE *file;

	fprintf(fileBatMLZ,"megalz page_%i.bin >nul\n",page);
	fprintf(fileBatMLZ,"call _getsize.bat page_%i.bin.mlz %i\n",page,page);
	fprintf(fileBatSCL,"trdtool + disk.scl page_%i.bin.mlz\n",page);

	sprintf(name,"_temp_/page_%i.bin",page);

	file=fopen(name,"wb");

	if(!file) return false;

	fwrite(&mem[slot<<14],16384,1,file);
	fclose(file);

	fileCnt++;

	return true;
}



void error(void)
{
	fclose(fileBatMLZ);
	fclose(fileBatSCL);

	if(tileData) free(tileData);
}



bool load_palette(const char* filename,unsigned char* dst)
{
	FILE *file;
	unsigned char bmp[256];
	int i,pp,size,bpp,r,g,b;

	file=fopen(filename,"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	if(size>256) size=256;
	fseek(file,0,SEEK_SET);
	fread(bmp,size,1,file);
	fclose(file);
	
	if(bmp[0]!='B'||bmp[1]!='M')
	{
		printf("ERR: Not BMP file format (%s)\n",filename);
		return false;
	}
	
	bpp=bmp[28]+(bmp[29]<<8);
	
	if(bpp!=4&&bpp!=8)
	{
		printf("ERR: Only uncompressed 16 and 256 colors BMP supported (%s)\n",filename);
		return false;
	}

	pp=54;

	for(i=0;i<16;i++)
	{
		b=bmp[pp++]>>6;
		g=bmp[pp++]>>6;
		r=bmp[pp++]>>6;

		dst[i]=(b<<4)|(g<<2)|r;
		pp++;
	}

	return true;
}



inline void convert_tile(unsigned char* tile,int tx,int ty)
{
	int i,j,pp,pd;
	unsigned char chr[8][8];
	
	switch(BMP.bpp)
	{
	case 4:
		{
			pd=BMP.flip?BMP.offset+(ty<<3)*(BMP.wdt>>1)+(tx<<2):BMP.offset+(BMP.hgt-1-(ty<<3))*(BMP.wdt>>1)+(tx<<2);
			
			for(i=0;i<8;++i)
			{
				pp=pd;

				for(j=0;j<8;j+=2)
				{
					chr[i][j+0]=BMP.data[pp]>>4;
					chr[i][j+1]=BMP.data[pp]&15;
					++pp;
				}

				pd+=BMP.flip?(BMP.wdt>>1):0-(BMP.wdt>>1);
			}
		}
		break;

	case 8:
		{
			pd=BMP.flip?BMP.offset+(ty<<3)*BMP.wdt+(tx<<3):BMP.offset+(BMP.hgt-1-(ty<<3))*BMP.wdt+(tx<<3);
			
			for(i=0;i<8;++i)
			{
				pp=pd;

				for(j=0;j<8;++j)
				{
					chr[i][j]=BMP.data[pp++]&15;
				}

				pd+=BMP.flip?BMP.wdt:0-BMP.wdt;
			}
		}
		break;
	}

	pp=0;

	for(i=0;i<8;++i)
	{
		tile[pp+0]=PACK_PIXELS(chr[i][0],chr[i][1]);
		tile[pp+3]=PACK_PIXELS(chr[i][2],chr[i][3]);
		tile[pp+1]=PACK_PIXELS(chr[i][4],chr[i][5]);
		tile[pp+2]=PACK_PIXELS(chr[i][6],chr[i][7]);
		pp+=4;
	}
}



unsigned int read_dword(unsigned char* data)
{
	return data[0]+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}



unsigned int read_word(unsigned char* data)
{
	return data[0]+(data[1]<<8);
}



unsigned char* load_graphics(const char* filename,int &tilecnt,int &tsizex,int &tsizey)
{
	FILE *file;
	int i,j,ptr,rle,size,tssize;
	unsigned char* tileset;

	file=fopen(filename,"rb");

	if(!file) return NULL;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);
	BMP.data=(unsigned char*)malloc(size);
	fread(BMP.data,size,1,file);
	fclose(file);
	
	if(BMP.data[0]!='B'||BMP.data[1]!='M')
	{
		printf("ERR: Not BMP file format (%s)\n",filename);
		return NULL;
	}
	
	BMP.offset=read_dword(&BMP.data[10]);
	BMP.wdt=read_dword(&BMP.data[18]);
	BMP.hgt=read_dword(&BMP.data[22]);
	BMP.bpp=read_word (&BMP.data[28]);
	rle=read_dword(&BMP.data[30]);
	
	if((BMP.wdt&7)||(BMP.hgt&7))
	{
		printf("ERR: Width and height should be 8px aligned (%s)\n",filename);
		return NULL;
	}

	if(rle!=0||(BMP.bpp!=4&&BMP.bpp!=8))
	{
		printf("ERR: Only uncompressed 16 and 256 colors BMP supported (%s)\n",filename);
		return NULL;
	}

	tsizex=BMP.wdt>>3;
	tsizey=BMP.hgt>>3;
	
	BMP.flip=(BMP.hgt>0)?false:true;
	if(BMP.flip) BMP.hgt=0-BMP.hgt;
	
	tilecnt=(BMP.wdt>>3)*(BMP.hgt>>3);
	tssize=tilecnt<<5;
	tileset=(unsigned char*)malloc(tssize);
	
	ptr=0;
	
	for(i=0;i<tsizey;i++)
	{
		for(j=0;j<tsizex;j++)
		{
			convert_tile(&tileset[ptr],j,i);
			ptr+=32;
		}
	}

	return tileset;
}



void trim_str(char* line)
{
	unsigned int i;

	for(i=0;i<strlen(line);i++)
	{
		if(line[i]<0x20)
		{
			line[i]=0;
			break;
		}
	}
}



#include "makesprite.h"
#include "makesample.h"



int main(int argc,char* argv[])
{
	const char cc_page[4]={CC_PAGE0,CC_PAGE1,CC_PAGE2,CC_PAGE3};
	FILE *list,*file;
	char line[1024];
	unsigned char *data;
	int i,pp,off,ptr,size,page,img_count;
	int mus_offset[256],mus_page[256];
	int smp_offset[256],smp_page[256],smp_pitch[256];
	int mus_count,mus_page_start,mus_pages,gfx_pages;
	int code_size,code_pages;
	int spr_page_start,spr_pages;
	int smp_page_start,smp_pages,smp_count;
	int tile_ptr,tile_cnt;
	int wdt,hgt,cnt,pitch;

	if(argc<7)
	{
		printf("EVOSDK Resource Compiler by Shiru and Alone Coder 03'12\n");
		printf("Error: Some parameters are missing\n");
		return 1;
	}

	tileData=NULL;
	fileCnt=0;

	fileBatMLZ=fopen("_temp_/compress.bat","wt");
	fileBatSCL=fopen("_temp_/createscl.bat","wt");

	fprintf(fileBatMLZ,"PATH=..\\..\\evosdk\n");
	fprintf(fileBatMLZ,"echo messageStr > filelist.asm\n");
	fprintf(fileBatMLZ,"echo \tdb %%title%%,0 >> filelist.asm\n");
	fprintf(fileBatMLZ,"echo fileList >> filelist.asm\n");

	fprintf(fileBatSCL,"trdtool # disk.scl\n");
	fprintf(fileBatSCL,"trdtool + disk.scl boot.$b\n");

	atexit(error);

	//графика изображений

	clear_mem();

	list=fopen(argv[6],"rt");

	if(!list)
	{
		printf("Error: Image list is missing\n");
		exit(1);
	}

	page=GFX_PAGE;
	gfx_pages=0;
	ptr=0;
	img_count=0;
	tile_ptr=0;

	while(fgets(line,sizeof(line),list)!=NULL)
	{
		if(img_count==256)
		{
			printf("ERR: Too many images\n");
			exit(1);
		}

		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		trim_str(line);

		tileData=load_graphics(line,tile_cnt,wdt,hgt);

		if(!tileData)
		{
			printf("ERR: Can't open image (%s)\n",line);
			exit(1);
		}

		IMGLIST[img_count].tile=tile_ptr;
		IMGLIST[img_count].wdt=wdt;
		IMGLIST[img_count].hgt=hgt;
		img_count++;
		tile_ptr+=tile_cnt;

		off=0;

		for(i=0;i<tile_cnt;i++)
		{
			memcpy(&mem[ptr],&tileData[off],32);
			ptr+=32;
			off+=32;

			if(ptr>=16384)
			{
				page_save(0,page);
				ptr=0;
				page++;
				gfx_pages++;
			}
		}

		free(tileData);

		off+=16;
	}

	if(ptr)
	{
		page_save(0,page);
		gfx_pages++;
	}

	fclose(list);

	mus_page_start=off?page+1:page;

	//палитры и параметры изображений

	clear_mem();

	list=fopen(argv[5],"rt");

	if(!list)
	{
		printf("Error: Palette list is missing\n");
		exit(1);
	}

	off=PAL_ADR;
	cnt=0;

	while(fgets(line,sizeof(line),list)!=NULL)
	{
		if(cnt==256)
		{
			printf("ERR: Too many palettes\n");
			exit(1);
		}

		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		trim_str(line);

		if(!load_palette(line,&mem[off]))
		{
			printf("ERR: Can't load palette from image (%s)\n",line);
			exit(1);
		}

		off+=16;
		cnt++;
	}

	fclose(list);

	off=IMGLIST_ADR;

	for(i=0;i<img_count;i++)
	{
		mem[off++]=IMGLIST[i].tile&255;
		mem[off++]=IMGLIST[i].tile>>8;
		mem[off++]=IMGLIST[i].wdt;
		mem[off++]=IMGLIST[i].hgt;
	}

	page_save(0,PAL_PAGE);

	//данные музыки

	clear_mem();

	list=fopen(argv[4],"rt");

	if(!list)
	{
		printf("Error: Music list is missing\n");
		exit(1);
	}

	off=0;
	page=mus_page_start;
	mus_count=0;
	mus_pages=0;

	while(fgets(line,sizeof(line),list)!=NULL)
	{
		if(mus_count==256)
		{
			printf("ERR: Too many music files\n");
			exit(1);
		}

		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		trim_str(line);

		file=fopen(line,"rb");

		if(!file)
		{
			printf("Error: Can't open file from music list (%s)\n",line);
			fclose(list);
			exit(1);
		}

		fseek(file,0,SEEK_END);
		size=ftell(file);
		fseek(file,0,SEEK_SET);

		if(size>16384)
		{
			printf("Error: Music file is too large, 16384 bytes max (%s)\n",line);
			fclose(list);
			exit(1);
		}

		data=(unsigned char*)malloc(size);
		fread(data,size,1,file);
		fclose(file);

		if(off+size>16384)
		{
			page_save(0,page);
			clear_mem();
			off=0;
			++page;
			++mus_pages;
		}

		mus_offset[mus_count]=0x8000+off;
		mus_page[mus_count]=page;
		++mus_count;

		memcpy(&mem[off],data,size);
		off+=size;

		free(data);
	}

	if(mus_count&&off)
	{
		page_save(0,page);
		++page;
		++mus_pages;
	}

	fclose(list);

	smp_page_start=page;

	//данные сэмплов

	clear_mem();

	list=fopen(argv[7],"rt");

	if(!list)
	{
		printf("Error: Sample list is missing\n");
		exit(1);
	}

	off=0;
	page=smp_page_start;
	smp_count=0;
	smp_pages=0;

	while(fgets(line,sizeof(line),list)!=NULL)
	{
		if(smp_count==256)
		{
			printf("ERR: Too many samples\n");
			exit(1);
		}

		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		trim_str(line);

		data=load_wav(line,size,pitch);

		if(!data)
		{
			printf("Error: Can't open file from sample list (%s)\n",line);
			fclose(list);
			exit(1);
		}

		smp_offset[smp_count]=off;
		smp_page  [smp_count]=page^255;
		smp_pitch [smp_count]=pitch;

		smp_count++;
		pp=0;

		while(pp<size)
		{
			mem[off++]=data[pp++];

			if(off>=16384)
			{
				page_save(0,page);
				clear_mem();
				off=0;
				page++;
				smp_pages++;
			}
		}

		free(data);
	}

	if(smp_count&&off)
	{
		page_save(0,page);
		page++;
		smp_pages++;
	}

	fclose(list);

	spr_page_start=page;

	//плееры музыки и звука, данные звуков, списки сэмплов и треков

	clear_mem();

	if(!load_bin(0x4000,"../evosdk/sound.bin"))
	{
		printf("Error: sound.bin not found\n");
		exit(1);
	}

	if(strcmp(argv[3],"")) load_bin(SFX_ADR,argv[3]);

	mem[MUS_COUNT]=mus_count;
	off=MUSLIST_ADR;

	for(i=0;i<mus_count;i++)
	{
		mem[off+0  ]=mus_offset[i]&255;
		mem[off+256]=mus_offset[i]>>8;
		mem[off+512]=mus_page  [i];
		++off;
	}

	mem[SMP_COUNT]=smp_count;
	off=SMPLIST_ADR;

	for(i=0;i<smp_count;i++)
	{
		mem[off+0  ]=smp_offset[i]&255;
		mem[off+256]=smp_offset[i]>>8;
		mem[off+512]=smp_page  [i];
		mem[off+768]=smp_pitch [i];
		++off;
	}

	page_save(1,SND_PAGE);

	//спрайты

	list=fopen(argv[8],"rt");

	if(!list)
	{
		printf("Error: Sprite list is missing\n");
		exit(1);
	}

	mkspr_init(spr_page_start);

	spr_pages=0;

	while(fgets(line,sizeof(line),list)!=NULL)
	{
		if(cnt==256)
		{
			printf("ERR: Too many sprites\n");
			exit(1);
		}

		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		trim_str(line);

		i=mkspr_add(line);

		if(i<0) exit(1);

		spr_pages=i-spr_page_start+1;
	}

	if(mempos!=mem) page_save(0,sprpg);

	//параметры спрайтов

	if(spr_pages)
	{
		memcpy(mem,tspraddr,16384);
		page_save(0,SPRTBL_PAGE);
	}

	//код программы

	clear_mem();

	size=load_ihx(argv[1]);
	
	if(!size)
	{
		printf("Error: Can't load Intel HEX from file\n");
		exit(1);
	}

	code_size=size;

	if(code_size>=STARTUP_ADR-STACK_SIZE)
	{
		printf("Error: Out of memory, compiled code is too large\n");
		exit(1);
	}

	if(!load_bin(STARTUP_ADR,argv[2]))
	{
		printf("Error: Can't load startup code\n");
		exit(1);
	}

	code_pages=0;

	for(i=0;i<4;i++)
	{
		if(!page_is_empty(i))
		{
			page_save(i,cc_page[i]);
			code_pages++;
		}
	}

	//завершение вывода данных для загрузчика

	fprintf(fileBatMLZ,"echo \tdb 0,%i >>filelist.asm\n",CC_PAGE3);
	fprintf(fileBatMLZ,"echo \tdw %i >>filelist.asm\n",STARTUP_ADR);
	fprintf(fileBatMLZ,"echo progressStep >>filelist.asm\n");
	fprintf(fileBatMLZ,"echo \tdw %i >> filelist.asm\n",32*256/fileCnt/2);

	//информация об использовании памяти

	if(spr_page_start+spr_pages>=192)
	{
		printf("ERR: Too many RAM pages used (192)\n");
		exit(1);
	}

	page=4/*code_pages*/+4/*sprbuf*/+mus_pages+smp_pages+gfx_pages+spr_pages+1+1+(spr_pages?1:0);//snd, pal, sprtbl

	printf("\nCompiled code size %i bytes (%i max, %i left)\n\n",code_size,STARTUP_ADR-STACK_SIZE,STARTUP_ADR-STACK_SIZE-code_size);

	printf("%i RAM pages (%iK) used:\n",page,page*16);
	printf("Code:\t\t\t%i,%i,%i,%i\n",CC_PAGE0,CC_PAGE1,CC_PAGE2,CC_PAGE3);
	printf("Sprites buffer:\t\t%i,%i,%i,%i\n",SPRBUF_PAGE,SPRBUF_PAGE+1,SPRBUF_PAGE+2,SPRBUF_PAGE+3);
	printf("Graphics data:\t\t");
	if(gfx_pages)
	{
		for(i=0;i<gfx_pages;i++) printf("%i%c",GFX_PAGE+i,i<gfx_pages-1?',':'\n');
	}
	else
	{
		printf("no data\n");
	}
	printf("Palettes and params:\t%i\n",PAL_PAGE);
	printf("Sound code and sfx:\t%i\n",SND_PAGE);
	printf("Music data:\t\t");
	if(mus_pages)
	{
		for(i=0;i<mus_pages;i++) printf("%i%c",mus_page_start+i,i<mus_pages-1?',':'\n');
	}
	else
	{
		printf("no data\n");
	}
	printf("Sample data:\t\t");
	if(smp_pages)
	{
		for(i=0;i<smp_pages;i++) printf("%i%c",smp_page_start+i,i<smp_pages-1?',':'\n');
	}
	else
	{
		printf("no data\n");
	}
	printf("Sprite data:\t\t");
	if(spr_pages)
	{
		for(i=0;i<spr_pages;i++) printf("%i%c",spr_page_start+i,i<spr_pages-1?',':'\n');
		printf("Sprite parameters:\t%i\n",SPRTBL_PAGE);
	}
	else
	{
		printf("no data\n");
		printf("Sprite parameters:\tno data\n");
	}
	printf("\n");

	return 0;
}