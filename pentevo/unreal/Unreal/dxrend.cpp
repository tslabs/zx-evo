#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dxrend.h"
#include "dxrcopy.h"
#include "dxr_512.h"
#include "dxr_4bpp.h"
#include "dxr_prof.h"
#include "dxr_atm.h"
#include "draw.h"
#include "util.h"

void rend_1x_32(u8 *dst, unsigned pitch)
{
	RGB32 *src = (RGB32*)vbuf;
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		RGB32 *src1 = src;
		for (u32 j=0; j<(pitch/4); j++)
		{
			RGB32 p1 = *src1++; RGB32 p2 = *src1++;
			*dst++ = (p1.b + p2.b) >> 1;
			*dst++ = (p1.g + p2.g) >> 1;
			*dst++ = (p1.r + p2.r) >> 1;
			*dst++;
		}
		src += VID_WIDTH * 2;
	}
}

void __fastcall render_1x(unsigned char *dst, unsigned pitch)
{
	if (conf.noflic)    {
		if (temp.obpp == 8)  { rend_copy8_nf(dst, pitch); }
		if (temp.obpp == 16) { rend_copy16_nf(dst, pitch); }
		if (temp.obpp == 32) { rend_copy32_nf(dst, pitch); }
		memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
		return;

	} else {
		if (temp.obpp == 8)  { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_1x_32(dst, pitch); return; }
	}

/*

   if (comp.pEFF7 & EFF7_4BPP)
   {
       rend_p4bpp_small(dst, pitch);
       return;
   }

   if (conf.mem_model == MM_ATM450)
   {
       rend_atm_1_small(dst, pitch);
       return;
   }

   if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3)
   {
       rend_atm_2_small(dst, pitch);
       return;
   }

   rend_small(dst, pitch);
*/
}

/*
void rend_dbl(unsigned char *dst, unsigned pitch)
{
   if (temp.oy > temp.scy && conf.fast_sl)
       pitch *= 2;

   if (conf.noflic)
   {
      if (temp.obpp == 8)
      {
          if (conf.fast_sl)
              rend_copy8d1_nf (dst, pitch);
          else
              rend_copy8d_nf (dst, pitch);
      }
      else if (temp.obpp == 16)
      {
          if (conf.fast_sl)
              rend_copy16d1_nf(dst, pitch);
          else
              rend_copy16d_nf(dst, pitch);
      }
      else if (temp.obpp == 32)
      {
          if (conf.fast_sl)
              rend_copy32d1_nf(dst, pitch);
          else
              rend_copy32d_nf(dst, pitch);
      }

      memcpy(rbuf_s, rbuf, temp.scy * temp.scx / 4);
   }
   else
   {
      if (temp.obpp == 8)
      {
          if (conf.fast_sl)
              rend_copy8d1 (dst, pitch);
          else
              rend_copy8d (dst, pitch);
          return;
      }
      if (temp.obpp == 16)
      {
          if (conf.fast_sl)
              rend_copy16d1(dst, pitch);
          else
              rend_copy16d(dst, pitch);
          return;
      }
      if (temp.obpp == 32)
      {
          if (conf.fast_sl)
              rend_copy32d1(dst, pitch);
          else
              rend_copy32d(dst, pitch);
          return;
      }
   }
}
*/

void rend_2x_32(u8 *dst, unsigned pitch)
{
	u32 *src = vbuf;
	src += conf.framex * 2;
	src += conf.framey * VID_WIDTH * 2;
	for (u32 i=0; i<conf.frameysize; i++)
	{
		memcpy (dst, src, pitch); dst += pitch;
		memcpy (dst, src, pitch); dst += pitch;
		src += VID_WIDTH * 2;
	}
}

