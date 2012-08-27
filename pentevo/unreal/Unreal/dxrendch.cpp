#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dx.h"
#include "dxrframe.h"

RENDER_FUNC auto_ch = 0;

void __fastcall _render_ch_ax(unsigned char *dst, unsigned pitch, RENDER_FUNC c2, RENDER_FUNC c4)
{
   if (conf.ch_size == 2) { c2(dst, pitch); return; }
   if (conf.ch_size == 4) { c4(dst, pitch); return; }
   static unsigned counter;
   unsigned i; //Alone Coder 0.36.7
   if (!auto_ch || needclr || !(++counter & 7)) {
      unsigned char n[0x100];
      for (/*unsigned*/ i = 0; i < 0x100; i++) n[i] = 0;
      for (i = 0; i < 0x300; i++) n[temp.base[i+0x1800]] = 1;
      unsigned max = 0;
      for (i = 0; i < 0x100; i++) max += n[i];
      auto_ch = (max > 3) ? c2 : c4;
   }
   auto_ch(dst, pitch);
}

void get_c2_32() {
   unsigned char *base = temp.base;
   unsigned char *attr = base + 0x1800;

   unsigned char *ptr = (unsigned char*)t.bs2h;
   for (unsigned p = 0; p < 3; p++) {
      for (unsigned y = 0; y < 8; y++) {
         for (unsigned x = 0; x < 32; x++) {
            unsigned *tab = t.c32tab[*attr++];
            unsigned r =
                t.settab2[base[y*0x20+x]] +
                t.settab2[base[y*0x20+x+0x100]];
            *(unsigned*)(ptr+16*x+0) = tab[r>>24];
            *(unsigned*)(ptr+16*x+4) = tab[(r>>16)&0xFF];
            *(unsigned*)(ptr+16*x+8) = tab[(r>>8)&0xFF];
            *(unsigned*)(ptr+16*x+12) = tab[r&0xFF];
            r = t.settab2[base[y*0x20+x+0x200]] +
                t.settab2[base[y*0x20+x+0x300]];
            *(unsigned*)(ptr+16*x+129*4+0) = tab[r>>24];
            *(unsigned*)(ptr+16*x+129*4+4) = tab[(r>>16)&0xFF];
            *(unsigned*)(ptr+16*x+129*4+8) = tab[(r>>8)&0xFF];
            *(unsigned*)(ptr+16*x+129*4+12) = tab[r&0xFF];
            r = t.settab2[base[y*0x20+x+0x400]] +
                t.settab2[base[y*0x20+x+0x500]];
            *(unsigned*)(ptr+16*x+129*8+0) = tab[r>>24];
            *(unsigned*)(ptr+16*x+129*8+4) = tab[(r>>16)&0xFF];
            *(unsigned*)(ptr+16*x+129*8+8) = tab[(r>>8)&0xFF];
            *(unsigned*)(ptr+16*x+129*8+12) = tab[r&0xFF];
            r = t.settab2[base[y*0x20+x+0x600]] +
                t.settab2[base[y*0x20+x+0x700]];
            *(unsigned*)(ptr+16*x+129*12+0) = tab[r>>24];
            *(unsigned*)(ptr+16*x+129*12+4) = tab[(r>>16)&0xFF];
            *(unsigned*)(ptr+16*x+129*12+8) = tab[(r>>8)&0xFF];
            *(unsigned*)(ptr+16*x+129*12+12) = tab[r&0xFF];
         }
         *(unsigned*)(ptr+512) = *(unsigned*)(ptr+512-4);
         *(unsigned*)(ptr+512+129*4) = *(unsigned*)(ptr+512+129*4-4);
         *(unsigned*)(ptr+512+129*8) = *(unsigned*)(ptr+512+129*8-4);
         *(unsigned*)(ptr+512+129*12) = *(unsigned*)(ptr+512+129*12-4);
         ptr += 129*16;
      }
      base += 2048;
   }
}
void get_c4_32() {
   unsigned char *base = temp.base;
   unsigned char *attr = base + 0x1800;

   unsigned char *ptr = (unsigned char*)t.bs4h;
   for (unsigned p = 0; p < 3; p++) {
      for (unsigned y = 0; y < 8; y++) {
         for (unsigned x = 0; x < 32; x++) {
            unsigned *tab = t.c32tab[*attr++];
            unsigned r =
                t.settab[base[y*0x20+x]] +
                t.settab[base[y*0x20+x+0x100]] +
                t.settab[base[y*0x20+x+0x200]] +
                t.settab[base[y*0x20+x+0x300]];
            *(unsigned*)(ptr+8*x+0) = tab[r >> 8];
            *(unsigned*)(ptr+8*x+4) = tab[r & 0xFF];
            r = t.settab[base[y*0x20+x+0x400]] +
                t.settab[base[y*0x20+x+0x500]] +
                t.settab[base[y*0x20+x+0x600]] +
                t.settab[base[y*0x20+x+0x700]];
            *(unsigned*)(ptr+8*x+65*4+0) = tab[r >> 8];
            *(unsigned*)(ptr+8*x+65*4+4) = tab[r & 0xFF];
         }
         *(unsigned*)(ptr+256) = *(unsigned*)(ptr+256-4);
         *(unsigned*)(ptr+256+65*4) = *(unsigned*)(ptr+256+65*4-4);
         ptr += 65*8;
      }
      base += 2048;
   }
}

