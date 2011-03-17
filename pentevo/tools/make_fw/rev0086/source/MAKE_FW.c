#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long LONGWORD;

#define HEXLEN 0
#define DATATYPE 1
#define CHECKSUM 2

BYTE      checksum;
WORD      col, row;
LONGWORD  err;
BYTE      s[256], s1[256];

//-----------------------------------------------------------------------------

BYTE toupper(BYTE b)
{
 if ( (b>0x61) && (b<0x7b) )  b&=0xdf;
 return b;
}

//-----------------------------------------------------------------------------

void print_err_rc()
{
 printf("Error! (Row %d, Col %d)\n",row,col+1);
 err++;
}

//-----------------------------------------------------------------------------

void print_err_r(BYTE cause)
{
 BYTE* cause_str[3]={"Number of byte","Unknown datatype","Checksum"};
 printf("Error! %s. (Row %d)\n",cause_str[cause],row);
 err++;
}

//-----------------------------------------------------------------------------

BYTE getbyte()
{
 BYTE  b1, b0;

 b1=s[col];
 if ( ( (b1>=0x30)&&(b1<=0x39) )||( (b1>=0x41)&&(b1<=0x46) ) )
 {
  b1-=0x30;
  if (b1>9) b1-=7;
 }
 else
 {
  print_err_rc();
  b1=0;
 }
 col++;

 b0=s[col];
 if ( ( (b0>=0x30)&&(b0<=0x39) )||( (b0>=0x41)&&(b0<=0x46) ) )
 {
  b0-=0x30;
  if (b0>9) b0-=7;
 }
 else
 {
  print_err_rc();
  b0=0;
 }
 col++;

 b0|=(b1<<4);
 checksum+=b0;
 return b0;
}

//-----------------------------------------------------------------------------

int main(int argc,char*argv[])
{
 BYTE       h[]="0123456789ABCDEF";
 BYTE       b, m, o, hexlen, datatype;
 WORD       i, ih, crc;
 LONGWORD   x0, x1, adr, segadr;
 struct tm  stm;
 BYTE       vs[57]; //58-1
 WORD       tabcrc[256];
 BYTE       buff[0x1e000];
 BYTE       header[0x80];
 FILE*      f;

 printf("ZX EVO project:  HEX to BIN + CRC + Header\n");
 if (argc<3) { printf("usage: MAKE_FW <HexFileName> <VersionFileName>\n"); return 2; }

 header[0]='Z';
 header[1]='X';
 header[2]='E';
 header[3]='V';
 header[4]='O';
 header[5]=0x1a;
 for (ih=0x06; ih<0x80; ih++) header[ih]=0;
 ih=6;
 o=0;
 if (argc==4)
  {
   strncpy(s1,argv[3],1);
   if (s1[0]=='o') o=0x80;
  }

 strncpy(s1,argv[2],255);
 f=fopen(s1,"rt");
 vs[0]=0;
 if (f)
  {
   fgets(vs,56,f);
   fclose(f);
  }
 i=strlen(vs);
 if ((i) && (vs[i-1]=='\n')) vs[--i]=0;
 if (!i)
  {
   strcpy(vs, "No info");
   o=0;
  }

 strcpy(&header[ih], vs);
 ih=strlen(header);

 strncpy(s1,argv[1],255);
 f=fopen(s1,"rt");
 if (!f) { printf("Can't open file %s!\n",s1); return 1; }

 for (adr=0;adr<0x1e000;adr++) buff[adr]=0xff;
 err=0;
 segadr=0;
 row=0;
 while (!feof(f))
 {
  row++;
  col=0;
  if (fgets(s,255,f) && strlen(s))
  {
   if (s[col]!=':') print_err_rc();
   col++;
   checksum=0;
   hexlen=getbyte();
   x1=getbyte();
   x0=getbyte();
   adr=segadr|(x1<<8)|x0;
   datatype=getbyte();
   switch (datatype)
   {
    // Data record
    case 0: while (hexlen>0)
            {
             b=getbyte();
             hexlen--;
             if (adr<0x1e000) buff[adr]=b;
             adr++;
            }
            break;
    // End of file record
    case 1: if (hexlen!=0) print_err_r(HEXLEN);
            break;
    // Extended segment address record
    case 2: x1=getbyte();
            x0=getbyte();
            segadr=(x1<<12)|(x0<<4);
            if (hexlen!=2) print_err_r(HEXLEN);
            break;
    // Start segment address record
    case 3: break;
    // Extended linear address record
    case 4: x1=getbyte();
            x0=getbyte();
            segadr=(x1<<24)|(x0<<16);
            if (hexlen!=2) print_err_r(HEXLEN);
            break;
    // Start linear address record
    case 5: break;
    default: print_err_r(DATATYPE);
             while (hexlen!=0) { getbyte(); hexlen--; }
   }
   getbyte();
   if (checksum!=0) print_err_r(CHECKSUM);
  }
 }
 fclose(f);

 if (err) { printf("Total %d error(s)!\n",err); return 3; }


 // comments place
 {
  time_t tt;
  tt=time(NULL);
  memcpy(&stm,localtime(&tt),sizeof(stm));
 }
 i=(WORD)(((stm.tm_year-100)&0x3f)<<9) | (((stm.tm_mon+1)&0x0f)<<5) | (stm.tm_mday&0x1f);
 header[0x003e]=buff[0x1dffc]=(i>>8)&0x7f|o;
 header[0x003f]=buff[0x1dffd]=i&0xff;

 strncpy(&buff[0x1dff0], vs, 12);

 for (i=0;i<256;i++)
 {
  crc=i<<8;
  b=8;
  do
  {
   if (crc&0x8000)
    crc=(crc<<1)^0x1021;
   else
    crc<<=1;
   b--;
  }
  while ((b)&&(crc));
  tabcrc[i]=crc;
 }

 crc=0xffff;
 for (adr=0;adr<0x1dffe;adr++) crc=tabcrc[(crc>>8)^buff[adr]]^(crc<<8);
 buff[0x1dffe]=crc>>8;
 buff[0x1dfff]=crc&0xff;

 segadr=0x40;
 adr=0;
 do
 {
  m=0;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x01;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x02;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x04;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x08;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x10;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x20;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x40;
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) m|=0x80;
  header[segadr++]=m;
 }
 while (adr<0x1e000);

 crc=0x0000;
 for (i=0;i<0x7e;i++) crc=tabcrc[(crc>>8)^header[i]]^(crc<<8);
 header[0x7e]=crc>>8;
 header[0x7f]=crc&0xff;

 f=fopen("ZXEVO_FW.BIN","wb");
 if (!f) { printf("Can't create output file!\n"); return 1; }
 fwrite(header,1,0x80,f);
 adr=0;
 do
 {
  b=0xff;
  for (i=0;i<256;i++) b&=buff[adr++];
  if (b!=0xff) fwrite(&buff[adr-256],256,1,f);
 }
 while (adr<0x1e000);
 fclose(f);
 printf("Created file ZXEVO_FW.BIN\n");
 return 0;
}
