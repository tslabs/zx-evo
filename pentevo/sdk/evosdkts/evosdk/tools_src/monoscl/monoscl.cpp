#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>



int main(int argc,char* argv[])
{
	FILE *file;
	unsigned char *src,*dst;
	int i,pd,sec,off,sum,size,out_size,blocks,data_size;

	if(argc<2)
	{
		printf("Error: No input file\n");
		return 1;
	}

	file=fopen(argv[1],"rb");

	if(!file)
	{
		printf("Error: Can't open file\n");
		return 1;
	}

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	src=(unsigned char*)malloc(size);
	fread(src,size,1,file);
	fclose(file);

	if(memcmp(src,"SINCLAIR",8))
	{
		printf("Error: Wrong format\n");
		free(src);
		return 1;
	}

	sec=0;
	off=9;

	for(i=0;i<src[8];i++)
	{
		sec+=src[off+13];
		off+=14;
	}

	data_size=sec<<8;
	blocks=sec/255;
	if(sec%255) blocks++;

	dst=(unsigned char*)malloc(size);

	memcpy(dst,"SINCLAIR",8);
	dst[8]=blocks;

	pd=9;

	for(i=0;i<blocks;i++)
	{
		if(!i)
		{
			memcpy(&dst[pd],&src[9],14);
		}
		else
		{
			memcpy(&dst[pd],"data    ",8);
			dst[pd+8]=i-1+'0';
			dst[pd+9]=0;
			dst[pd+10]=0;
			dst[pd+11]=0;
			dst[pd+12]=0;
		}

		dst[pd+13]=sec>255?255:sec;

		sec-=255;
		pd+=14;
	}

	memcpy(&dst[pd],&src[off],data_size);
	pd+=data_size;

	free(src);

	sum=0;

	for(i=0;i<pd;i++) sum+=dst[i];

	dst[pd++]=sum&255;
	dst[pd++]=(sum>>8)&255;
	dst[pd++]=(sum>>16)&255;
	dst[pd++]=(sum>>24)&255;

	out_size=pd;

	file=fopen(argv[1],"wb");

	if(!file)
	{
		printf("Error: Can't create file\n");
		free(dst);
		return 1;
	}

	fwrite(dst,out_size,1,file);
	fclose(file);

	free(dst);
}