void __fastcall _render_c2x16(u8 *dst, u32 pitch) {
   get_c2_32();
   if (!(temp.rflags & RF_128x96)) temp.rflags = (temp.rflags & ~RF_64x48) | RF_128x96, set_vidmode();
   else
   for (unsigned y = 0; y < 96; y++) {
      for (unsigned x = 0; x < 128*4; x+=4)
         *(unsigned*)(dst+x) = (*(unsigned*)((char*)t.bs2h[y]+2*x) & 0x0000FFFF) +
                               (*(unsigned*)((char*)t.bs2h[y]+2*x+4) & 0xFFFF0000);
      dst += pitch;
   }
}
void __fastcall _render_c4x16(u8 *dst, u32 pitch) {
   get_c4_32();
   if (!(temp.rflags & RF_64x48)) temp.rflags = (temp.rflags & ~RF_128x96) | RF_64x48, set_vidmode();
   else
   for (unsigned y = 0; y < 48; y++) {
      for (unsigned x = 0; x < 64*4; x+=4)
         *(unsigned*)(dst+x) = (*(unsigned*)((char*)t.bs4h[y]+2*x) & 0x0000FFFF) +
                               (*(unsigned*)((char*)t.bs4h[y]+2*x+4) & 0xFFFF0000);
      dst += pitch;
   }
}

void __fastcall render_ch_ov(u8 *dst, u32 pitch)
{
   _render_ch_ax(dst, pitch, _render_c2x16, _render_c4x16);
}

void _render_blt(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned dp)
{
   for (unsigned y = 0; y < temp.oy; y++) {
      for (unsigned x = 0; x < temp.ox; x++)
         *(unsigned*)(dst+4*x) = *(unsigned*)(src+4*x);
      dst += pitch; src += dp;
   }
}

void __fastcall  _render_c2hw(u8 *dst, u32 pitch)
{
   get_c2_32();
   if (!(temp.rflags & RF_128x96)) { temp.rflags = (temp.rflags & ~RF_64x48) | RF_128x96, set_vidmode(); return; }
   _render_blt(dst, pitch, (unsigned char*)t.bs2h, sizeof t.bs2h[0]);
}

void __fastcall  _render_c4hw(u8 *dst, u32 pitch)
{
   get_c4_32();
   if (!(temp.rflags & RF_64x48)) { temp.rflags = (temp.rflags & ~RF_128x96) | RF_64x48, set_vidmode(); return; }
   _render_blt(dst, pitch, (unsigned char*)t.bs4h, sizeof t.bs4h[0]);
}

void __fastcall render_ch_hw(u8 *dst, u32 pitch)
{
   _render_ch_ax(dst, pitch, _render_c2hw, _render_c4hw);
}

