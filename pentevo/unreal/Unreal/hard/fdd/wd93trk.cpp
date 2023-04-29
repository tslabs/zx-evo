#include "std.h"
#include "emul.h"
#include "vars.h"
#include "wd93crc.h"
#include "util.h"

void TRKCACHE::seek(FDD *d, unsigned cyl, unsigned side, SEEK_MODE fs)
{
   if ((d == drive) && (sf == fs) && (cyl == TRKCACHE::cyl) && (side == TRKCACHE::side))
       return;

   drive = d; sf = fs; s = 0;
   TRKCACHE::cyl = cyl; TRKCACHE::side = side;
   if (cyl >= d->cyls || !d->rawdata)
   {
       trkd = 0;
       return;
   }

   assert(cyl < MAX_CYLS);
   trkd = d->trkd[cyl][side];
   trki = d->trki[cyl][side];
   trklen = d->trklen[cyl][side];
   if (!trklen)
   {
       trkd = 0;
       return;
   }

   ts_byte = Z80FQ / (trklen * FDD_RPS);
   if (fs == JUST_SEEK)
       return; // else find sectors

   for (unsigned i = 0; i < trklen - 8; i++)
   {
      if (trkd[i] != 0xA1 || trkd[i+1] != 0xFE || !test_i(i)) // Поиск idam
          continue;

      if (s == MAX_SEC)
          errexit("too many sectors");

      SECHDR *h = &hdr[s++]; // Заполнение заголовка
      h->id = trkd + i + 2; // Указатель на заголовок сектора
      h->c = h->id[0];
      h->s = h->id[1];
      h->n = h->id[2];
      h->l = h->id[3];
      h->crc = *(u16*)(trkd+i+6);
      h->c1 = (wd93_crc(trkd+i+1, 5) == h->crc);
      h->data = 0;
      h->datlen = 0;
//      if (h->l > 5) continue; [vv]

      unsigned end = min(trklen - 8, i + 8 + 43); // 43-DD, 30-SD

      // Формирование указателя на зону данных сектора
      for (unsigned j = i + 8; j < end; j++)
      {
         if (trkd[j] != 0xA1 || !test_i(j) || test_i(j+1))
             continue;

         if (trkd[j+1] == 0xF8 || trkd[j+1] == 0xFB) // Найден data am
         {
            h->datlen = 128 << (h->l & 3); // [vv] FD1793 use only 2 lsb of sector size code
            h->data = trkd + j + 2;
            h->c2 = (wd93_crc(h->data-1, h->datlen+1) == *(u16*)(h->data+h->datlen));
         }
         break;
      }
   }
}

void TRKCACHE::format()
{
   memset(trkd, 0, trklen);
   memset(trki, 0, trklen/8 + ((trklen&7) ? 1:0));

   u8 *dst = trkd;

   unsigned i;
   memset(dst, 0x4E, 80); dst += 80; // gap4a
   memset(dst, 0, 12); dst += 12; //sync

   for (i = 0; i < 3; i++) // iam
       write(dst++ - trkd, 0xC2, 1);
   *dst++ = 0xFC;

   for (unsigned is = 0; is < s; is++)
   {
      memset(dst, 0x4E, 40); dst += 40; // gap1 // 50 [vv] // fixme: recalculate gap1 only for non standard formats
      memset(dst, 0, 12); dst += 12; //sync
      for (i = 0; i < 3; i++) // idam
          write(dst++ - trkd, 0xA1, 1);
      *dst++ = 0xFE;

      SECHDR *sechdr = hdr + is;
      *dst++ = sechdr->c; // c
      *dst++ = sechdr->s; // h
      *dst++ = sechdr->n; // s
      *dst++ = sechdr->l; // n

      unsigned crc = wd93_crc(dst-5, 5); // crc
      if (sechdr->c1 == 1)
          crc = sechdr->crc;
      if (sechdr->c1 == 2)
          crc ^= 0xFFFF;
      *(unsigned*)dst = crc;
      dst += 2;

      if (sechdr->data)
      {
         memset(dst, 0x4E, 22); dst += 22; // gap2
         memset(dst, 0, 12); dst += 12; //sync
         for (i = 0; i < 3; i++) // data am
             write(dst++ - trkd, 0xA1, 1);
         *dst++ = 0xFB;

//         if (sechdr->l > 5) errexit("strange sector"); // [vv]
         unsigned len = 128 << (sechdr->l & 3); // data
         if (sechdr->data != (u8*)1)
             memcpy(dst, sechdr->data, len);
         else
             memset(dst, 0, len);

         crc = wd93_crc(dst-1, len+1); // crc
         if (sechdr->c2 == 1)
             crc = sechdr->crcd;
         if (sechdr->c2 == 2)
             crc ^= 0xFFFF;
         *(unsigned*)(dst+len) = crc;
             dst += len+2;
      }
   }
   if (dst > trklen + trkd)
       errmsg("track too long");
   while (dst < trkd + trklen)
       *dst++ = 0x4E;
}

#if 1
void TRKCACHE::dump()
{
   printf("\n%d/%d:", cyl, side);
   if (!trkd) { printf("<e>"); return; }
   if (!sf) { printf("<n>"); return; }
   for (unsigned i = 0; i < s; i++)
      printf("%c%02X-%02X-%02X-%02X,%c%c%c", i?' ':'<', hdr[i].c,hdr[i].s,hdr[i].n,hdr[i].l, hdr[i].c1?'+':'-', hdr[i].c2?'+':'-', hdr[i].data?'d':'h');
   printf(">");
}
#endif

int TRKCACHE::write_sector(unsigned sec, u8 *data)
{
   const SECHDR *h = get_sector(sec);
   if (!h || !h->data)
       return 0;
   unsigned sz = h->datlen;
   memcpy(h->data, data, sz);
   *(u16*)(h->data+sz) = (u16)wd93_crc(h->data-1, sz+1);
   return sz;
}

const SECHDR *TRKCACHE::get_sector(unsigned sec) const
{
   unsigned i;
   for (i = 0; i < s; i++)
   {
      if (hdr[i].n == sec)
          break;
   }
   if (i == s)
       return 0;

//   dump();

   if (/*(hdr[i].l & 3) != 1 ||*/ hdr[i].c != cyl) // [vv]
       return 0;
   return &hdr[i];
}

