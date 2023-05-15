#include "std.h"
#include "emul.h"
#include "vars.h"

#if 0
void _render_black(u8 *dst, unsigned pitch)
{
   unsigned dx = ((temp.rflags & RF_OVR) ? temp.ox : temp.rsx)*temp.obpp/8;
   unsigned filler = (temp.rflags & RF_OVR)? WORD4(0,0x80,0,0x80) : 0;
   for (unsigned y = (temp.rflags & (RF_OVR|RF_CLIP)) ? temp.oy : temp.rsy; y; y--) {
      for (unsigned x = 0; x < dx; x += 4)
         *(unsigned*)(dst+x) = filler;
      dst += pitch;
   }
}

void rend_frame8(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line8(dst, src, t.sctab8[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*8;
   for (y = 0; y < 192; y++) {
      line8(dst, src, t.sctab8[0]);
      line8(dst+offs, src + d1, t.sctab8[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line8(dst, src, t.sctab8[0]); dst += pitch;
      src += delta;
   }
}

void rend_frame_8d1(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*8;
   for (y = 0; y < 192; y++) {
      line8d(dst, src, t.sctab8d[0]);
      line8d(dst+offs, src + d1, t.sctab8d[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
}

void rend_frame_8d(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      line8d(dst, src, t.sctab8d[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*8;
   for (y = 0; y < 192; y++) {
      line8d(dst, src, t.sctab8d[0]);
      line8d(dst+offs, src + d1, t.sctab8d[0]);
      dst += pitch,
      line8d(dst, src, t.sctab8d[1]);
      line8d(dst+offs, src + d1, t.sctab8d[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      line8d(dst, src, t.sctab8d[1]); dst += pitch;
      src += delta;
   }
}

void rend_frame16(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line16(dst, src, t.sctab16[0]);
      dst += pitch; src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*8;
   for (y = 0; y < 192; y++) {
      line16(dst, src, t.sctab16[0]);
      line16(dst+offs, src + d1, t.sctab16[0]);
      dst += pitch, src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line16(dst, src, t.sctab16[0]);
      dst += pitch; src += delta;
   }
}

void rend_frame_16d1(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*16;
   for (y = 0; y < 192; y++) {
      line16d(dst, src, t.sctab16d[0]);
      line16d(dst+offs, src + d1, t.sctab16d[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
}

void rend_frame_16d(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      line16d(dst, src, t.sctab16d[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*16;
   for (y = 0; y < 192; y++) {
      line16d(dst, src, t.sctab16d[0]);
      line16d(dst+offs, src + d1, t.sctab16d[0]);
      dst += pitch,
      line16d(dst, src, t.sctab16d[1]);
      line16d(dst+offs, src + d1, t.sctab16d[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      line16d(dst, src, t.sctab16d[1]); dst += pitch;
      src += delta;
   }
}

void rend_frame32(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line32(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*32;
   for (y = 0; y < 192; y++) {
      line32(dst, src, t.sctab32[0]);
      line32(dst+offs, src + d1, t.sctab32[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line32(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}

void rend_frame_32d1(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*32;
   for (y = 0; y < 192; y++) {
      line32d(dst, src, t.sctab32[0]);
      line32d(dst+offs, src + d1, t.sctab32[0]);
      dst += pitch; src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}

void rend_frame_32d(u8 *dst, unsigned pitch)
{
   if (!conf.updateb) return;
   u8 *src = rbuf; unsigned scx = temp.scx, delta = scx/4;
   unsigned y; //Alone Coder 0.36.7
   for (/*unsigned*/ y = 0; y < temp.b_top; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      line32d(dst, src, t.sctab32[1]); dst += pitch;
      src += delta;
   }
   temp.scx = (scx-256)/2;
   unsigned d1 = (temp.b_left+256)/4, offs = d1*32;
   for (y = 0; y < 192; y++) {
      line32d(dst, src, t.sctab32[0]);
      line32d(dst+offs, src + d1, t.sctab32[0]);
      dst += pitch,
      line32d(dst, src, t.sctab32[1]);
      line32d(dst+offs, src + d1, t.sctab32[1]);
      dst += pitch,
      src += delta;
   }
   temp.scx = scx;
   for (y = 0; y < temp.b_bottom; y++) {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      line32d(dst, src, t.sctab32[1]); dst += pitch;
      src += delta;
   }
}
#endif
void gdi_frame()
{
   RECT rc, r0;
   GetClientRect(wnd, &rc);
   HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
   if (temp.oy < temp.gdy)
   {
      r0.top = rc.top; r0.left = rc.left; r0.right = rc.right; r0.bottom = temp.gy;
      FillRect(temp.gdidc, &r0, black);
      r0.top = temp.gy+temp.oy; r0.bottom = rc.bottom;
      FillRect(temp.gdidc, &r0, black);
   }
   if (temp.ox < temp.gdx)
   {
      r0.top = rc.top, r0.bottom = rc.bottom, r0.left = rc.left, r0.right = temp.gx;
      FillRect(temp.gdidc, &r0, black);
      r0.left = temp.gx+temp.ox, r0.right = rc.right;
      FillRect(temp.gdidc, &r0, black);
   }
   memset(gdibuf, 0, temp.ox*temp.oy*temp.obpp/8);
}
