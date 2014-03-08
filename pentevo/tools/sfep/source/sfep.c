#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;

#define mode_normal 0
#define mode_string 1
#define mode_comment 2

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
 BYTE h[]="0123456789ABCDEF";
 BYTE b, l, i, o, mode, needcomma;
 WORD nstr;
 BYTE si[256], so[256], filename[256];
 FILE *fi, *fo;

 printf("SFEP\n");
 // раз уж залез в исходник, то может придумаешь путёвое название проге?!
 // она делает из этого
 //        .DB     "Привет",0
 // вот это
 //        .DB     $8F,$E0,$A8,$A2,$A5,$E2,0
 // (вложенные кавычки не поддерживаются)
 if (argc!=2) { printf("usage: sfep <FileName>\n"); return 2; }
 filename[251]=0;
 strncpy(filename,argv[1],251);
 fi=fopen(filename,"rt");
 if (!fi) { printf("Can't open file %s!\n",filename); return 1; }

 l=strlen(filename);
 b=1;
 do
  b++;
 while ((l>b) && (b<4) && (filename[l-b]!='.'));
 if (filename[l-b]=='.')
  strcpy(&filename[l-b],".inc");
 else
  strcat(filename,".inc");
 fo=fopen(filename,"wt");
 if (!fo) { printf("Can't create output file!\n"); return 1; }

 nstr=1;
 si[254]=0;
 while (!feof(fi))
 {
  if (fgets(si,254,fi))
  {
   for (l=0; l<254; l++) if ( (si[l]==0) || (si[l]==0x0d) || (si[l]==0x0a) ) break;
   si[l]=0;
   i=0;
   o=0;
   mode=mode_normal;
   needcomma=0;
   do
   {
    b=si[i];
    switch (mode)
    {
     case mode_string:
                        if (b=='\"')
                         mode=mode_normal;
                        else
                        {
                         if (needcomma) so[o++]=',';
                         so[o++]='$';
                         so[o++]=h[b>>4];
                         so[o++]=h[b&0xf];
                         needcomma=1;
                        }
                        break;
     case mode_comment:
                        so[o++]=b;
                        break;
     default:
                        if (b=='\"')
                        {
                         mode=mode_string;
                         needcomma=0;
                        }
                        else
                        {
                         so[o++]=b;
                         if (b==',')
                          needcomma=0;
                         else if (b==';')
                          mode=mode_comment;
                        }
    }
    if (o>250)
    {
     printf("WARNING! Too long line %d. Loss of data is possible.\n",nstr);
     i=254;
    }
    i++;
   }
   while (i<l);
   so[o++]='\n';
   so[o]=0;
   fputs(so,fo);
   nstr++;
  }
 }

 fclose(fi);
 fclose(fo);
 printf("Created file %s\n",filename);
 return 0;
}