void __fastcall  _render_c2x16b(u8 *dst, u32 pitch)
{
   if (conf.updateb)
   {
       if (!conf.fast_sl) rend_frame_16d(dst, pitch);
       else rend_frame_16d1(dst, pitch*2);
   }

   get_c2_32();
   dst += pitch * (temp.b_top*2) + temp.b_left*2*sizeof(WORD);

   unsigned ll[512*2];
   unsigned y; //Alone Coder 0.36.7
   unsigned x; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < 512*4; y += 4)
      *(unsigned*)((unsigned char*)ll+y) = 0;
   for (y = 0; y < 96; y++) {
      unsigned *bp = t.bs2h[y];
      for (/*unsigned*/ x = 0; x < 128; x++) {
         unsigned *dst1 = ll + 512 + x*4;
         unsigned b0 = bp[x] & 0xFCFCFC, b4 = bp[x+1] & 0xFCFCFC;
         dst1[0] = bp[x];
         dst1[1] = (3*b0+b4)/4;
         dst1[2] = (b0+b4)/2;
         dst1[3] = (b0+3*b4)/4;
      }

      if (temp.hi15) {
#define ps1(yy)                                                              \
      for (x = 0; x < 512/2; x++) {                                          \
         unsigned r1 = ll[2*x] & 0xFCFCFC, r2 = ll[512+2*x] & 0xFCFCFC;      \
         unsigned r = (r2*yy + r1*(4-yy))/4;                                 \
         r1 = ll[2*x+1] & 0xFCFCFC, r2 = ll[512+2*x+1] & 0xFCFCFC;           \
         r2 = (r2*yy + r1*(4-yy))/4;                                         \
         *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>6) & 0x03E0) + ((r>>9) & 0x7C00) + \
            ((r2<<13) & 0x1F0000) + ((r2<<10) & 0x03E00000) + ((r2<<7) & 0x7C000000);      \
      }                                                                      \
      dst += pitch;
         ps1(0); ps1(1); ps1(2); ps1(3);
#undef ps1
      } else {
#define ps1(yy)                                                              \
      for (x = 0; x < 512/2; x++) {                                          \
         unsigned r1 = ll[2*x] & 0xFCFCFC, r2 = ll[512+2*x] & 0xFCFCFC;      \
         unsigned r = (r2*yy + r1*(4-yy))/4;                                 \
         r1 = ll[2*x+1] & 0xFCFCFC, r2 = ll[512+2*x+1] & 0xFCFCFC;           \
         r2 = (r2*yy + r1*(4-yy))/4;                                         \
         *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>5) & 0x07E0) + ((r>>8) & 0xF800) + \
            ((r2<<13) & 0x1F0000) + ((r2<<11) & 0x07E00000) + ((r2<<8) & 0xF8000000);      \
      }                                                                      \
      dst += pitch;
         ps1(0); ps1(1); ps1(2); ps1(3);
#undef ps1
      }
      for (x = 0; x < 512; x++) ll[x] = ll[x+512];
   }
}
void __fastcall  _render_c2x16bl(u8 *dst, u32 pitch)
{
   rend_frame16(dst, pitch);
   get_c2_32();
   dst += pitch * temp.b_top + temp.b_left*sizeof(WORD);

   unsigned ll[256*2];
   unsigned y; //Alone Coder 0.36.7
   unsigned x; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < 256*4; y += 4)
      *(unsigned*)((unsigned char*)ll+y) = 0;
   for (y = 0; y < 96; y++) {
      unsigned *bp = t.bs2h[y];
      for (/*unsigned*/ x = 0; x < 128; x++) {
         unsigned *dst1 = ll + 256 + x*2;
         unsigned b0 = bp[x] & 0xFEFEFE, b2 = bp[x+1] & 0xFEFEFE;
         dst1[0] = bp[x];
         dst1[1] = (b0+b2)/2;
      }

      if (temp.hi15) {
#define ps1(yy)                                                              \
      for (x = 0; x < 256/2; x++) {                                          \
         unsigned r1 = ll[2*x] & 0xFEFEFE, r2 = ll[512+2*x] & 0xFEFEFE;      \
         unsigned r = (r2*yy + r1*(2-yy))/2;                                 \
         r1 = ll[2*x+1] & 0xFEFEFE, r2 = ll[256+2*x+1] & 0xFEFEFE;           \
         r2 = (r2*yy + r1*(2-yy))/2;                                         \
         *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>6) & 0x03E0) + ((r>>9) & 0x7C00) + \
            ((r2<<13) & 0x1F0000) + ((r2<<10) & 0x03E00000) + ((r2<<7) & 0x7C000000);      \
      }                                                                      \
      dst += pitch;
         ps1(0); ps1(1);
#undef ps1
      } else {
#define ps1(yy)                                                              \
      for (x = 0; x < 256/2; x++) {                                          \
         unsigned r1 = ll[2*x] & 0xFEFEFE, r2 = ll[256+2*x] & 0xFEFEFE;      \
         unsigned r = (r2*yy + r1*(2-yy))/2;                                 \
         r1 = ll[2*x+1] & 0xFEFEFE, r2 = ll[256+2*x+1] & 0xFEFEFE;           \
         r2 = (r2*yy + r1*(2-yy))/2;                                         \
         *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>5) & 0x07E0) + ((r>>8) & 0xF800) + \
            ((r2<<13) & 0x1F0000) + ((r2<<11) & 0x07E00000) + ((r2<<8) & 0xF8000000);      \
      }                                                                      \
      dst += pitch;
         ps1(0); ps1(1);
