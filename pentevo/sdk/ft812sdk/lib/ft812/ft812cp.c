
#include "ft812types.h"
#include "ft812.h"
#include "ft812lib.h"

void ft_SetBitmap(u32 source, u16 fmt, u16 w, u16 h)
{
  ft_ccmd(FT_CCMD_SETBITMAP);
  ft_ccmd(source);
  ft_ccmd((((u32)w << 16) | (fmt & 0xffff)));
  ft_ccmd(h);
}

void ft_SetScratch(u32 handle)
{
  ft_ccmd(FT_CCMD_SETSCRATCH);
  ft_ccmd(handle);
}

void ft_Text(s16 x, s16 y, s16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_TEXT);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Number(s16 x, s16 y, s16 font, u16 options, s32 n)
{
  ft_ccmd(FT_CCMD_NUMBER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_ccmd(n);
}

void ft_LoadIdentity()
{
  ft_ccmd(FT_CCMD_LOADIDENTITY);
}

void ft_Toggle(s16 x, s16 y, s16 w, s16 font, u16 options, u16 state, const char *s)
{
  ft_ccmd(FT_CCMD_TOGGLE);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)font << 16) | (w & 0xffff)));
  ft_ccmd((((u32)state << 16) | options));
  ft_cstr(s);
}

void ft_Gauge(s16 x, s16 y, s16 r, u16 options, u16 major, u16 minor, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_GAUGE);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd((((u32)minor << 16) | (major & 0xffff)));
  ft_ccmd((((u32)range << 16) | (val & 0xffff)));
}

void ft_RegRead(u32 ptr, u32 result)
{
  ft_ccmd(FT_CCMD_REGREAD);
  ft_ccmd(ptr);
  ft_ccmd(result);
}

void ft_VideoStart()
{
  ft_ccmd(FT_CCMD_VIDEOSTART);
}

void ft_GetProps(u32 ptr, u32 w, u32 h)
{
  ft_ccmd(FT_CCMD_GETPROPS);
  ft_ccmd(ptr);
  ft_ccmd(w);
  ft_ccmd(h);
}

void ft_Memcpy(u32 dest, u32 src, u32 num)
{
  ft_ccmd(FT_CCMD_MEMCPY);
  ft_ccmd(dest);
  ft_ccmd(src);
  ft_ccmd(num);
}

void ft_Spinner(s16 x, s16 y, u16 style, u16 scale)
{
  ft_ccmd(FT_CCMD_SPINNER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)scale << 16) | (style & 0xffff)));
}

void ft_BgColor(u32 c)
{
  ft_ccmd(FT_CCMD_BGCOLOR);
  ft_ccmd(c);
}

void ft_Swap()
{
  ft_ccmd(FT_CCMD_SWAP);
}

void ft_Inflate(u32 ptr)
{
  ft_ccmd(FT_CCMD_INFLATE);
  ft_ccmd(ptr);
}

void ft_Translate(s32 tx, s32 ty)
{
  ft_ccmd(FT_CCMD_TRANSLATE);
  ft_ccmd(tx);
  ft_ccmd(ty);
}

void ft_Stop()
{
  ft_ccmd(FT_CCMD_STOP);
}

void ft_SetBase(u32 base)
{
  ft_ccmd(FT_CCMD_SETBASE);
  ft_ccmd(base);
}

void ft_Slider(s16 x, s16 y, s16 w, s16 h, u16 options, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_SLIDER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd(range);
}

void ft_VideoFrame(u32 dst, u32 ptr)
{
  ft_ccmd(FT_CCMD_VIDEOFRAME);
  ft_ccmd(dst);
  ft_ccmd(ptr);
}

