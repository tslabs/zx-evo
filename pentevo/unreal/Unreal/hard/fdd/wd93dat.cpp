#include "std.h"
#include "emul.h"
#include "vars.h"
#include "snapshot.h"
#include "util.h"

int FDD::index()
{
   return this - comp.wd.fdd;
}

// return: 0 - CANCEL, 1 - YES/SAVED, 2 - NOT SAVED
char FDD::test()
{
   if (!optype) return 1;

   static char changed[] = "Disk A: changed. Save ?"; changed[5] = index()+'A';
   int r = MessageBox(GetForegroundWindow(), changed, "Save disk", exitflag? MB_ICONQUESTION | MB_YESNO : MB_ICONQUESTION | MB_YESNOCANCEL);
   if (r == IDCANCEL) return 0;
   if (r == IDNO) return 2;
   // r == IDYES
   u8 *image = (u8*)malloc(sizeof snbuf);
   memcpy(image, snbuf, sizeof snbuf);
   savesnap(index());
   memcpy(snbuf, image, sizeof snbuf);
   ::free(image);

   if (*(volatile char*)&optype) return 0;
   return 1;
}

void FDD::free()
{
   if (rawdata)
       VirtualFree(rawdata, 0, MEM_RELEASE);

   motor = 0;
   track = 0;

   rawdata = 0;
   unsigned rawsize = 0;
   cyls = 0;
   sides = 0;
   name[0] = 0;
   dsc[0] = 0;
   memset(trklen, 0,  sizeof(trklen));
   memset(trkd, 0,  sizeof(trkd));
   memset(trki, 0,  sizeof(trki));
   optype = 0;
   snaptype = 0;
   conf.trdos_wp[index()] = 0;
   /*comp.wd.trkcache.clear(); [vv]*/
   t.clear();
}

void FDD::newdisk(unsigned cyls, unsigned sides)
{
   free();

   FDD::cyls = cyls; FDD::sides = sides;
   unsigned len = MAX_TRACK_LEN;
   unsigned len2 = len + (len/8) + ((len & 7) ? 1 : 0);
   rawsize = align_by(cyls * sides * len2, 4096);
   rawdata = (u8*)VirtualAlloc(0, rawsize, MEM_COMMIT, PAGE_READWRITE);
   // ZeroMemory(rawdata, rawsize); // already done by VirtualAlloc

   for (unsigned i = 0; i < cyls; i++)
      for (unsigned j = 0; j < sides; j++) {
         trklen[i][j] = len;
         trkd[i][j] = rawdata + len2*(i*sides + j);
         trki[i][j] = trkd[i][j] + len;
      }
   // comp.wd.trkcache.clear(); // already done in free()
}

int FDD::read(u8 type)
{
   int ok = 0;
   switch(type)
   {
   case snTRD: ok = read_trd(); break;
   case snUDI: ok = read_udi(); break;
   case snHOB: ok = read_hob(); break;
   case snSCL: ok = read_scl(); break;
   case snFDI: ok = read_fdi(); break;
   case snTD0: ok = read_td0(); break;
   case snISD: ok = read_isd(); break;
   case snPRO: ok = read_pro(); break;
   }
   return ok;
}

bool done_fdd(bool Cancelable)
{
   for (int i = 0; i < 4; i++)
      if (!comp.wd.fdd[i].test() && Cancelable)
          return false;
   return true;
}
