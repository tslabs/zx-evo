
void wait(u16 d)
{
  volatile u16 a = d;
  while (a--);
}

u16 get_bits(u8 *buf, u16 offs, u8 num)
{
  u16 rc = 0;
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

u16 get_bf(u16 offs, u8 num)
{
  return get_bits(sdbuf, bitmax - offs, num);
}

void get_bfa(u8 *ptr, u16 offs, u8 num)
{
  offs = bitmax - offs;

  while (num--)
  {
    *ptr++ = (u8)get_bits(sdbuf, offs, 8);
    offs += 8;
  }
}
