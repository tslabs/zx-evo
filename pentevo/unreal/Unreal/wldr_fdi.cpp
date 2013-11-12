#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

int FDD::read_fdi()
{
   newdisk(snbuf[4], snbuf[6]);
   strncpy(dsc, (char*)snbuf + *(u16*)(snbuf+8), sizeof dsc);

   int res = 1;
   u8 *trk = snbuf + 0x0E + *(u16*)(snbuf+0x0C);
   u8 *dat = snbuf + *(u16*)(snbuf+0x0A);

   for (unsigned c = 0; c < snbuf[4]; c++)
   {
      for (unsigned s = 0; s < snbuf[6]; s++)
      {
         t.seek(this, c,s, JUST_SEEK);
         u8 *t0 = dat + *(unsigned*)trk;
         unsigned ns = trk[6]; trk += 7;
         for (unsigned sec = 0; sec < ns; sec++)
         {
            *(unsigned*)&t.hdr[sec] = *(unsigned*)trk;
            t.hdr[sec].c1 = 0;
            if (trk[4] & 0x40)
                t.hdr[sec].data = 0;
            else
            {
               t.hdr[sec].data = t0 + *(u16*)(trk+5);
               if (t.hdr[sec].data+128 > snbuf+snapsize)
                   return 0;
               t.hdr[sec].c2 = (trk[4] & (1<<(trk[3] & 3))) ? 0:2; // [vv]
            }
/* [vv]
            if (t.hdr[sec].l>5)
            {
                t.hdr[sec].data = 0;
                if (!(trk[4] & 0x40))
                    res = 0;
            }
*/
            trk += 7;
         }
         t.s = ns;
         t.format();
      }
   }
   return res;
}

int FDD::write_fdi(FILE *ff)
{
   unsigned c,s, total_s = 0;
   for (c = 0; c < cyls; c++)
      for (s = 0; s < sides; s++) {
         t.seek(this, c,s, LOAD_SECTORS);
         total_s += t.s;
      }
   unsigned tlen = strlen(dsc)+1;
   unsigned hsize = 14+(total_s+cyls*sides)*7;
   *(unsigned*)snbuf = WORD4('F','D','I',0);
   *(u16*)(snbuf+4) = cyls;
   *(u16*)(snbuf+6) = sides;
   *(u16*)(snbuf+8) = hsize;
   *(u16*)(snbuf+0x0A) = hsize + tlen;
   *(u16*)(snbuf+0x0C) = 0;
   fwrite(snbuf, 1, 14, ff);
   unsigned trkoffs = 0;
   for (c = 0; c < cyls; c++)
      for (s = 0; s < sides; s++) {
         t.seek(this,c,s,LOAD_SECTORS);
         unsigned secoffs = 0;
         *(unsigned*)snbuf = trkoffs;
         *(unsigned*)(snbuf+4) = 0;
         snbuf[6] = t.s;
         fwrite(snbuf, 1, 7, ff);
         for (unsigned se = 0; se < t.s; se++) {
            *(unsigned*)snbuf = *(unsigned*)&t.hdr[se];
            snbuf[4] = t.hdr[se].c2 ? (1<<(t.hdr[se].l & 3)) : 0; // [vv]
            if (t.hdr[se].data && t.hdr[se].data[-1] == 0xF8) snbuf[4] |= 0x80;
            if (!t.hdr[se].data) snbuf[4] |= 0x40;
            *(unsigned*)(snbuf+5) = secoffs;
            fwrite(snbuf, 1, 7, ff);
            secoffs += t.hdr[se].datlen;
         }
         trkoffs += secoffs;
      }
   fseek(ff, hsize, SEEK_SET);
   fwrite(dsc, 1, tlen, ff);
   for (c = 0; c < cyls; c++)
      for (s = 0; s < sides; s++) {
         t.seek(this,c,s,LOAD_SECTORS);
         for (unsigned se = 0; se < t.s; se++)
            if (t.hdr[se].data)
               if (fwrite(t.hdr[se].data, 1, t.hdr[se].datlen, ff) != t.hdr[se].datlen) return 0;
      }
   return 1;
}