#undef ps1
      }
      for (x = 0; x < 256; x++) ll[x] = ll[x+256];
   }
}
void __fastcall  _render_c4x16b(u8 *dst, u32 pitch)
{
   if (conf.updateb)
   {
       if (!conf.fast_sl) rend_frame_16d(dst, pitch);
       else rend_frame_16d1(dst, pitch*2);
   }

   get_c4_32();
   dst += pitch * (temp.b_top*2) + 2*temp.b_left*sizeof(WORD);

   unsigned ll[512*2];
   unsigned y; //Alone Coder 0.36.7
   unsigned x; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < 512*4; y += 4)
      *(unsigned*)(((unsigned char*)ll)+y) = 0;
   for (y = 0; y < 48; y++) {
      unsigned *bp = t.bs4h[y];
      for (/*unsigned*/ x = 0; x < 64; x++) {
         unsigned *dst1 = ll + 512 + x*8;
         unsigned b0 = bp[x] & 0xF8F8F8, b8 = bp[x+1] & 0xF8F8F8;
         dst1[0] = bp[x];
         dst1[1] = (7*b0+b8)/8;
         dst1[2] = (3*b0+b8)/4;
         dst1[3] = (5*b0+3*b8)/8;
         dst1[4] = (b0+b8)/2;
         dst1[5] = (3*b0+5*b8)/8;
         dst1[6] = (b0+3*b8)/4;
         dst1[7] = (b0+7*b8)/8;
      }
      if (temp.hi15) {
#define ps1(yy)                                                            \
         for (x = 0; x < 512/2; x++) {                                     \
            unsigned r1 = ll[2*x] & 0xF8F8F8, r2 = ll[2*x+512] & 0xF8F8F8; \
            unsigned r = (r2*yy + r1*(8-yy))/8;                            \
            r1 = ll[2*x+1] & 0xF8F8F8, r2 = ll[2*x+1+512] & 0xF8F8F8;      \
            r2 = ((r2*yy + r1*(8-yy))/8);                         \
            *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>6) & 0x03E0) + ((r>>9) & 0x7C00) +\
               ((r2<<13) & 0x1F0000) + ((r2<<10) & 0x03E00000) + ((r2<<7) & 0x7C000000);      \
         }                                                                 \
         dst += pitch;
         ps1(0); ps1(1); ps1(2); ps1(3); ps1(4); ps1(5); ps1(6); ps1(7);
#undef ps1
      } else {
#define ps1(yy)                                                            \
         for (x = 0; x < 512/2; x++) {                                     \
            unsigned r1 = ll[2*x] & 0xF8F8F8, r2 = ll[2*x+512] & 0xF8F8F8; \
            unsigned r = (r2*yy + r1*(8-yy))/8;                            \
            r1 = ll[2*x+1] & 0xF8F8F8, r2 = ll[2*x+1+512] & 0xF8F8F8;      \
            r2 = ((r2*yy + r1*(8-yy))/8);                         \
            *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>5) & 0x07E0) + ((r>>8) & 0xF800) + \
               ((r2<<13) & 0x1F0000) + ((r2<<11) & 0x07E00000) + ((r2<<8) & 0xF8000000);      \
         }                                                                 \
         dst += pitch;
      ps1(0); ps1(1); ps1(2); ps1(3); ps1(4); ps1(5); ps1(6); ps1(7);
