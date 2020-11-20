#include "std.h"
#include "emul.h"
#include "vars.h"

void FDD::format_isd()
{
   static const u8 sn[] = { 1, 2, 3, 4, 9 };

   newdisk(80, 2);

   for (unsigned c = 0; c < cyls; c++)
   {
      for (unsigned h = 0; h < 2; h++)
      {
         t.seek(this, c, h, JUST_SEEK);
         t.s = 5;
         for (unsigned s = 0; s < 5; s++)
         {
            unsigned n = sn[s];
            t.hdr[s].n = n; t.hdr[s].l = 3;
            t.hdr[s].c = c; t.hdr[s].s = 0;
            t.hdr[s].c1 = t.hdr[s].c2 = 0;
            t.hdr[s].data = (u8*)1;
         }
         t.format();
      }
   }
}

int FDD::read_isd()
{
   static const u8 sn[] = { 1, 2, 3, 4, 9 };

   format_isd();

   for (unsigned c = 0; c < cyls; c++)
   {
      for (unsigned h = 0; h < 2; h++)
      {
         for (unsigned s = 0; s < 5; s++)
         {
            t.seek(this, c, h, LOAD_SECTORS);
            t.write_sector(sn[s], snbuf+(c*10 + h*5 + s)*1024);
         }
      }
   }
   return 1;
}

int FDD::write_isd(FILE *ff)
{
   for (unsigned c = 0; c < 80; c++)
   {
      for (unsigned h = 0; h < 2; h++)
      {
          t.seek(this, c, h, LOAD_SECTORS);
          for (unsigned s = 0; s < 5; s++)
          {
              if (fwrite(t.hdr[s].data, 1, 1024, ff) != 1024)
                 return 0;
          }
      }
   }
   return 1;
}
