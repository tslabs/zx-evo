
u32 AlphaFunc(u8 func, u8 ref)
{
  return ((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}

u32 Begin(u8 prim)
{
  return ((31UL << 24) | prim);
}

u32 BitmapHandle(u8 handle)
{
  return ((5UL << 24) | handle);
}

u32 BitmapLayout(u8 format, u16 linestride, u16 height)
{
  // return ((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
  union
  {
    u32 c;
    uint8_t  b[4];
  };
  b[0] = (u8)height;
  b[1] = (1 & (height >> 8)) | (linestride << 1);
  b[2] = (7 & (linestride >> 7)) | (format << 3);
  b[3] = 7;
  return (c);
}

u32 BitmapSize(u8 filter, u8 wrapx, u8 wrapy, u16 width, u16 height)
{
  u8 fxy = (filter << 2) | (wrapx << 1) | (wrapy);
  // return ((8UL << 24) | ((u32)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
  union
  {
    u32 c;
    uint8_t  b[4];
  };
  b[0] = (u8)height;
  b[1] = (1 & (height >> 8)) | (width << 1);
  b[2] = (3 & (width >> 7)) | (fxy << 2);
  b[3] = 8;
  return (c);
}

u32 BitmapSource(u32 addr)
{
  return ((1UL << 24) | ((addr & 1048575L) << 0));
}

u32 BitmapTransformA(int32_t a)
{
  return ((21UL << 24) | ((a & 131071L) << 0));
}

u32 BitmapTransformB(int32_t b)
{
  return ((22UL << 24) | ((b & 131071L) << 0));
}

u32 BitmapTransformC(int32_t c)
{
  return ((23UL << 24) | ((c & 16777215L) << 0));
}

u32 BitmapTransformD(int32_t d)
{
  return ((24UL << 24) | ((d & 131071L) << 0));
}

u32 BitmapTransformE(int32_t e)
{
  return ((25UL << 24) | ((e & 131071L) << 0));
}

u32 BitmapTransformF(int32_t f)
{
  return ((26UL << 24) | ((f & 16777215L) << 0));
}

u32 BlendFunc(u8 src, u8 dst)
{
  return ((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}

u32 Call(u16 dest)
{
  return ((29UL << 24) | ((dest & 2047L) << 0));
}

u32 Cell(u8 cell)
{
  return ((6UL << 24) | ((cell & 127L) << 0));
}

u32 ClearColorA(u8 alpha)
{
  return ((15UL << 24) | ((alpha & 255L) << 0));
}

u32 ClearColorRGB(u8 red, u8 green, u8 blue)
{
  return ((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

u32 ClearColorRGB(u32 rgb)
{
  return ((2UL << 24) | (rgb & 0xffffffL));
}

u32 Clear(u8 c, u8 s, u8 t)
{
  u8 m = (c << 2) | (s << 1) | t;
  return ((38UL << 24) | m);
}

u32 Clear()
{
  return ((38UL << 24) | 7);
}

u32 ClearStencil(u8 s)
{
  return ((17UL << 24) | ((s & 255L) << 0));
}

u32 ClearTag(u8 s)
{
  return ((18UL << 24) | ((s & 255L) << 0));
}

u32 ColorA(u8 alpha)
{
  return ((16UL << 24) | ((alpha & 255L) << 0));
}

u32 ColorMask(u8 r, u8 g, u8 b, u8 a)
{
  return ((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}

u32 ColorRGB(u8 red, u8 green, u8 blue)
{
  // return ((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
  union
  {
    u32 c;
    uint8_t  b[4];
  };
  b[0] = blue;
  b[1] = green;
  b[2] = red;
  b[3] = 4;
  return (c);
}

u32 ColorRGB(u32 rgb)
{
  return ((4UL << 24) | (rgb & 0xffffffL));
}

u32 Display(void)
{
  return ((0UL << 24));
}

u32 End(void)
{
  return ((33UL << 24));
}

u32 Jump(u16 dest)
{
  return ((30UL << 24) | ((dest & 2047L) << 0));
}

u32 LineWidth(u16 width)
{
  return ((14UL << 24) | ((width & 4095L) << 0));
}

u32 Macro(u8 m)
{
  return ((37UL << 24) | ((m & 1L) << 0));
}

u32 PointSize(u16 size)
{
  return ((13UL << 24) | ((size & 8191L) << 0));
}

u32 RestoreContext(void)
{
  return ((35UL << 24));
}

u32 Return(void)
{
  return ((36UL << 24));
}

u32 SaveContext(void)
{
  return ((34UL << 24));
}

u32 ScissorSize(u16 width, u16 height)
{
  return ((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
}

u32 ScissorXY(u16 x, u16 y)
{
  return ((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
}

u32 StencilFunc(u8 func, u8 ref, u8 mask)
{
  return ((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}

u32 StencilMask(u8 mask)
{
  return ((19UL << 24) | ((mask & 255L) << 0));
}

u32 StencilOp(u8 sfail, u8 spass)
{
  return ((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}

u32 TagMask(u8 mask)
{
  return ((20UL << 24) | ((mask & 1L) << 0));
}

u32 Tag(u8 s)
{
  return ((3UL << 24) | ((s & 255L) << 0));
}

u32 Vertex2f(int16_t x, int16_t y)
{
  // x = int(16 * x);
  // y = int(16 * y);
  return ((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}

u32 Vertex2ii(u16 x, u16 y, u8 handle, u8 cell)
{
  // return ((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
  union
  {
    u32 c;
    uint8_t  b[4];
  };
  b[0] = cell | ((handle & 1) << 7);
  b[1] = (handle >> 1) | (y << 4);
  b[2] = (y >> 4) | (x << 5);
  b[3] = (2 << 6) | (x >> 3);
  return (c);
}
