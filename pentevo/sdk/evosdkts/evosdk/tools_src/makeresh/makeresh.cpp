#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



unsigned int read_dword(unsigned char* data)
{
	return data[0]+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}



unsigned int read_word(unsigned char* data)
{
	return data[0]+(data[1]<<8);
}



bool get_bmp_dimensions(const char* filename,unsigned int& wdt,unsigned int& hgt)
{
	FILE *fin;
	int bpp,rle;
	unsigned char data[34];

	fin=fopen(filename,"rb");

	if(!fin) return false;

	fread(data,sizeof(data),1,fin);
	fclose(fin);

	bpp=read_word (&data[28]);
	rle=read_dword(&data[30]);

	if(bpp!=8||rle) return false;

	wdt=read_dword(&data[18]);
	hgt=read_dword(&data[22]);

	if((wdt&15)||(hgt&15)) return false;

	return true;
}



void make_define(char *line)
{
	unsigned int i;

	for(i=0;i<strlen(line);i++)
	{
		if(line[i]>='0'&&line[i]<='9') continue;
		if(line[i]>='A'&&line[i]<='Z') continue;
		if(line[i]>='a'&&line[i]<='z')
		{
			line[i]-=32;
			continue;
		}
		line[i]='_';
	}
}



void make_defines(FILE *fout,char* filename,char* prefix)
{
	FILE *fin;
	char line[1024];
	unsigned int i,id;

	fin=fopen(filename,"rt");

	if(!fin) return;

	id=0;

	while(fgets(line,sizeof(line),fin)!=NULL)
	{
		if(!memcmp(line,"rem",3)||!memcmp(line,":",1)) continue;

		line[strlen(line)-2]=0;

		i=strlen(line)-1;
		while(--i)
		{
			if(line[i]=='.')
			{
				line[i]=0;
				break;
			}
		}

		i=strlen(line)-1;
		while(--i)
		{
			if(line[i]=='\\'||line[i]=='/')
			{
				strcpy(line,&line[i+1]);
				break;
			}
		}

		make_define(line);

		fprintf(fout,"#define %s%s\t%i\n",prefix,line,id);
		id++;
	}

	fprintf(fout,"\n");
	fclose(fin);
}



void make_defines_sprites(FILE *fout,char* filename,char* prefix)
{
	FILE *fin;
	char line[1024];
	unsigned int i,wdt,hgt,spr;

	fin=fopen(filename,"rt");

	if(!fin) return;

	spr=0;

	while(fgets(line,sizeof(line),fin)!=NULL)
	{
		if(!memcmp(line,"rem",3)) continue;

		line[strlen(line)-1]=0;

		if(!get_bmp_dimensions(line,wdt,hgt)) continue;

		i=strlen(line)-1;
		while(--i)
		{
			if(line[i]=='.')
			{
				line[i]=0;
				break;
			}
		}

		i=strlen(line)-1;
		while(--i)
		{
			if(line[i]=='\\'||line[i]=='/')
			{
				strcpy(line,&line[i+1]);
				break;
			}
		}

		make_define(line);

		fprintf(fout,"#define %s%s\t%i\n",prefix,line,spr);

		spr+=(hgt>>4)*(wdt>>4);
	}

	fprintf(fout,"\n");
	fclose(fin);
}



int skip_effect(int fxn,unsigned char *buf,int size)
{
	int pp,it,noise;
	
	pp=0;
	
	while(pp<size)
	{
		it=buf[pp++];
		
		if(it&(1<<5)) pp+=2;

		if(it&(1<<6))
		{
			noise=buf[pp++];
			if(it==0xd0&&noise>=0x20) break;
		}
	}
	
	return pp;
}



void make_defines_sounds(FILE *fout,const char* filename,const char* prefix)
{
	FILE *fin;
	unsigned char* data;
	int i,off,len,rlen,size,effects_all;
	char line[1024];

	fin=fopen(filename,"rb");

	if(!fin) return;

	fseek(fin,0,SEEK_END);
	size=ftell(fin);
	fseek(fin,0,SEEK_SET);
	data=(unsigned char*)malloc(size);
	fread(data,size,1,fin);
	fclose(fin);

	effects_all=data[0];
	
	for(i=0;i<effects_all;++i)
	{
		off=read_word(&data[1+i*2])+2+i*2;

		if(i<effects_all-1)
		{
			len=read_word(&data[3+i*2])+4+i*2-off;
		}
		else
		{
			len=size-off;
		}

		rlen=skip_effect(i,&data[off],len);
		
		if(rlen!=len)
		{
			strcpy(line,(const char*)&data[off+rlen]);
		}
		else
		{
			sprintf(line,"noname%3.3i",i+1);
		}

		make_define(line);

		fprintf(fout,"#define %s%s\t%i\n",prefix,line,i);
	}

	free(data);
}



int main(int argc,char* argv[])
{
	FILE *fout;

	if(argc!=7) return 1;

	fout=fopen("resources.h","wt");

	if(!fout) return 1;

	fprintf(fout,"//Автоматически генерируемые идентификаторы ресурсов\n\n");

	make_defines(fout,argv[1],"IMG_");
	make_defines(fout,argv[2],"PAL_");
	make_defines(fout,argv[3],"MUS_");
	make_defines(fout,argv[4],"SMP_");
	make_defines_sprites(fout,argv[5],"SPR_");
	make_defines_sounds(fout,argv[6],"SFX_");

	fclose(fout);

	return 0;
}