void __fastcall render_2x(unsigned char *dst, unsigned pitch)
{

   if (conf.noflic)    {
      if (temp.obpp == 8)  { rend_copy8_nf (dst, pitch); }
      if (temp.obpp == 16) { rend_copy16_nf(dst, pitch); }
      if (temp.obpp == 32) { rend_copy32_nf(dst, pitch); }
      memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
      return;

   } else {
    if (temp.obpp == 8)  { rend_2x_32(dst, pitch); return; }
    if (temp.obpp == 16) { rend_2x_32(dst, pitch); return; }
    if (temp.obpp == 32) { rend_2x_32(dst, pitch); return; }
   }

/*

   #ifdef MOD_VID_VD
   if ((comp.pVD & 8) && temp.obpp == 8)
   {
       rend_vd8dbl(dst, pitch);
       return;
   }
   #endif

   // todo: add ini option to show zx-screen with palette or with MC


	{
		if (comp.pEFF7 & EFF7_512)
		{
			rend_512(dst, pitch);
			return;
		}

		if (comp.pEFF7 & EFF7_4BPP)
		{
			rend_p4bpp(dst, pitch);
			return;
		}
   }

   if ((comp.pDFFD & 0x80) && conf.mem_model == MM_PROFI)
   {
       rend_profi(dst, pitch);
       return;
   }

   if (conf.mem_model == MM_ATM450)
   {
       rend_atm_1(dst, pitch);
       return;
   }

   if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3)
   {
       rend_atm_2(dst, pitch);
       return;
   }

   rend_2x(dst, pitch);
   */
}

void rend_3x_32(u8 *dst, unsigned pitch)
{
	RGB32 *src = (RGB32*)vbuf;
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		RGB32 *src1 = src;
		u8 *dst1 = dst;
		for (u32 j=0; j<(pitch/12); j++)
		{
			RGB32 p1 = *src1++; RGB32 p2 = *src1++;
			*dst++ = p1.b;
			*dst++ = p1.g;
			*dst++ = p1.r;
			*dst++;
			*dst++ = (p1.b + p2.b) >> 1;
			*dst++ = (p1.g + p2.g) >> 1;
			*dst++ = (p1.r + p2.r) >> 1;
			*dst++;
			*dst++ = p2.b;
			*dst++ = p2.g;
			*dst++ = p2.r;
			*dst++;
		}
		memcpy (dst, dst1, pitch); dst += pitch;
		memcpy (dst, dst1, pitch); dst += pitch;
		src += VID_WIDTH * 2;
	}
}

void __fastcall render_3x(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic) {
		if (temp.obpp == 8)  rend_copy8t_nf (dst, pitch);
		if (temp.obpp == 16) rend_copy16t_nf(dst, pitch);
		if (temp.obpp == 32) rend_copy32t_nf(dst, pitch);
		memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
   } else {
		if (temp.obpp == 8)  { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_3x_32(dst, pitch); return; }
   }
}

void __fastcall render_4x(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic) {
      if (temp.obpp == 8)  rend_copy8q_nf (dst, pitch);
      if (temp.obpp == 16) rend_copy16q_nf(dst, pitch);
      if (temp.obpp == 32) rend_copy32q_nf(dst, pitch);
      memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
   } else {
      if (temp.obpp == 8)  { rend_copy8q (dst, pitch); return; }
      if (temp.obpp == 16) { rend_copy16q(dst, pitch); return; }
      if (temp.obpp == 32) { rend_copy32q(dst, pitch); return; }
   }
}


