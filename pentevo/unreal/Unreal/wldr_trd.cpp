#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

enum TDiskType
{
    DS_80 = 0x16,
    DS_40 = 0x17,
    SS_80 = 0x18,
    SS_40 = 0x19
};

enum { TRD_SIG = 0x10 };

#pragma pack(push, 1)
struct TTrdDirEntryBase
{
    char Name[8];
    u8 Type;
    u16 Start;
    u16 Length;
    u8 SecCnt; // Длина файла в секторах
};

struct TTrdDirEntry : public TTrdDirEntryBase
{
    u8 Sec; // Начальный сектор
    u8 Trk; // Начальная дорожка
};

struct TTrdSec9
{
    u8 Zero;         // 00
    u8 Reserved[224];
    u8 FirstFreeSec; // E1
    u8 FirstFreeTrk; // E2
    u8 DiskType;     // E3
    u8 FileCnt;      // E4
    u16 FreeSecCnt;  // E5, E6
    u8 TrDosSig;     // E7
    u8 Res1[2];      // | 0
    u8 Res2[9];      // | 32
    u8 Res3;         // | 0
    u8 DelFileCnt;   // F4
    char Label[8];   // F5-FC
    u8 Res4[3];      // | 0
};

struct TSclHdr
{
    u8 Sig[8];  // SINCLAIR
    u8 FileCnt;
#pragma warning(push)
#pragma warning(disable: 4200)
    TTrdDirEntryBase Files[];
#pragma warning(pop)
};
#pragma pack(pop)

void FDD::format_trd()
{
   static const u8 lv[3][16] =
    { { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 },
      { 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16 },
      { 1,12,7,2,13,8,3,14,9,4,15,10,5,16,11,6 } };

   newdisk(MAX_CYLS, 2);

   for (unsigned c = 0; c < cyls; c++) {
      for (unsigned side = 0; side < 2; side++) {
         t.seek(this, c, side, JUST_SEEK); t.s = 16;
         for (unsigned sn = 0; sn < 16; sn++) {
            unsigned s = lv[conf.trdos_interleave][sn];
            t.hdr[sn].n = s, t.hdr[sn].l = 1;
            t.hdr[sn].c = c, t.hdr[sn].s = 0;
            t.hdr[sn].c1 = t.hdr[sn].c2 = 0;
            t.hdr[sn].data = (u8*)1;
         }
         t.format();
      }
   }
}

void FDD::emptydisk()
{
    format_trd();
    t.seek(this, 0, 0, LOAD_SECTORS);
    const SECHDR *Sec9Hdr = t.get_sector(9);
    if (!Sec9Hdr)
        return;

    TTrdSec9 *Sec9 = (TTrdSec9 *)Sec9Hdr->data;
    Sec9->FirstFreeTrk = 1;           // first free track
    Sec9->DiskType = DS_80;           // 80T,DS
    Sec9->FreeSecCnt = 2544;          // free sec
    Sec9->TrDosSig = TRD_SIG;         // trdos flag
    memset(Sec9->Label, ' ', 8);      // label
    memset(Sec9->Res2, ' ', 9);       // reserved
    t.write_sector(9, Sec9Hdr->data); // update sector CRC
}

int FDD::addfile(u8 *hdr, u8 *data)
{
    t.seek(this, 0, 0, LOAD_SECTORS);
    const SECHDR *Sec9Hdr = t.get_sector(9);
    if (!Sec9Hdr)
        return 0;

    TTrdSec9 *Sec9 = (TTrdSec9 *)Sec9Hdr->data;

    if (Sec9->FileCnt >= 128) // Каталог заполнен полностью
        return 0;

    unsigned len = ((TTrdDirEntry *)hdr)->SecCnt;
    unsigned pos = Sec9->FileCnt * sizeof(TTrdDirEntry);
    const SECHDR *dir = t.get_sector(1 + pos / 0x100);

    if (!dir)
        return 0;

    if (Sec9->FreeSecCnt < len)
        return 0; // На диске нет места

    TTrdDirEntry *TrdDirEntry = (TTrdDirEntry *)(dir->data + (pos & 0xFF));
    memcpy(TrdDirEntry, hdr, 14);
    TrdDirEntry->Sec = Sec9->FirstFreeSec;
    TrdDirEntry->Trk = Sec9->FirstFreeTrk;
    t.write_sector(1 + pos / 0x100, dir->data);

    pos = Sec9->FirstFreeSec + 16*Sec9->FirstFreeTrk;
    Sec9->FirstFreeSec = (pos+len) & 0x0F;
    Sec9->FirstFreeTrk = (pos+len) >> 4;
    Sec9->FileCnt++;
    Sec9->FreeSecCnt -= len;
    t.write_sector(9, Sec9Hdr->data);

    // goto next track. s8 become invalid
    for (unsigned i = 0; i < len; i++, pos++)
    {
       t.seek(this, pos/32, (pos/16) & 1, LOAD_SECTORS);
       if (!t.trkd)
           return 0;
       if (!t.write_sector((pos & 0x0F) + 1, data + i * 0x100))
           return 0;
    }
    return 1;
}