#undef ps1
      }
      for (x = 0; x < 512; x++) ll[x] = ll[x+512];
   }
}
void __fastcall  _render_c4x16bl(u8 *dst, u32 pitch)
{
   rend_frame16(dst, pitch);
   get_c4_32();
   dst += pitch * temp.b_top + temp.b_left*sizeof(WORD);

   unsigned ll[256*2];
   unsigned y; //Alone Coder 0.36.7
   unsigned x; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < 256*4; y += 4)
      *(unsigned*)(((unsigned char*)ll)+y) = 0;
   for (y = 0; y < 48; y++) {
      unsigned *bp = t.bs4h[y];
      for (/*unsigned*/ x = 0; x < 64; x++) {
         unsigned *dst1 = ll + 256 + x*4;
         unsigned b0 = bp[x] & 0xFCFCFC, b4 = bp[x+1] & 0xFCFCFC;
         dst1[0] = bp[x];
         dst1[1] = (3*b0+b4)/4;
         dst1[2] = (b0+b4)/2;
         dst1[3] = (b0+3*b4)/4;
      }
      if (temp.hi15) {
#define ps1(yy)                                                            \
         for (x = 0; x < 256/2; x++) {                                     \
            unsigned r1 = ll[2*x] & 0xFCFCFC, r2 = ll[2*x+256] & 0xFCFCFC; \
            unsigned r = (r2*yy + r1*(4-yy))/4;                            \
            r1 = ll[2*x+1] & 0xFCFCFC, r2 = ll[2*x+1+256] & 0xFCFCFC;      \
            r2 = ((r2*yy + r1*(4-yy))/4);                         \
            *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>6) & 0x03E0) + ((r>>9) & 0x7C00) +\
               ((r2<<13) & 0x1F0000) + ((r2<<10) & 0x03E00000) + ((r2<<7) & 0x7C000000);      \
         }                                                                 \
         dst += pitch;
         ps1(0); ps1(1); ps1(2); ps1(3);
#undef ps1
      } else {
#define ps1(yy)                                                            \
         for (x = 0; x < 256/2; x++) {                                     \
            unsigned r1 = ll[2*x] & 0xFCFCFC, r2 = ll[2*x+256] & 0xFCFCFC; \
            unsigned r = (r2*yy + r1*(4-yy))/4;                            \
            r1 = ll[2*x+1] & 0xFCFCFC, r2 = ll[2*x+1+256] & 0xFCFCFC;      \
            r2 = ((r2*yy + r1*(4-yy))/4);                         \
            *(unsigned*)(dst+4*x) = ((r>>3) & 0x1F) + ((r>>5) & 0x07E0) + ((r>>8) & 0xF800) + \
               ((r2<<13) & 0x1F0000) + ((r2<<11) & 0x07E00000) + ((r2<<8) & 0xF8000000);      \
         }                                                                 \
         dst += pitch;
      ps1(0); ps1(1); ps1(2); ps1(3);
#undef ps1
      }
      for (x = 0; x < 256; x++) ll[x] = ll[x+256];
   }
}

void __fastcall render_c16bl(u8 *dst, u32 pitch)
{
   _render_ch_ax(dst, pitch, _render_c2x16bl, _render_c4x16bl);
}

void __fastcall render_c16b(u8 *dst, u32 pitch)
{
   _render_ch_ax(dst, pitch, _render_c2x16b, _render_c4x16b);
}

void __fastcall render_c4x32b(u8 *dst, u32 pitch)
{
   if (!conf.fast_sl) rend_frame_32d(dst, pitch);
   else rend_frame_32d1(dst, pitch*2);

   get_c4_32();
   dst += pitch * (temp.b_top*2) + 2*temp.b_left*sizeof(DWORD);

   unsigned ll[512*2];
   unsigned y; //Alone Coder 0.36.7
   unsigned x; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < 512*4; y += 4)
      *(unsigned*)(((unsigned char*)ll)+y) = 0;
   #define inter(a,b,c) ((unsigned char)((a*(8-c)+b*c)>>3))
   for (y = 0; y < 48; y++) {
      unsigned *bp = t.bs4h[y];
      for (/*unsigned*/ x = 0; x < 64; x++) {
         unsigned char *dst1 = (unsigned char*)ll + 512*4 + x*8*4;
         unsigned char *src = (unsigned char*)&bp[x];
         #define inter2(n) dst1[n*4]=inter(src[0],src[4],n); dst1[n*4+1]=inter(src[1],src[5],n); dst1[n*4+2]=inter(src[2],src[6],n);
         inter2(0);inter2(1);inter2(2);inter2(3);
         inter2(4);inter2(5);inter2(6);inter2(7);
         #undef inter2
      }
      unsigned char *l1 = (unsigned char*)ll, *l2 = (unsigned char*)(ll+512);
      #define ps1(yy)                                                   \
        for (x = 0; x < 512; x++) {                                     \
           dst[4*x+0] = inter(l1[4*x+0], l2[4*x+0], yy);                \
           dst[4*x+1] = inter(l1[4*x+1], l2[4*x+1], yy);                \
           dst[4*x+2] = inter(l1[4*x+2], l2[4*x+2], yy);                \
        } dst += pitch;
      ps1(0);ps1(1);ps1(2);ps1(3);ps1(4);ps1(5);ps1(6);ps1(7);
      #undef ps1
      for (x = 0; x < 512; x++) ll[x] = ll[x+512];
   }
   #undef inter
}