void __fastcall render_scale(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf;
   unsigned dx = temp.scx / 4;
   unsigned char buf[MAX_WIDTH*2];
   unsigned x; //Alone Coder 0.36.7
   for (unsigned y = 0; y < temp.scy-1; y++)
   {
      for (x = 0; x < dx; x += 2)
      {
         unsigned xx = (t.dbl[src[x]] << 16) + t.dbl[src[x+2]];
         unsigned yy = (t.dbl[src[x+dx]] << 16) + t.dbl[src[x+dx+2]];
         unsigned x1 = xx | (yy & ((xx>>1) | (xx<<1)));
         unsigned *tab0 = t.sctab8[0] + (src[x+1] << 4);
         *(unsigned*)(dst+x*8+ 0)   = tab0[(x1>>28) & 0x0F];
         *(unsigned*)(dst+x*8+ 4)   = tab0[(x1>>24) & 0x0F];
         *(unsigned*)(dst+x*8+ 8)   = tab0[(x1>>20) & 0x0F];
         *(unsigned*)(dst+x*8+12)   = tab0[(x1>>16) & 0x0F];
         unsigned *tab1 = t.sctab8[0] + src[x+3];
         *(unsigned*)(dst+x*8+16)   = tab1[(x1>>12) & 0x0F];
         *(unsigned*)(dst+x*8+20)   = tab1[(x1>> 8) & 0x0F];
         *(unsigned*)(dst+x*8+24)   = tab1[(x1>> 4) & 0x0F];
         *(unsigned*)(dst+x*8+28)   = tab1[(x1>> 0) & 0x0F];
         x1 = yy | (xx & ((yy>>1) | (yy<<1)));
         *(unsigned*)(buf+x*8+ 0)   = tab0[(x1>>28) & 0x0F];
         *(unsigned*)(buf+x*8+ 4)   = tab0[(x1>>24) & 0x0F];
         *(unsigned*)(buf+x*8+ 8)   = tab0[(x1>>20) & 0x0F];
         *(unsigned*)(buf+x*8+12)   = tab0[(x1>>16) & 0x0F];
         *(unsigned*)(buf+x*8+16)   = tab1[(x1>>12) & 0x0F];
         *(unsigned*)(buf+x*8+20)   = tab1[(x1>> 8) & 0x0F];
         *(unsigned*)(buf+x*8+24)   = tab1[(x1>> 4) & 0x0F];
         *(unsigned*)(buf+x*8+28)   = tab1[(x1>> 0) & 0x0F];
      }
      dst += pitch;
      for (x = 0; x < temp.ox; x += 4)
          *(unsigned*)(dst+x) = *(unsigned*)(buf+x);
      src += dx; dst += pitch;
   }
}

static u64 mask49 = 0x4949494949494949;
static u64 mask92 = 0x9292929292929292;

static void /*__declspec(naked)*/ __fastcall _bil_line1(unsigned char *dst, unsigned char *src)
{
    for (unsigned i = 0; i < temp.scx; i += 2)
    {
       dst[i] = src[i];
       dst[i+1] = ((src[i] + src[i+1]) >> 1);
    }
/*
   __asm {

      push ebx
      push edi
      push ebp

      mov  ebp, [temp.scx]
      xor  eax, eax
      xor  ebx, ebx // ebx - prev. pixel
      shr ebp,1

l1:
      mov  al, [edx]
      xadd eax, ebx
      shr  eax, 1
      mov  [ecx+1], bl
      mov  [ecx], al
      mov  al, [edx+1]
      add  ecx, 4
      xadd eax, ebx
      add  edx, 2
      shr  eax, 1
      mov  [ecx-1], bl
      dec  ebp
      mov  [ecx-2], al
      jnz l1

      pop ebp
      pop edi
      pop ebx
      retn
   }
*/
}

static void /*__declspec(naked)*/ __fastcall _bil_line2(unsigned char *dst, unsigned char *s1)
{
      u32 *s = (u32 *)s1;
      u32 *d = (u32 *)dst;

      for (unsigned j = 0; j < temp.ox/4; j++)
      {
          u32 a = s[j];
          u32 b = s[j+2*MAX_WIDTH/4];
          u32 x = a & b;
          u32 y = (a ^ b) >> 1;
          u32 z = a | b;
          u32 n = x << 1;
          u32 v1 = x ^ y;
          v1 &= 0x49494949;
          u32 v2 = z & n;
          v2 |= x;
          v2 &= 0x92929292;

          d[j] = v1 | v2;
      }

/*
   __asm {

      mov  eax, [temp.ox]
      movq mm2, [mask49]
      movq mm3, [mask92]
      shr  eax, 3

m2:   movq  mm0, [edx]
      movq  mm1, [edx+MAX_WIDTH*2]
      movq  mm4, mm0
      movq  mm5, mm0
      pand  mm4, mm1    // mm4 = a & b
      pxor  mm5, mm1    // mm5 = a ^ b
      movq  mm6, mm0
      psrlq mm5, 1      // mm5 = (a ^ b) >> 1
      por   mm6, mm1    // mm6 = a | b
      movq  mm7, mm4
      pxor  mm5, mm4    // mm5 = (a & b) ^ ((a ^ b) >> 1)
      psllq mm7, 1      // mm7 = (a & b) << 1
      pand  mm5, mm2    // mm5 = 0x49494949 & ((a & b) ^ ((a ^ b) >> 1))
      pand  mm7, mm6    // mm7 = (a|b) & ((a & b) << 1)
      por   mm7, mm4    // mm7 = (a&b) | ((a|b)&((a&b)<<1))
      add   ecx, 8
      pand  mm7, mm3    // mm7 &= 0x92929292
      add  edx, 8
      por   mm7, mm5
      dec  eax
      movq [ecx-8], mm7
      jnz  m2

      retn
   }
*/
}

