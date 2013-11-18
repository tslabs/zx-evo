#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long LONGWORD;

//-----------------------------------------------------------------------------

int main(int argc,char*argv[])
{
 BYTE       h[]="0123456789ABCDEF";
 BYTE       b0, b1, l, az;
 LONGWORD   i, z;
 BYTE       s[256];
 BYTE       buff[0x10000];
 FILE*      f;

 printf("BinnaryFile_To_AVRAssmFile Converter\n");
 if (argc==1) { printf("usage: bin2avr <FileName> [0]\n"); return 2; }
 az=0;
 if (argc==3)
 {
  strncpy(s,argv[2],1);
  if (s[0]=='0') az=1;
 }
 strncpy(s,argv[1],255);
 f=fopen(s,"rb");
 if (!f) { printf("Can't open file %s!\n",s); return 1; }

 for (i=0;i<0x10000;i++) buff[i]=0x00;
 z=fread(buff,1,0x10000,f);
 fclose(f);
 if (!z) { printf("Can't read file %s!\n",s); return 1; }

 l=strlen(s);
 b0=1;
 do
  b0++;
 while ((l>b0) && (b0<4) && (s[l-b0]!='.'));
 if (s[l-b0]=='.')
   strcpy(&s[l-b0],".inc");
  else
   strcat(s,".inc");

 f=fopen(s,"wt");
 if (!f) { printf("Can't create output file!\n"); return 1; }

 z+=az;
 i=0;
 l=0;
 do
 {
  if (!l) fputs("        .DW     ",f);
  b0=buff[i++];
  b1=buff[i++];
  fputc('$',f);
  fputc(h[b1>>4],f);
  fputc(h[b1&0x0f],f);
  fputc(h[b0>>4],f);
  fputc(h[b0&0x0f],f);
  if (l==7)
    fputc('\n',f);
   else
    if (i<z) fputc(',',f);
  l++; l&=0x07;
 }
 while (i<z);
 fputc('\n',f);
 fclose(f);
 printf("Created file %s\n",s);
 return 0;
}