static bool FillCheck(const void *buf, char fill, size_t size)
{
    const char *p = (const char *)buf;
    while (size--)
    {
        if (*p++ != fill)
            return false;
    }
    return true;
}

// destroys snbuf - use after loading all files
void FDD::addboot()
{
   t.seek(this, 0, 0, LOAD_SECTORS);

   // Проверка на то что диск имеет tr-dos формат
   const SECHDR *Hdr = t.get_sector(9);
   if (!Hdr)
       return;

   if ((Hdr->l & 3) != 1) // Проверка кода размера сектора (1 - 256 байт)
       return;

   const TTrdSec9 *Sec9 = (const TTrdSec9 *)Hdr->data;
   if (Sec9->Zero != 0)
       return;

   if (Sec9->TrDosSig != TRD_SIG)
       return;

   if (!(FillCheck(Sec9->Res2, ' ', 9) || FillCheck(Sec9->Res2, 0, 9)))
       return;

   if (!(Sec9->DiskType == DS_80 || Sec9->DiskType == DS_40 ||
       Sec9->DiskType == SS_80 || Sec9->DiskType == SS_40))
       return;

   for (unsigned s = 0; s < 8; s++)
   {
      const SECHDR *sc = t.get_sector(1 + s);
      if (!sc)
          return;
      TTrdDirEntry *TrdDirEntry = (TTrdDirEntry *)sc->data;
      for (unsigned i = 0; i < 16; i++)
      {
         if (memcmp(TrdDirEntry[i].Name, "boot    B", 9) == 0)
             return;
      }
   }

   FILE *f = fopen(conf.appendboot, "rb");
   if (!f)
       return;
   fread(snbuf, 1, sizeof snbuf, f);
   fclose(f);
   snbuf[13] = snbuf[14]; // copy length
   addfile(snbuf, snbuf+0x11);
}

int FDD::read_scl()
{
   emptydisk();
   unsigned size = 0, i;
   TSclHdr *SclHdr = (TSclHdr *)snbuf;
   for (i = 0; i < SclHdr->FileCnt; i++)
       size += SclHdr->Files[i].SecCnt;

   if (size > 2544)
   {
      t.seek(this, 0, 0, LOAD_SECTORS);
      const SECHDR *s8 = t.get_sector(9);
      TTrdSec9 *Sec9 = (TTrdSec9 *)s8->data;
      Sec9->FreeSecCnt = size;     // free sec
      t.write_sector(9, s8->data); // update sector CRC
   }
   u8 *data = snbuf + sizeof(TSclHdr) + SclHdr->FileCnt * sizeof(TTrdDirEntryBase);
   for (i = 0; i < SclHdr->FileCnt; i++)
   {
      if (!addfile((u8 *)&SclHdr->Files[i], data))
          return 0;
      data += SclHdr->Files[i].SecCnt * 0x100;
   }

   return 1;
}

int FDD::read_hob()
{
   if (!rawdata)
   {
       emptydisk();
   }
   snbuf[13] = snbuf[14];
   int r = addfile(snbuf, snbuf+0x11);
   return r;
}

int FDD::read_trd()
{
   format_trd();
   for (unsigned i = 0; i < snapsize; i += 0x100)
   {
      t.seek(this, i>>13, (i>>12) & 1, LOAD_SECTORS);
      t.write_sector(((i>>8) & 0x0F)+1, snbuf+i);
   }
   return 1;
}

int FDD::write_trd(FILE *ff)
{
   static u8 zerosec[256] = { 0 };

   for (unsigned i = 0; i < 2560; i++)
   {
      t.seek(this, i>>5, (i>>4) & 1, LOAD_SECTORS);
      const SECHDR *hdr = t.get_sector((i & 0x0F)+1);
      u8 *ptr = zerosec;
      if (hdr && hdr->data)
          ptr = hdr->data;
      if (fwrite(ptr, 1, 256, ff) != 256)
          return 0;
   }
   return 1;
}
