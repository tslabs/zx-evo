
void wait(u16 d)
{
  volatile u16 a = d;
  while (a--);
}

u32 power(u32 a, u8 b)
{
  u32 p = 1;
  for (u8 i = 0; i < b; i++) p *= a;
  return p;
}

u32 get_bits(u8 *buf, u16 offs, u8 num)
{
  u32 rc = 0;
  u16 i = offs >> 3;
  u8 m = 0x80 >> (offs & 7);
  u8 b = buf[i++];

  while (num--)
  {
    rc <<= 1;
    if (b & m) rc |= 1;
    m >>= 1;

    if (!m)
    {
      b = buf[i++];
      m = 0x80;
    }
  }

  return rc;
}

u32 get_bitfield_bits(u16 offs, u8 num)
{
  return get_bits(sdbuf, bitmax - offs, num);
}

void get_bitfield_bytes(u16 offs, u8 num, void *ptr)
{
  u8 *_ptr = (u8*)ptr;
  offs = bitmax - offs;

  while (num--)
  {
    *_ptr++ = (u8)get_bits(sdbuf, offs, 8);
    offs += 8;
  }
}

const char *look_up_mid(u8 mid)
{
  for (u8 i = 0; i < countof(mid_tab); i++)
    if (mid_tab[i].mid == mid)
      return mid_tab[i].name;

  return "Unknown";
}
