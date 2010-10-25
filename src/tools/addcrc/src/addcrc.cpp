// addcrc.cpp : Defines the entry point for the console application.
//

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>



unsigned int calc_crc(unsigned char *,unsigned int,unsigned int init_crc=0x0000FFFF);


int main(int argc, char* argv[])
{
	unsigned int filesize;
	unsigned int crc16;

	FILE * infile;

	unsigned char * indata = 0;

	if(argc < 2)
	{
	   printf("usage: crc16ccitt file\n");
	   return -1;
	}

     	infile = fopen(argv[1],"rb");
     	if( infile==NULL )
     	{
     		printf("cant open %s!\n",argv[1]);
     		return -1;
     	}


       	// get length of input file
       	fseek(infile,0,SEEK_END);
       	filesize=ftell(infile);
       	fseek(infile,0,SEEK_SET);



       	// load file in mem
       	indata=(unsigned char*)malloc(filesize);
       	if(!indata)
       	{
       		printf("can't allocate memory for input file!\n");\
       		fclose(infile);
       		return -1;
       	}

       	if(filesize != fread(indata,1,filesize,infile))
       	{
       		printf("can't load input file in mem!\n");
       		fclose(infile);
  	        free(indata);
       		return -1;
       	}

	
       	// calc and write crc
       	crc16=0x0000FFFF;
       	
       	crc16 = calc_crc(indata,filesize,crc16);

       	printf("crc16_ccitt is 0x%X\n",crc16);

	FILE *f = fopen("crc.bin", "wb");
	if(f)
	{
    	    fwrite(&crc16, 1, 2, f);
      	    fclose(f);
	}
	
	free(indata);

	return 0;
}



unsigned int calc_crc(unsigned char * data,unsigned int size,unsigned int init_crc)
{

	unsigned int i,ctr,crc;

	unsigned char * ptr;

	crc = init_crc;

	ptr=data;
	ctr=0;
	while( (ctr++) <size)
	{
		crc ^= (*ptr++) << 8;
		
		for(i=8;i!=0;i--)
			crc = (crc&0x00008000) ? ( (crc<<1)^0x1021 ) : (crc<<1);
	}

	return crc&0x0000FFFF;
}
