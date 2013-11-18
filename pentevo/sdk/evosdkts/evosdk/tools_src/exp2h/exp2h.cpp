#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int main(int argc,char* argv[])
{
	FILE *fin,*fout;
	char line[1024];
	unsigned int pp,pp1;

	if(argc<2) return 1;

	strcpy(line,argv[1]);
	line[strlen(line)-3]='h';
	line[strlen(line)-2]=0;

	fin=fopen(argv[1],"rt");

	if(!fin) return 1;

	fout=fopen(line,"wt");

	if(!fout)
	{
		fclose(fin);
		return 1;
	}

	while(fgets(line,sizeof(line),fin)!=NULL)
	{
		if(line[strlen(line)-1]<0x20) line[strlen(line)-1]=0;

		pp=0;

		while(pp<strlen(line))
		{
			if(line[pp]==':') break;
			if(line[pp]>='a'&&line[pp]<='z') line[pp]-=32;
			pp++;
		}

		line[pp++]='\t';
		pp1=pp;

		while(pp<strlen(line))
		{
			if(line[pp]=='0') break;
			pp++;
		}

		strcpy(&line[pp1],&line[pp]);

		fprintf(fout,"#define %s\n",line);
	}

	fclose(fin);
	fclose(fout);

	return 0;
}
