#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "dxrcopy.h"
#include "dxr_atmf.h"

// small
void rend_atmframe8(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb)
       return;
   unsigned char *src = rbuf;
   unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y;
   for (y = 0; y < top; y++)
   {
      line8(dst, src, t.sctab8[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx;
   unsigned scr_offs = r_start*2;
   unsigned atr_offs = r_start/4;
   for (y = 0; y < 200; y++)
   {
      line8(dst, src, t.sctab8[0]);
      line8(dst+scr_offs, src + atr_offs, t.sctab8[0]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++)
   {
      line8(dst, src, t.sctab8[0]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe16(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb)
       return;
   unsigned char *src = rbuf;
   unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y;
   for (y = 0; y < top; y++)
   {
      line16(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx;
   unsigned scr_offs = r_start*4;
   unsigned atr_offs = r_start/4;

   for (y = 0; y < 200; y++)
   {
      line16(dst, src, t.sctab16[0]);
      line16(dst+scr_offs, src + atr_offs, t.sctab16d[0]);
      dst += pitch,
      src += delta;
   }

   temp.scx = scx;
   for (y = 0; y < top; y++)
   {
      line16(dst, src, t.sctab16[0]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe32(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb)
       return;
   unsigned char *src = rbuf;
   unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y;
   for (y = 0; y < top; y++)
   {
      line32(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx;
   unsigned scr_offs = r_start*8;
   unsigned atr_offs = r_start/4;
   for (y = 0; y < 200; y++)
   {
      line32(dst, src, t.sctab32[0]);
      line32(dst+scr_offs, src + atr_offs, t.sctab32[0]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++)
   {
      line32(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}


// double
void rend_atmframe_8d1(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*2,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line8d(dst, src, t.sctab8d[0]);
      line8d(dst+scr_offs, src + atr_offs, t.sctab8d[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe_8d(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      line8d(dst, src, t.sctab8d[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*2,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line8d(dst, src, t.sctab8d[0]);
      line8d(dst+scr_offs, src + atr_offs, t.sctab8d[0]);
      dst += pitch,
      line8d(dst, src, t.sctab8d[1]);
      line8d(dst+scr_offs, src + atr_offs, t.sctab8d[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      line8d(dst, src, t.sctab8d[1]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe_16d1(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*4,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line16d(dst, src, t.sctab16d[0]);
      line16d(dst+scr_offs, src + atr_offs, t.sctab16d[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe_16d(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      line16d(dst, src, t.sctab16d[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*4,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line16d(dst, src, t.sctab16d[0]);
      line16d(dst+scr_offs, src + atr_offs, t.sctab16d[0]);
      dst += pitch,
      line16d(dst, src, t.sctab16d[1]);
      line16d(dst+scr_offs, src + atr_offs, t.sctab16d[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      line16d(dst, src, t.sctab16d[1]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe_32d1(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*8,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line32d(dst, src, t.sctab32[0]);
      line32d(dst+scr_offs, src + atr_offs, t.sctab32[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}

void rend_atmframe_32d(unsigned char *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   unsigned char *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned top = (temp.scy-200)/2;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      line32d(dst, src, t.sctab32[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-320)/2;
   unsigned r_start = scx - temp.scx,
            scr_offs = r_start*8,
            atr_offs = r_start/4;
   for (y = 0; y < 200; y++) {
      line32d(dst, src, t.sctab32[0]);
      line32d(dst+scr_offs, src + atr_offs, t.sctab32[0]);
      dst += pitch,
      line32d(dst, src, t.sctab32[1]);
      line32d(dst+scr_offs, src + atr_offs, t.sctab32[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      line32d(dst, src, t.sctab32[1]); dst += pitch;
      src += delta;
   }
}
