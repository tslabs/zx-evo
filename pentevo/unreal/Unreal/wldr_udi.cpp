#include "std.h"
#include "emul.h"
#include "vars.h"
#include "wd93crc.h"
#include "util.h"

struct UDI
{
   char label[3];
   unsigned char cyls;
   unsigned char sides;
};

int FDD::read_udi()
{
   free();
   unsigned c,s;
   unsigned mem = 0;
   unsigned char *ptr = snbuf + 0x10;

   for (c = 0; c <= snbuf[9]; c++)
   {
      for (s = 0; s <= snbuf[10]; s++)
      {
         if (*ptr)
             return 0;
         unsigned sz = *(unsigned short*)(ptr+1);
         sz += sz/8 + ((sz & 7)? 1 : 0);
         mem += sz;
         ptr += 3 + sz;
         if (ptr > snbuf + snapsize)
             return 0;
      }
   }

   cyls = snbuf[9]+1;
   sides = snbuf[10]+1;
   rawsize = align_by(mem, 4096);
   rawdata = (unsigned char*)VirtualAlloc(0, rawsize, MEM_COMMIT, PAGE_READWRITE);
   ptr = snbuf+0x10;
   unsigned char *dst = rawdata;

   for (c = 0; c < cyls; c++)
   {
      for (s = 0; s < sides; s++)
      {
         unsigned sz = *(unsigned short*)(ptr+1);
         trklen[c][s] = sz;
         trkd[c][s] = dst;
         trki[c][s] = dst + sz;
         sz += sz/8 + ((sz & 7)? 1 : 0);
         memcpy(dst, ptr+3, sz);
         ptr += 3 + sz;
         dst += sz;
      }
   }
   if (snbuf[11] & 1)
       strcpy(dsc, (char*)ptr);
   return 1;
}

int FDD::write_udi(FILE *ff)
{
   memset(snbuf, 0, 0x10);
   *(unsigned*)snbuf = WORD4('U','D','I','!');
   snbuf[8] = snbuf[11] = 0;
   snbuf[9] = cyls-1;
   snbuf[10] = sides-1;
   *(unsigned*)(snbuf+12) = 0;

   unsigned char *dst = snbuf+0x10;
   for (unsigned c = 0; c < cyls; c++)
      for (unsigned s = 0; s < sides; s++)
      {
         *dst++ = 0;
         unsigned len = trklen[c][s];
         *(unsigned short*)dst = len; dst += 2;
         memcpy(dst, trkd[c][s], len); dst += len;
         len = (len+7)/8;
         memcpy(dst, trki[c][s], len); dst += len;
      }
   if (*dsc) strcpy((char*)dst, dsc), dst += strlen(dsc)+1, snbuf[11] = 1;
   *(unsigned*)(snbuf+4) = dst-snbuf;
   int crc = -1; crc32(crc, snbuf, dst-snbuf);
   *(unsigned*)dst = crc; dst += 4;
   if (fwrite(snbuf, 1, dst-snbuf, ff) != (unsigned)(dst-snbuf)) return 0;
   return 1;
}