void ft_TouchTransform(s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, s32 tx0, s32 ty0, s32 tx1, s32 ty1, s32 tx2, s32 ty2, u16 result)
{
  ft_ccmd(FT_CCMD_TOUCH_TRANSFORM);
  ft_ccmd(x0);
  ft_ccmd(y0);
  ft_ccmd(x1);
  ft_ccmd(y1);
  ft_ccmd(x2);
  ft_ccmd(y2);
  ft_ccmd(tx0);
  ft_ccmd(ty0);
  ft_ccmd(tx1);
  ft_ccmd(ty1);
  ft_ccmd(tx2);
  ft_ccmd(ty2);
  ft_ccmd(result);
}

void ft_Interrupt(u32 ms)
{
  ft_ccmd(FT_CCMD_INTERRUPT);
  ft_ccmd(ms);
}

void ft_FgColor(u32 c)
{
  ft_ccmd(FT_CCMD_FGCOLOR);
  ft_ccmd(c);
}

void ft_Rotate(s32 a)
{
  ft_ccmd(FT_CCMD_ROTATE);
  ft_ccmd(a);
}

void ft_Button(s16 x, s16 y, s16 w, s16 h, s16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_BUTTON);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_MemWrite(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_MEMWRITE);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_Scrollbar(s16 x, s16 y, s16 w, s16 h, u16 options, u16 val, u16 size, u16 range)
{
  ft_ccmd(FT_CCMD_SCROLLBAR);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd((((u32)range << 16) | (size & 0xffff)));
}

void ft_GetMatrix(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f)
{
  ft_ccmd(FT_CCMD_GETMATRIX);
  ft_ccmd(a);
  ft_ccmd(b);
  ft_ccmd(c);
  ft_ccmd(d);
  ft_ccmd(e);
  ft_ccmd(f);
}

void ft_Sketch(s16 x, s16 y, u16 w, u16 h, u32 ptr, u16 format)
{
  ft_ccmd(FT_CCMD_SKETCH);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd(ptr);
  ft_ccmd(format);
}

void ft_RomFont(u32 font, u32 romslot)
{
  ft_ccmd(FT_CCMD_ROMFONT);
  ft_ccmd(font);
  ft_ccmd(romslot);
}

void ft_PlayVideo(u32 options)
{
  ft_ccmd(FT_CCMD_PLAYVIDEO);
  ft_ccmd(options);
}

void ft_MemSet(u32 ptr, u32 value, u32 num)
{
  ft_ccmd(FT_CCMD_MEMSET);
  ft_ccmd(ptr);
  ft_ccmd(value);
  ft_ccmd(num);
}

void ft_GradColor(u32 c)
{
  ft_ccmd(FT_CCMD_GRADCOLOR);
  ft_ccmd(c);
}

void ft_Sync()
{
  ft_ccmd(FT_CCMD_SYNC);
}

void ft_BitmapTransform(s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, s32 tx0, s32 ty0, s32 tx1, s32 ty1, s32 tx2, s32 ty2, u16 result)
{
  ft_ccmd(FT_CCMD_BITMAP_TRANSFORM);
  ft_ccmd(x0);
  ft_ccmd(y0);
  ft_ccmd(x1);
  ft_ccmd(y1);
  ft_ccmd(x2);
  ft_ccmd(y2);
  ft_ccmd(tx0);
  ft_ccmd(ty0);
  ft_ccmd(tx1);
  ft_ccmd(ty1);
  ft_ccmd(tx2);
  ft_ccmd(ty2);
  ft_ccmd(result);
}

void ft_Calibrate(u32 result)
{
  ft_ccmd(FT_CCMD_CALIBRATE);
  ft_ccmd(result);
}

void ft_SetFont(u32 font, u32 ptr)
{
  ft_ccmd(FT_CCMD_SETFONT);
  ft_ccmd(font);
  ft_ccmd(ptr);
}

void ft_Logo()
{
  ft_ccmd(FT_CCMD_LOGO);
}

void ft_Append(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_APPEND);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_MemZero(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_MEMZERO);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_Scale(s32 sx, s32 sy)
{
  ft_ccmd(FT_CCMD_SCALE);
  ft_ccmd(sx);
  ft_ccmd(sy);
}

