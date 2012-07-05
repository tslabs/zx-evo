#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"

void draw_alco_384()
{
   unsigned ofs = (comp.p7FFD & 8) << 12;
   unsigned char *dst = rbuf;
   for (unsigned y = 0; y < temp.scy; y++) {
      for (unsigned x = 0; x < 6; x++) {

         unsigned char *data = t.alco[y][x].s+ofs, *attr = t.alco[y][x].a+ofs;
         unsigned d = *(unsigned*)data, a = *(unsigned*)attr;
         *(unsigned*)dst = (d & 0xFF) + colortab_s8[a & 0xFF] +
                           ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+4) = ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         d = *(unsigned*)(data+4), a = *(unsigned*)(attr+4);
         *(unsigned*)(dst+8) = (d & 0xFF) + colortab_s8[a & 0xFF] +
                               ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+12)= ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         dst += 16;
      }
   }
}

void draw_alco_320()
{
   unsigned ofs = (comp.p7FFD & 8) << 12;
   unsigned char *dst = rbuf;
   unsigned base = (304-temp.scy)/2;
   for (unsigned y = 0; y < temp.scy; y++) {

      unsigned char *data = t.alco[base+y][0].s+ofs+4, *attr = t.alco[base+y][0].a+ofs+4;
      unsigned d = *(unsigned*)data, a = *(unsigned*)attr;
      *(unsigned*)dst = (d & 0xFF) + colortab_s8[a & 0xFF] +
                        ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
      *(unsigned*)(dst+4) = ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                            ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
      dst += 8;

      for (unsigned x = 1; x < 5; x++) {
         data = t.alco[base+y][x].s+ofs, attr = t.alco[base+y][x].a+ofs;
         d = *(unsigned*)data, a = *(unsigned*)attr;
         *(unsigned*)dst = (d & 0xFF) + colortab_s8[a & 0xFF] +
                           ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+4) = ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         d = *(unsigned*)(data+4), a = *(unsigned*)(attr+4);
         *(unsigned*)(dst+8) = (d & 0xFF) + colortab_s8[a & 0xFF] +
                               ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+12)= ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         dst += 16;
      }

      data = t.alco[base+y][5].s+ofs, attr = t.alco[base+y][5].a+ofs;
      d = *(unsigned*)data, a = *(unsigned*)attr;
      *(unsigned*)dst = (d & 0xFF) + colortab_s8[a & 0xFF] +
                        ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
      *(unsigned*)(dst+4) = ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                            ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
      dst += 8;

   }
}

void draw_alco_256()
{
   unsigned ofs = (comp.p7FFD & 8) << 12;
   unsigned char *dst = rbuf;
   unsigned base = (304-temp.scy)/2;
   for (unsigned y = 0; y < temp.scy; y++) {
      for (unsigned x = 1; x < 5; x++) {

         unsigned char *data = t.alco[base+y][x].s+ofs, *attr = t.alco[base+y][x].a+ofs;
         unsigned d = *(unsigned*)data, a = *(unsigned*)attr;
         *(unsigned*)dst = (d & 0xFF) + colortab_s8[a & 0xFF] +
                           ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+4) = ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         d = *(unsigned*)(data+4), a = *(unsigned*)(attr+4);
         *(unsigned*)(dst+8) = (d & 0xFF) + colortab_s8[a & 0xFF] +
                               ((d << 8) & 0xFF0000) + colortab_s24[(a >> 8) & 0xFF];
         *(unsigned*)(dst+12)= ((d >> 16) & 0xFF) + colortab_s8[(a >> 16) & 0xFF] +
                               ((d >> 8) & 0xFF0000) + colortab_s24[(a >> 24) & 0xFF];
         dst += 16;
      }
   }
}

void draw_alco()
{
   if (temp.scx == 384) { draw_alco_384(); return; }
   if (temp.scx == 320) { draw_alco_320(); return; }
   draw_alco_256();
}