void __fastcall render_bil(unsigned char *dst, unsigned pitch)
{
   render_1x(snbuf, MAX_WIDTH);

   unsigned char *src = snbuf;
   unsigned char ATTR_ALIGN(16) l1[MAX_WIDTH*4];
   #define l2 (l1+MAX_WIDTH*2)
   _bil_line1(l1, src); src += MAX_WIDTH;
   memcpy(dst, l1, temp.ox);
   dst += pitch;

   for (unsigned i = temp.scy/2-1; i; i--)
   {
      _bil_line1(l2, src); src += MAX_WIDTH;
      _bil_line2(dst, l1); dst += pitch;
      memcpy(dst, l2, temp.ox);
      dst += pitch;

      _bil_line1(l1, src); src += MAX_WIDTH;
      _bil_line2(dst, l1); dst += pitch;
      memcpy(dst, l1, temp.ox);
      dst += pitch;
   }
   _bil_line1(l2, src); src += MAX_WIDTH;
   _bil_line2(dst, l1); dst += pitch;
   memcpy(dst, l2, temp.ox);
   dst += pitch;
   memcpy(dst, l2, temp.ox);
   #undef l2

//   _mm_empty();
}

void __fastcall render_tv(unsigned char *dst, unsigned pitch)
{
// ripped from ccs and *highly* simplified and optimized

   unsigned char midbuf[MAX_WIDTH*2];
   unsigned char line[MAX_WIDTH*2+4*2], line2[MAX_WIDTH*2];

   unsigned j; //Alone Coder 0.36.7
   for (/*unsigned*/ j = 0; j < MAX_WIDTH/2; j++)
      *(unsigned*)(midbuf+j*4) = WORD4(0,0x80,0,0x80);

   unsigned char *src = rbuf; unsigned delta = temp.scx/4;

   for (unsigned i = temp.scy; i; i--) {
      *(unsigned*)line = *(unsigned*)(line+4) = WORD4(0,0x80,0,0x80);

      if (conf.noflic) line16_nf(line+8, src, t.sctab16_nf[0]);
      else line16(line+8, src, t.sctab16[0]);

      src += delta;

      for (j = 0; j < temp.scx; j++) {

         unsigned Y = line[j*2+8]*9+
                      line[j*2-2+8]*4+
                      line[j*2-4+8]*2+
                      line[j*2-8+8];
/*
         unsigned U = line[j*2+8+1]*12 +
                      line[j*2-2+8+1]*2+
                      line[j*2-4+8+1]+
                      line[j*2-8+8+1];
*/
         line2[j*2] = Y>>4;
//         line2[j*2+1] = U>>4;
         line2[j*2+1] = line[j*2+9];
      }
      // there must be only fixed length fader buffer
      for (j = 0; j < temp.scx/2; j++) {
         *(unsigned*)(midbuf+j*4) = *(unsigned*)(dst + j*4) =
         ((*(unsigned*)(midbuf+j*4) & 0xFEFEFEFE)/2 + (*(unsigned*)(line2+j*4) & 0xFEFEFEFE)/2);
      }
      dst += pitch;
   }
   if (conf.noflic) memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
}

