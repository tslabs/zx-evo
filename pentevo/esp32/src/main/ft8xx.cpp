
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "esp_console.h"
#include "driver/uart.h"

#include "main.h"
#include "spi_slave.h"
#include "ft8xx.h"

const char TAG[] = "ft8xx";

void hexdump(const void *data, size_t len, uint64_t base_off);

#define CHUNK_PAYLOAD 1024

u32 *ft_ccmdb = nullptr;
u16 ft_ccmdp = 0;
u8 *cmdl;
u8 *tx;
u8 *rx;

const FT_MODE ft_modes[] =                         //  |  # | visible  | Fpix MHz | clocks/line | lines/frame | line kHz | frame Hz |
{                                                  //  | -- | -------- | -------- | ----------- | ----------- | -------- | -------- |
  {6,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  0 | 640x480  |       24 |         800 |         524 |   30.000 |   57.252 |
  {8,  2,  24,  40, 128,  640,  9, 3, 28,  480},   //  |  1 | 640x480  |       32 |         832 |         520 |   38.462 |   73.964 |
  {8,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  2 | 640x480  |       32 |         800 |         524 |   40.000 |   76.336 |
  {5,  1,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  3 | 800x600  |       40 |        1056 |         628 |   37.879 |   60.317 |
  {10, 2,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  4 | 800x600  |       40 |        1056 |         628 |   37.879 |   60.317 |
  {6,  1,  56, 120,  64,  800, 37, 6, 23,  600},   //  |  5 | 800x600  |       48 |        1040 |         666 |   46.154 |   69.300 |
  {7,  1,  32,  64, 152,  800,  1, 3, 27,  600},   //  |  6 | 800x600  |       56 |        1048 |         631 |   53.435 |   84.683 |
  {8,  1,  24, 136, 160, 1024,  3, 6, 29,  768},   //  |  7 | 1024x768 |       64 |        1344 |         806 |   47.619 |   59.081 |
  {9,  1,  24, 136, 144, 1024,  3, 6, 29,  768},   //  |  8 | 1024x768 |       72 |        1328 |         806 |   54.217 |   67.267 |
  {10, 1,  16,  96, 176, 1024,  1, 3, 28,  768},   //  |  9 | 1024x768 |       80 |        1312 |         800 |   60.976 |   76.220 |
  {7,  1,  24,  56, 124,  640,  1, 3, 38, 1024},   //  | 10 | 640x1024 |       56 |         844 |        1066 |   66.351 |   62.243 |
  {9,  1, 110,  40, 220, 1280,  5, 5, 20,  720},   //  | 11 | 1280x720 |       72 |        1650 |         750 |   43.636 |   58.182 |
  {9,  1,  93,  40, 187, 1280,  5, 5, 20,  720},   //  | 12 | 1280x720 |       72 |        1600 |         750 |   45.000 |   60.000 |
};

const u16 sintab[257] =
{
  0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023,
  6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390,
  12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638,
  19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707,
  25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537,
  30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074,
  36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263,
  41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055,
  46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403,
  50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265,
  54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606,
  57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391,
  60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595,
  62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196,
  64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178,
  65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533,
  65535
};

i16 rsin(i16 r, u16 th)
{
  i16 th4;
  u16 s;
  i16 p;

  th >>= 6;
  th4 = (i16)(th & 511);

  if (th4 & 256)
    th4 = 512 - th4;

  s = sintab[(u16)th4];
  p = (i16)(((u32)s * (u32)r) >> 16);

  if (th & 512)
    p = -p;

  return p;
}

i16 rcos(i16 r, u16 th)
{
  return rsin(r, (u16)(th + 0x4000));
}

// -------- Display list ---------

void ft_AlphaFunc(u8 func, u8 ref)
{
  ft_ccmd((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}

void ft_Begin(u8 prim)
{
  ft_ccmd((31UL << 24) | prim);
}

void ft_BitmapHandle(u8 handle)
{
  ft_ccmd((5UL << 24) | handle);
}

void ft_BitmapLayout(u8 format, u16 linestride, u16 height)
{
  ft_ccmd((0x28UL << 24) | ((linestride >> 8) & 12L) | ((height >> 9) & 3L));
  ft_ccmd((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
}

void ft_BitmapSize(u8 filter, u8 wrapx, u8 wrapy, u16 width, u16 height)
{
  u8 fxy = (filter << 2) | (wrapx << 1) | (wrapy);
  ft_ccmd((0x29UL << 24) | ((width >> 7) & 12L) | ((height>> 9) & 3L));
  ft_ccmd((8UL << 24) | ((u32)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
}

void ft_BitmapSource(u32 addr)
{
  ft_ccmd((1UL << 24) | ((addr & 0x3FFFFFL) << 0));
}

void ft_BitmapTransformA(i32 a)
{
  ft_ccmd((21UL << 24) | ((a & 131071L) << 0));
}

void ft_BitmapTransformB(i32 b)
{
  ft_ccmd((22UL << 24) | ((b & 131071L) << 0));
}

void ft_BitmapTransformC(i32 c)
{
  ft_ccmd((23UL << 24) | ((c & 16777215L) << 0));
}

void ft_BitmapTransformD(i32 d)
{
  ft_ccmd((24UL << 24) | ((d & 131071L) << 0));
}

void ft_BitmapTransformE(i32 e)
{
  ft_ccmd((25UL << 24) | ((e & 131071L) << 0));
}

void ft_BitmapTransformF(i32 f)
{
  ft_ccmd((26UL << 24) | ((f & 16777215L) << 0));
}

void ft_BlendFunc(u8 src, u8 dst)
{
  ft_ccmd((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}

void ft_Call(u16 dest)
{
  ft_ccmd((29UL << 24) | ((dest & 2047L) << 0));
}

void ft_Cell(u8 cell)
{
  ft_ccmd((6UL << 24) | ((cell & 127L) << 0));
}

void ft_ClearColorA(u8 alpha)
{
  ft_ccmd((15UL << 24) | ((alpha & 255L) << 0));
}

void ft_ClearColorRGB(u8 red, u8 green, u8 blue)
{
  ft_ccmd((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

void ft_ClearColorRGB32(u32 rgb)
{
  ft_ccmd((2UL << 24) | (rgb & 0xffffffL));
}

void ft_Clear(u8 c, u8 s, u8 t)
{
  u8 m = (c << 2) | (s << 1) | t;
  ft_ccmd((38UL << 24) | m);
}

void ft_ClearAll()
{
  ft_ccmd((38UL << 24) | 7);
}

void ft_ClearStencil(u8 s)
{
  ft_ccmd((17UL << 24) | ((s & 255L) << 0));
}

void ft_ClearTag(u8 s)
{
  ft_ccmd((18UL << 24) | ((s & 255L) << 0));
}

void ft_ColorA(u8 alpha)
{
  ft_ccmd((16UL << 24) | ((alpha & 255L) << 0));
}

void ft_ColorMask(u8 r, u8 g, u8 b, u8 a)
{
  ft_ccmd((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}

void ft_ColorRGB(u8 red, u8 green, u8 blue)
{
  ft_ccmd((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

void ft_ColorRGB32(u32 rgb)
{
  ft_ccmd((4UL << 24) | (rgb & 0xffffffL));
}

void ft_Display(void)
{
  ft_ccmd(0UL << 24);
}

void ft_End(void)
{
  ft_ccmd(33UL << 24);
}

void ft_Jump(u16 dest)
{
  ft_ccmd((30UL << 24) | ((dest & 2047L) << 0));
}

void ft_LineWidth(u16 width)
{
  ft_ccmd((14UL << 24) | ((width & 4095L) << 0));
}

void ft_Macro(u8 m)
{
  ft_ccmd((37UL << 24) | ((m & 1L) << 0));
}

void ft_PaletteSource(u32 addr)
{
  ft_ccmd((0x2AUL << 24) | ((addr & 0x3FFFFFL) << 0));
}

void ft_PointSize(u16 size)
{
  ft_ccmd((13UL << 24) | ((size & 8191L) << 0));
}

void ft_RestoreContext(void)
{
  ft_ccmd(35UL << 24);
}

void ft_Return(void)
{
  ft_ccmd(36UL << 24);
}

void ft_SaveContext(void)
{
  ft_ccmd(34UL << 24);
}

void ft_ScissorSize(u16 width, u16 height)
{
  ft_ccmd((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
}

void ft_ScissorXY(u16 x, u16 y)
{
  ft_ccmd((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
}

void ft_StencilFunc(u8 func, u8 ref, u8 mask)
{
  ft_ccmd((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}

void ft_StencilMask(u8 mask)
{
  ft_ccmd((19UL << 24) | ((mask & 255L) << 0));
}

void ft_StencilOp(u8 sfail, u8 spass)
{
  ft_ccmd((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}

void ft_TagMask(u8 mask)
{
  ft_ccmd((20UL << 24) | ((mask & 1L) << 0));
}

void ft_Tag(u8 s)
{
  ft_ccmd((3UL << 24) | s);
}

void ft_Vertex2f(i16 x, i16 y)
{
  ft_ccmd((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}

void ft_Vertex2ii(u16 x, u16 y, u8 handle, u8 cell)
{
  ft_ccmd((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
}

void ft_VertexFormat(u8 f)
{
  ft_ccmd((0x27UL << 24) | (f & 7L));
}

void ft_VertexTranslateX(i32 v)
{
  ft_ccmd((0x2BUL << 24) | ((u32)v & 0x1FFFF));
}

void ft_VertexTranslateY(i32 v)
{
  ft_ccmd((0x2CUL << 24) | ((u32)v & 0x1FFFF));
}

// Co-processor
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

void ft_Text(i16 x, i16 y, i16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_TEXT);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Number(i16 x, i16 y, i16 font, u16 options, i32 n)
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

void ft_Toggle(i16 x, i16 y, i16 w, i16 font, u16 options, u16 state, const char *s)
{
  ft_ccmd(FT_CCMD_TOGGLE);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)font << 16) | (w & 0xffff)));
  ft_ccmd((((u32)state << 16) | options));
  ft_cstr(s);
}

void ft_Gauge(i16 x, i16 y, i16 r, u16 options, u16 major, u16 minor, u16 val, u16 range)
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

void ft_Spinner(i16 x, i16 y, u16 style, u16 scale)
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

void ft_Translate(i32 tx, i32 ty)
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

void ft_Slider(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 range)
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

void ft_TouchTransform(i32 x0, i32 y0, i32 x1, i32 y1, i32 x2, i32 y2, i32 tx0, i32 ty0, i32 tx1, i32 ty1, i32 tx2, i32 ty2, u16 result)
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

void ft_Rotate(i32 a)
{
  ft_ccmd(FT_CCMD_ROTATE);
  ft_ccmd(a);
}

void ft_Button(i16 x, i16 y, i16 w, i16 h, i16 font, u16 options, const char *s)
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

void ft_Scrollbar(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 size, u16 range)
{
  ft_ccmd(FT_CCMD_SCROLLBAR);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd((((u32)range << 16) | (size & 0xffff)));
}

void ft_GetMatrix(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f)
{
  ft_ccmd(FT_CCMD_GETMATRIX);
  ft_ccmd(a);
  ft_ccmd(b);
  ft_ccmd(c);
  ft_ccmd(d);
  ft_ccmd(e);
  ft_ccmd(f);
}

void ft_Sketch(i16 x, i16 y, u16 w, u16 h, u32 ptr, u16 format)
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

void ft_BitmapTransform(i32 x0, i32 y0, i32 x1, i32 y1, i32 x2, i32 y2, i32 tx0, i32 ty0, i32 tx1, i32 ty1, i32 tx2, i32 ty2, u16 result)
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

void ft_Scale(i32 sx, i32 sy)
{
  ft_ccmd(FT_CCMD_SCALE);
  ft_ccmd(sx);
  ft_ccmd(sy);
}

void ft_Clock(i16 x, i16 y, i16 r, u16 options, u16 h, u16 m, u16 s, u16 ms)
{
  ft_ccmd(FT_CCMD_CLOCK);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd((((u32)m << 16) | (h & 0xffff)));
  ft_ccmd((((u32)ms << 16) | (s & 0xffff)));
}

void ft_Gradient(i16 x0, i16 y0, u32 rgb0, i16 x1, i16 y1, u32 rgb1)
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

void ft_Track(i16 x, i16 y, i16 w, i16 h, i16 tag)
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

void ft_Progress(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 range)
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

void ft_Keys(i16 x, i16 y, i16 w, i16 h, i16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_KEYS);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Dial(i16 x, i16 y, i16 r, u16 options, u16 val)
{
  ft_ccmd(FT_CCMD_DIAL);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd(val);
}

void ft_Snapshot2(u32 fmt, u32 ptr, i16 x, i16 y, i16 w, i16 h)
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

void ft_FlashAttach()
{
  ft_ccmd(FT_CCMD_FLASHATTACH);
}

void ft_FlashDetach()
{
  ft_ccmd(FT_CCMD_FLASHDETACH);
}

void ft_FlashFast(u32 rc)
{
  ft_ccmd(FT_CCMD_FLASHFAST);
  ft_ccmd(rc);
}

void ft_FlashSpiDesel()
{
  ft_ccmd(FT_CCMD_FLASHSPIDESEL);
}

void ft_FlashTx(u32 num)
{
  ft_ccmd(FT_CCMD_FLASHTX);
  ft_ccmd(num);
}

void ft_FlashRx(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_FLASHRX);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_FlashErase()
{
  ft_ccmd(FT_CCMD_FLASHERASE);
}

void ft_FlashUpdate(u32 dest, u32 src, u32 num)
{
  ft_ccmd(FT_CCMD_FLASHUPDATE);
  ft_ccmd(dest);
  ft_ccmd(src);
  ft_ccmd(num);
}

// ------------- System layer ---------------

void init_ft8xx()
{
  cmdl = (u8*)malloc_spiram(8192);
  if (!cmdl)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for FT8xx cmdl!");
    return;
  }

  tx = (u8*)heap_caps_malloc(CHUNK_PAYLOAD + 4, MALLOC_CAP_DMA);
  if (!tx)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for FT8xx TX!");
    return;
  }

  rx = (u8*)heap_caps_malloc(CHUNK_PAYLOAD + 4, MALLOC_CAP_DMA);
  if (!rx)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for FT8xx RX!");
    return;
  }
}

// ------------- Hardware layer ---------------

esp_err_t ft_switch_spi_to_2_bit()
{
  ft_wreg8(FT_REG_SPI_WIDTH, FT_SPI_WIDTH_DUAL);
  return spi_master_set_data_lines(2);
}

esp_err_t ft_switch_spi_to_1_bit()
{
  ft_wreg8(FT_REG_SPI_WIDTH, FT_SPI_WIDTH_SINGLE);
  return spi_master_set_data_lines(1);
}

esp_err_t ft_open_session()
{
  esp_err_t err;

  err = spi_switch_to_master();
  if (err != ESP_OK) return err;

  err = spi_master_set_data_lines(1);
  if (err != ESP_OK)
  {
    spi_switch_to_slave();
    return err;
  }

  err = ft_switch_spi_to_2_bit();
  if (err != ESP_OK)
  {
    spi_switch_to_slave();
    return err;
  }

  return ESP_OK;
}

esp_err_t ft_close_session()
{
  esp_err_t err;
  esp_err_t err2;

  err = ft_switch_spi_to_1_bit();

  err2 = spi_switch_to_slave();
  if (err == ESP_OK)
    err = err2;

  return err;
}

esp_err_t ft_xfer(const void *tx, void *rx, size_t size)
{
  return spi_master_xfer((void*)tx, rx, size);
}

inline void wr16le(u8 *p, u16 v)
{
  p[0] = (u8)(v & 0xFF);
  p[1] = (u8)((v >> 8) & 0xFF);
}

inline void wr32le(u8 *p, u32 v)
{
  p[0] = (u8)(v & 0xFF);
  p[1] = (u8)((v >> 8) & 0xFF);
  p[2] = (u8)((v >> 16) & 0xFF);
  p[3] = (u8)((v >> 24) & 0xFF);
}

esp_err_t ft_cmdp(u8 a, u8 v)
{
  u8 tx[3] = { a, v, 0 };
  return ft_xfer(tx, nullptr, sizeof(tx));
}

esp_err_t ft_cmd(u8 a)
{
  return ft_cmdp(a, 0);
}

void ft_wreg8(u32 a, u8 v)
{
  u8 tx[4];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  tx[3] = v;
  ft_xfer(tx, nullptr, sizeof(tx));
}

void ft_wreg16(u32 a, u16 v)
{
  u8 tx[5];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  wr16le(&tx[3], v);
  ft_xfer(tx, nullptr, sizeof(tx));
}

void ft_wreg32(u32 a, u32 v)
{
  u8 tx[7];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  wr32le(&tx[3], v);
  ft_xfer(tx, nullptr, sizeof(tx));
}

u8 ft_rreg8(u32 a)
{
  u8 tx[5] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0
  };
  u8 rx[5] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return rx[4];
}

u16 ft_rreg16(u32 a)
{
  u8 tx[6] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0,
    0
  };
  u8 rx[6] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return (u16)rx[4] | ((u16)rx[5] << 8);
}

u32 ft_rreg32(u32 a)
{
  u8 tx[8] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0,
    0,
    0,
    0
  };
  u8 rx[8] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return (u32)rx[4] | ((u32)rx[5] << 8) | ((u32)rx[6] << 16) | ((u32)rx[7] << 24);
}

esp_err_t ft_write(const void *addr, u32 ft_addr, u32 size)
{
  if (!addr && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  const u8 *src = (const u8*)addr;

  esp_err_t err = ESP_OK;
  u32 a = ft_addr;
  u32 left = size;

  while (left)
  {
    u32 n = left > CHUNK_PAYLOAD ? CHUNK_PAYLOAD : left;

    tx[0] = (u8)(((a >> 16) & 0x3F) | 0x80);
    tx[1] = (u8)((a >> 8) & 0xFF);
    tx[2] = (u8)(a & 0xFF);
    memcpy(&tx[3], src, n);

    err = ft_xfer(tx, nullptr, n + 3);
    if (err != ESP_OK)
      break;

    src += n;
    a += n;
    left -= n;
  }

  return err;
}

esp_err_t ft_read(void *addr, u32 ft_addr, u32 size)
{
  if (!addr && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  u8 *dst = (u8*)addr;

  esp_err_t err = ESP_OK;
  u32 a = ft_addr;
  u32 left = size;

  while (left)
  {
    u32 n = left > CHUNK_PAYLOAD ? CHUNK_PAYLOAD : left;

    tx[0] = (u8)((a >> 16) & 0x3F);
    tx[1] = (u8)((a >> 8) & 0xFF);
    tx[2] = (u8)(a & 0xFF);
    tx[3] = 0;

    err = ft_xfer(tx, rx, n + 4);
    if (err != ESP_OK)
      break;

    memcpy(dst, &rx[4], n);

    dst += n;
    a += n;
    left -= n;
  }

  return err;
}

esp_err_t ft_write_dl(const void *addr, u32 size_dwords)
{
  return ft_write(addr, FT_RAM_DL, size_dwords << 2);
}

void ft_ccmd_start(void *addr)
{
  ft_ccmdb = (u32*)addr;
  ft_ccmdp = 0;
}

void ft_ccmd(u32 a)
{
  ft_ccmdb[ft_ccmdp++] = a;
}

void ft_cstr(const char *s)
{
  u8 *p = (u8*)&ft_ccmdb[ft_ccmdp];
  u16 c = 0;

  while ((p[c++] = (u8)*s++) != 0)
  {}

  ft_ccmdp += (c + 3) >> 2;
}

esp_err_t ft_ccmd_write()
{
  return ft_write((const void*)ft_ccmdb, FT_REG_CMDB_WRITE, (u32)ft_ccmdp << 2);
}

esp_err_t ft_cp_wait(uint32_t timeout_ms)
{
  int64_t t0 = esp_timer_get_time();
  while (1)
  {
    u16 space = ft_rreg16(FT_REG_CMDB_SPACE);

    if (space == 0x0FFC)
      return ESP_OK;

    if (((esp_timer_get_time() - t0) / 1000) >= (int64_t)timeout_ms)
      return ESP_ERR_TIMEOUT;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

esp_err_t ft_cp_reset()
{
  ft_wreg8(FT_REG_CPURESET, 1);
  ft_wreg32(FT_REG_CMD_READ, 0);
  ft_wreg32(FT_REG_CMD_WRITE, 0);
  ft_wreg8(FT_REG_CPURESET, 0);

  return ESP_OK;
}

void ft_swap()
{
  return ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
}

esp_err_t ft_wait_swap(uint32_t timeout_ms)
{
  int64_t t0 = esp_timer_get_time();
  while (1)
  {
    u8 flags = ft_rreg8(FT_REG_INT_FLAGS);

    if (flags & FT_INT_SWAP)
      return ESP_OK;

    if (((esp_timer_get_time() - t0) / 1000) >= (int64_t)timeout_ms)
      return ESP_ERR_TIMEOUT;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

esp_err_t ft_set_mode(u8 m)
{
  if (m >= (sizeof(ft_modes) / sizeof(ft_modes[0])))
    return ESP_ERR_INVALID_ARG;

  const FT_MODE *mode = &ft_modes[m];
  esp_err_t err;

  err = ft_switch_spi_to_1_bit();
  if (err != ESP_OK) return err;

  ft_cmd(FT_CMD_PWRDOWN);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_SLEEP);
  ft_cmd(FT_CMD_CLKEXT);
  ft_cmdp(FT_CMD_CLKSEL, mode->f_mul | 0xC0);
  ft_cmd(FT_CMD_ACTIVE);

  int64_t t0 = esp_timer_get_time();
  while (1)
  {
    u8 id = ft_rreg8(FT_REG_ID);
    if (id == FT_ID) break;

    if ((esp_timer_get_time() - t0) > 100000)
    {
      err = ESP_ERR_TIMEOUT;
      goto fail;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  ft_wreg16(FT_REG_HSYNC0, mode->h_fporch);
  ft_wreg16(FT_REG_HSYNC1, mode->h_fporch + mode->h_sync);
  ft_wreg16(FT_REG_HOFFSET, mode->h_fporch + mode->h_sync + mode->h_bporch);
  ft_wreg16(FT_REG_HCYCLE, mode->h_fporch + mode->h_sync + mode->h_bporch + mode->h_visible);
  ft_wreg16(FT_REG_HSIZE, mode->h_visible);
  ft_wreg16(FT_REG_VSYNC0, mode->v_fporch - 1);
  ft_wreg16(FT_REG_VSYNC1, mode->v_fporch + mode->v_sync - 1);
  ft_wreg16(FT_REG_VOFFSET, mode->v_fporch + mode->v_sync + mode->v_bporch - 1);
  ft_wreg16(FT_REG_VCYCLE, mode->v_fporch + mode->v_sync + mode->v_bporch + mode->v_visible);
  ft_wreg16(FT_REG_VSIZE, mode->v_visible);
  ft_wreg8(FT_REG_PCLK_POL, 0);
  ft_wreg8(FT_REG_CSPREAD, 0);
  ft_wreg8(FT_REG_PCLK, mode->f_div);
  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);

  vTaskDelay(pdMS_TO_TICKS(30));
  ft_wreg32(FT_REG_FREQUENCY, mode->f_mul * 8000000UL);

  err = ft_switch_spi_to_2_bit();
  if (err != ESP_OK) return err;

  return ESP_OK;

fail:
  ft_switch_spi_to_2_bit();
  return err;
}

esp_err_t ft_read_detect(u8 &chip_id, u8 chip_type[4], char datestamp[17])
{
  chip_id = ft_rreg8(FT_REG_ID);
  ft_read(chip_type, FT_ROM_CHIPID, 4);
  ft_read(datestamp, FT_REG_DATESTAMP, 16);
  datestamp[16] = 0;

  return ESP_OK;
}

esp_err_t ft_reset_chip()
{
  esp_err_t err = ft_switch_spi_to_1_bit();
    if (err != ESP_OK) return err;


  ft_cmd(FT_CMD_PWRDOWN);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_SLEEP);
  ft_cmd(FT_CMD_CLKEXT);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_RST_PULSE);

  u8 chip_id;
  int64_t t0 = esp_timer_get_time();

  while ((esp_timer_get_time() - t0) < 100000)
  {
    chip_id = ft_rreg8(FT_REG_ID);
    if (chip_id == FT_ID) break;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  vTaskDelay(pdMS_TO_TICKS(30));
  ft_wreg32(FT_REG_FREQUENCY, 40000000UL);

  err = ft_switch_spi_to_2_bit();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

// ------------- Console ---------------

void ft_print_string(const char *s, u16 x, u16 y)
{
  int i = 0;
  char c;

  while ((c = s[i++]) != 0)
  {
    ft_Cell((u8)c);
    ft_Vertex2f((i16)x, (i16)y);
    x += 9;
  }
}

u32 ft_parse_num_arg(const char *s, const char *name, int *ok)
{
  char buf[32];
  size_t n;
  char *endp = nullptr;
  uint64_t v;

  *ok = 0;

  if (!s)
  {
    printf("Missing <%s>\r\n", name);
    return 0;
  }

  n = strlen(s);
  if (n >= sizeof(buf))
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  memcpy(buf, s, n + 1);

  if (n && buf[n - 1] == ',')
    buf[n - 1] = 0;

  v = strtoull(buf, &endp, 0);
  if (!endp || *endp)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  if (v > 0xFFFFFFFFULL)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  *ok = 1;
  return (u32)v;
}

const char *ft_chip_type_name(const u8 chip_type[4])
{
  if (chip_type[0] != 0x08) return "Unknown";

  switch (chip_type[1])
  {
    case 0x10: return "FT810";
    case 0x11: return "FT811";
    case 0x12: return "FT812";
    case 0x13: return "FT813";
    case 0x15: return "BT815";
    case 0x16: return "BT816";
    case 0x17: return "BT817";
    case 0x18: return "BT818";
    default:   return "Unknown";
  }
}

void ft_print_detect(const u8 chip_id, const u8 chip_type[4], const char *datestamp)
{
  const char *type = ft_chip_type_name(chip_type);

  printf("Chip ID          : %02X\r\n", chip_id);
  printf("Chip type        : %s (%02X %02X %02X %02X)\r\n",
         type,
         chip_type[0], chip_type[1], chip_type[2], chip_type[3]);
  printf("Chip datestamp   : %s\r\n", datestamp);
}

esp_err_t ft_print_info()
{
  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  ft_read_detect(chip_id, chip_type, datestamp);

  u8 cpureset = ft_rreg8(FT_REG_CPURESET);
  u8 pclk = ft_rreg8(FT_REG_PCLK);
  u8 int_en = ft_rreg8(FT_REG_INT_EN);
  u8 int_mask = ft_rreg8(FT_REG_INT_MASK);
  u8 int_flags = ft_rreg8(FT_REG_INT_FLAGS);

  u16 hcycle = ft_rreg16(FT_REG_HCYCLE);
  u16 hoffset = ft_rreg16(FT_REG_HOFFSET);
  u16 hsize = ft_rreg16(FT_REG_HSIZE);
  u16 hsync0 = ft_rreg16(FT_REG_HSYNC0);
  u16 hsync1 = ft_rreg16(FT_REG_HSYNC1);
  u16 vcycle = ft_rreg16(FT_REG_VCYCLE);
  u16 voffset = ft_rreg16(FT_REG_VOFFSET);
  u16 vsize = ft_rreg16(FT_REG_VSIZE);
  u16 vsync0 = ft_rreg16(FT_REG_VSYNC0);
  u16 vsync1 = ft_rreg16(FT_REG_VSYNC1);
  u16 cmdb_space = ft_rreg16(FT_REG_CMDB_SPACE);

  u32 frames = ft_rreg32(FT_REG_FRAMES);
  u32 clock = ft_rreg32(FT_REG_CLOCK);
  u32 frequency = ft_rreg32(FT_REG_FREQUENCY);
  u8 spi_width = ft_rreg8(FT_REG_SPI_WIDTH);

  u16 hfront = hsync0;
  u16 hsync = hsync1 - hsync0;
  u16 hback = hoffset - hsync1;

  u16 vfront = vsync0 + 1;
  u16 vsync = vsync1 - vsync0;
  u16 vback = voffset - vsync1;

  u32 pixel_clock = 0;
  float line_rate_hz = 0.0f;
  float line_rate_khz = 0.0f;
  float frame_rate_hz = 0.0f;

  if (pclk)
  {
    pixel_clock = frequency / pclk;

    if (hcycle)
    {
      line_rate_hz = (float)pixel_clock / (float)hcycle;
      line_rate_khz = line_rate_hz / 1000.0f;
    }

    if (hcycle && vcycle)
      frame_rate_hz = line_rate_hz / (float)vcycle;
  }

  printf("FT812 information:\r\n");
  printf("  Chip ID          : %02X\r\n", chip_id);
  printf("  Chip type        : %s\r\n", ft_chip_type_name(chip_type));
  printf("  Datestamp        : %s\r\n", datestamp);

  printf("  PLL frequency    : %lu Hz\r\n", (unsigned long)frequency);
  printf("  Pixel clock      : %lu Hz\r\n", (unsigned long)pixel_clock);
  printf("  Pixel divider    : %u\r\n", (unsigned)pclk);

  printf("  Resolution       : %u x %u\r\n", (unsigned)hsize, (unsigned)vsize);
  printf("  Total frame      : %u x %u\r\n", (unsigned)hcycle, (unsigned)vcycle);
  printf("  Frame rate       : %.3f Hz\r\n", frame_rate_hz);
  printf("  Line rate        : %.3f kHz\r\n", line_rate_khz);
  printf("  Horizontal       : front=%u sync=%u back=%u active=%u\r\n", (unsigned)hfront, (unsigned)hsync, (unsigned)hback, (unsigned)hsize);
  printf("  Vertical         : front=%u sync=%u back=%u active=%u\r\n", (unsigned)vfront, (unsigned)vsync, (unsigned)vback, (unsigned)vsize);

  switch (spi_width & 3)
  {
    case FT_SPI_WIDTH_SINGLE: printf("  Bus width        : 1-bit SPI\r\n"); break;
    case FT_SPI_WIDTH_DUAL:   printf("  Bus width        : 2-bit SPI\r\n"); break;
    case FT_SPI_WIDTH_QUAD:   printf("  Bus width        : 4-bit SPI\r\n"); break;
    default:                  printf("  Bus width        : ? (%u)\r\n", (unsigned)(spi_width & 3)); break;
  }

  printf("  Frame counter    : %lu\r\n", (unsigned long)frames);
  printf("  Clock counter    : %lu\r\n", (unsigned long)clock);

  printf("  Interrupts       : enable=%u mask=%02X flags=%02X\r\n", (unsigned)int_en, (unsigned)int_mask, (unsigned)int_flags);
  printf("  Command space    : %u\r\n", (unsigned)cmdb_space);
  printf("  CPU reset flags  : %u\r\n", (unsigned)cpureset);

  return ESP_OK;
}

esp_err_t ft_demo_draw_mode_1024_768()
{
  const char *mode_txt = "1024x768@59Hz (64MHz)";
  u16 i;
  u16 j;

  ft_set_mode(FT_MODE_1024_768_59);
  ft_cp_reset();
  ft_ccmd_start(cmdl);
  ft_Dlstart();

  ft_VertexFormat(0);
  ft_ClearColorRGB(0, 0, 0);
  ft_Clear(1, 1, 1);

  ft_ColorRGB(255, 255, 255);
  ft_LineWidth(24);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(1023, 0);
  ft_Vertex2f(1023, 767);
  ft_Vertex2f(0, 767);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(799, 0);
  ft_Vertex2f(799, 599);
  ft_Vertex2f(0, 599);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_LINE_STRIP);
  ft_Vertex2f(0, 0);
  ft_Vertex2f(639, 0);
  ft_Vertex2f(639, 479);
  ft_Vertex2f(0, 479);
  ft_Vertex2f(0, 0);

  ft_Begin(FT_BITMAPS);
  ft_BitmapHandle(18);

  ft_ColorRGB(100, 220, 200);
  ft_print_string("Mode:", 10, 8);

  ft_ColorRGB(120, 100, 255);
  ft_print_string(mode_txt, 66, 8);

  ft_ColorRGB(120, 100, 255);
  ft_print_string("640x480", 572, 461);
  ft_print_string("800x600", 732, 581);
  ft_print_string("1024x768", 947, 749);

  ft_ColorA(255);
  ft_Begin(FT_POINTS);
  ft_PointSize(100 << 4);
  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);

  ft_ColorRGB(255, 0, 0);
  ft_Vertex2f(rsin(80, 32768) + 320, rcos(80, 32768) + 240);

  ft_ColorRGB(0, 255, 0);
  ft_Vertex2f(rsin(80, 21845 + 32768) + 320, rcos(80, 21845 + 32768) + 240);

  ft_ColorRGB(0, 0, 255);
  ft_Vertex2f(rsin(80, 43690 - 32768) + 320, rcos(80, 43690 - 32768) + 240);

  ft_ClearColorA(32);
  ft_ColorMask(0, 0, 0, 1);
  ft_Clear(1, 1, 1);

  ft_BlendFunc(FT_ONE, FT_ONE);
  ft_Begin(FT_POINTS);
  ft_ColorA(20);

  for (i = 120, j = 0; j <= 255; i -= 5, j += 20)
  {
    ft_PointSize(i << 4);
    ft_Vertex2f(320, 240);
  }

  ft_ColorRGB(0, 0, 0);
  ft_ColorMask(1, 1, 1, 1);
  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA);
  ft_Begin(FT_RECTS);
  ft_Vertex2f(100, 50);
  ft_Vertex2f(539, 429);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);

  ft_ccmd_write();
  ft_cp_wait(1000);
  return ESP_OK;
}

int ft_res_cmd(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  esp_err_t err;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_reset_chip();
  if (err != ESP_OK)
  {
    ft_close_session();
    printf("FT reset failed: %d\r\n", (int)err);
    return 1;
  }
  printf("\r\nFT reset done\r\n");

  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  ft_read_detect(chip_id, chip_type, datestamp);
  ft_print_detect(chip_id, chip_type, datestamp);

  err = ft_close_session();
  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_wreg_cmd(int argc, char **argv)
{
  esp_err_t err;
  u32 addr;
  u32 value;
  int ok;

  if (argc < 4)
  {
    printf("Usage:\r\n");
    printf("  ft wreg <addr> <value>\r\n");
    return 1;
  }

  addr = ft_parse_num_arg(argv[2], "addr", &ok);
  if (!ok)
    return 1;

  value = ft_parse_num_arg(argv[3], "value", &ok);
  if (!ok)
    return 1;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  ft_wreg32(addr, value);
  err = ft_close_session();

  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT write: [0x%06lX] = 0x%08lX (%lu)\r\n",
         (unsigned long)addr,
         (unsigned long)value,
         (unsigned long)value);

  return 0;
}

int ft_info_cmd(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  esp_err_t err;
  esp_err_t err2;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  err = ft_read_detect(chip_id, chip_type, datestamp);
  if (err == ESP_OK)
  {
    printf("\r\n");
    err = ft_print_info();
  }

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT info read failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_dump_cmd(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  esp_err_t err;
  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  u8 b[256];

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_read_detect(chip_id, chip_type, datestamp);
  if (err != ESP_OK) printf("FT detect failed: %d\r\n", (int)err);
  printf("\r\n");
  ft_print_detect(chip_id, chip_type, datestamp);

  printf("\r\nRegister hexdump:\r\n");
  ft_read(b, FT_REG_ID, sizeof(b));
  hexdump(b, sizeof(b), FT_REG_ID);

  err = ft_close_session();

  if (err != ESP_OK)
  {
    printf("FT dump close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_demo_cmd(int, char **)
{
  esp_err_t err;

  const i16 x_pos = 350;
  const i16 y_pos = 250;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_set_mode(FT_MODE_800_600_60);

  if (err == ESP_OK)
    err = ft_cp_reset();

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);

    while (1)
    {
      u32 clock = ft_rreg32(FT_REG_CLOCK);
      u32 frames = ft_rreg32(FT_REG_FRAMES);

      ft_ccmd_start(cmdl);

      ft_Dlstart();
      ft_VertexFormat(0);

      ft_ClearColorRGB(0, 0, 0);
      ft_ClearColorA(255);
      ft_Clear(1, 1, 1);

      ft_ColorA(255);
      ft_Begin(FT_POINTS);

      ft_PointSize(100 << 4);
      ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);

      ft_ColorRGB(255, 0, 0);
      ft_Vertex2f(rsin(80, 0) + x_pos, rcos(80, 0) + y_pos);

      ft_ColorRGB(0, 255, 0);
      ft_Vertex2f(rsin(80, 21845) + x_pos, rcos(80, 21845) + y_pos);

      ft_ColorRGB(0, 0, 255);
      ft_Vertex2f(rsin(80, 43690) + x_pos, rcos(80, 43690) + y_pos);

      ft_ClearColorA(32);
      ft_ColorMask(0, 0, 0, 1);
      ft_Clear(1, 1, 1);

      ft_BlendFunc(FT_ONE, FT_ONE);
      ft_Begin(FT_POINTS);
      ft_ColorA(20);

      for (int i = 120, j = 0; j <= 255; i -= 5, j += 20)
      {
        ft_PointSize(i << 4);
        ft_Vertex2f(x_pos, y_pos);
      }

      ft_ColorRGB(0, 0, 0);
      ft_ColorMask(1, 1, 1, 1);
      ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA);
      ft_Begin(FT_RECTS);
      ft_Vertex2f(0, 0);
      ft_Vertex2f(799, 599);

      ft_ColorA(255);
      ft_BlendFunc(FT_SRC_ALPHA, FT_ONE_MINUS_SRC_ALPHA);
      ft_RomFont(14, 34);

      ft_ColorRGB(255, 0, 0);
      ft_Number(100, 80, 14, 0, (i32)clock);

      ft_ColorRGB(0, 200, 240);
      ft_Number(100, 220, 14, 0, (i32)frames);

      ft_ColorRGB(0, 200, 0);
      ft_Text(100, 50, 31, 0, "Clocks:");
      ft_Text(100, 190, 31, 0, "Frames:");

      ft_ColorRGB(255, 120, 0);
      ft_Text(190, 500, 31, 0, "All your base are belong to us!");

      ft_Display();
      ft_Swap();

      err = ft_ccmd_write();
      if (err != ESP_OK) break;

      err = ft_cp_wait(1000);
      if (err != ESP_OK) break;

      err = ft_wait_swap(1000);
      if (err != ESP_OK) break;

      char c;
      if (uart_read_bytes(UART_NUM_0, &c, 1, 0) > 0)
      {
        printf("FT demo stopped\r\n");
        break;
      }
    }
  }

  err = ft_close_session();
  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_cli_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  ft res\r\n");
    printf("  ft info\r\n");
    printf("  ft dump\r\n");
    printf("  ft wreg <addr> <value>\r\n");
    printf("  ft demo\r\n");
    return 1;
  }

  const char *op = argv[1];

  if (!strcmp(op, "res"))
    return ft_res_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return ft_info_cmd(argc, argv);

  if (!strcmp(op, "wreg"))
    return ft_wreg_cmd(argc, argv);

  if (!strcmp(op, "dump"))
    return ft_dump_cmd(argc, argv);

  if (!strcmp(op, "demo"))
    return ft_demo_cmd(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void ft_console_register_system_commands()
{
  {
    const esp_console_cmd_t cmd =
    {
      .command  = "ft",
      .help     = "FT812 commands: 'ft res', 'ft info', 'ft dump', 'ft demo'",
      .hint     = NULL,
      .func     = &ft_cli_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
}