void ft_Clock(s16 x, s16 y, s16 r, u16 options, u16 h, u16 m, u16 s, u16 ms)
{
  ft_ccmd(FT_CCMD_CLOCK);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd((((u32)m << 16) | (h & 0xffff)));
  ft_ccmd((((u32)ms << 16) | (s & 0xffff)));
}

void ft_Gradient(s16 x0, s16 y0, u32 rgb0, s16 x1, s16 y1, u32 rgb1)
{
  ft_ccmd(FT_CCMD_GRADIENT);
  ft_ccmd((((u32)y0 << 16) | (x0 & 0xffff)));
  ft_ccmd(rgb0);
  ft_ccmd((((u32)y1 << 16) | (x1 & 0xffff)));
  ft_ccmd(rgb1);
}

void ft_SetMatrix()
{
  ft_ccmd(FT_CCMD_SETMATRIX);
}

void ft_Track(s16 x, s16 y, s16 w, s16 h, s16 tag)
{
  ft_ccmd(FT_CCMD_TRACK);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd(tag);
}

void ft_Int_RAMShared(u32 ptr)
{
  ft_ccmd(FT_CCMD_INT_RAMSHARED);
  ft_ccmd(ptr);
}

void ft_Int_SWLoadImage(u32 ptr, u32 options)
{
  ft_ccmd(FT_CCMD_INT_SWLOADIMAGE);
  ft_ccmd(ptr);
  ft_ccmd(options);
}

void ft_GetPtr(u32 result)
{
  ft_ccmd(FT_CCMD_GETPTR);
  ft_ccmd(result);
}

void ft_Progress(s16 x, s16 y, s16 w, s16 h, u16 options, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_PROGRESS);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd(range);
}

void ft_ColdStart()
{
  ft_ccmd(FT_CCMD_COLDSTART);
}

void ft_MediaFifo(u32 ptr, u32 size)
{
  ft_ccmd(FT_CCMD_MEDIAFIFO);
  ft_ccmd(ptr);
  ft_ccmd(size);
}

void ft_Keys(s16 x, s16 y, s16 w, s16 h, s16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_KEYS);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Dial(s16 x, s16 y, s16 r, u16 options, u16 val)
{
  ft_ccmd(FT_CCMD_DIAL);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd(val);
}

void ft_Snapshot2(u32 fmt, u32 ptr, s16 x, s16 y, s16 w, s16 h)
{
  ft_ccmd(FT_CCMD_SNAPSHOT2);
  ft_ccmd(fmt);
  ft_ccmd(ptr);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
}

void ft_LoadImage(u32 ptr, u32 options)
{
  ft_ccmd(FT_CCMD_LOADIMAGE);
  ft_ccmd(ptr);
  ft_ccmd(options);
}

void ft_SetFont2(u32 font, u32 ptr, u32 firstchar)
{
  ft_ccmd(FT_CCMD_SETFONT2);
  ft_ccmd(font);
  ft_ccmd(ptr);
  ft_ccmd(firstchar);
}

void ft_SetRotate(u32 r)
{
  ft_ccmd(FT_CCMD_SETROTATE);
  ft_ccmd(r);
}

void ft_Dlstart()
{
  ft_ccmd(FT_CCMD_DLSTART);
}

void ft_Snapshot(u32 ptr)
{
  ft_ccmd(FT_CCMD_SNAPSHOT);
  ft_ccmd(ptr);
}

void ft_ScreenSaver()
{
  ft_ccmd(FT_CCMD_SCREENSAVER);
}

void ft_MemCrc(u32 ptr, u32 num, u32 result)
{
  ft_ccmd(FT_CCMD_MEMCRC);
  ft_ccmd(ptr);
  ft_ccmd(num);
  ft_ccmd(result);
}
