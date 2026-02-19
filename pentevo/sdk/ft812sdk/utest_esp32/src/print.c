
u8 pcx, pcx0, pcy, pca, pcst, pscrl, pcxt;

void cls()
{
  pcy = pcx = pcx0 = pcxt = pcst = pscrl = 0;

  TS_GYOFFSL = 0;
  TS_GYOFFSH = 0;
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF1;
  memcpy((void*)0xC000, code_866_fnt, 2048);
  TS_PAGE3 = 0xF0;
  memset((u8*)0xC000, 0, 0x4000);
}

void set_rnd_color()
{
  pca++; pca &= 7;
  if (pca < 1) pca++;
  pca |= 8;
}

void crlf()
{
  if (pcy < 30)
    pcy++;
  else
  {
    pscrl = (pscrl + 1) & 63;
    u16 yoffs = (u16)pscrl << 3;
    u8 *addr = (u8*)(0xC000 | ((u16)(pscrl + pcy) << 8) | 128);
    memset(addr, 0, 128);
    asm(ei);
    asm(halt);
    TS_GYOFFSL = (u8)yoffs;
    TS_GYOFFSH = yoffs >> 8;
  }
}

int putchar(u8 b)
{
  if (pcst == 1)                           // ink argument
  {
    pca = ((b > '9') ? (b - 7) : b) - '0';
    pcst = 0;
  }
  else                                    // character
  {
    if (b == '\r')                        // CR
    {
      pcx = pcx0;
      pcxt = 0;
    }
    else if (b == '\n')                   // LF
      crlf();
    else if (b == '\a')                   // ink
      pcst = 1;
    else if (b == '\t')                   // tab
      pcx = pcxt = pcxt + 12;
    else                                  // draw char
    {
      u8 *addr = (u8*)(0xC000 | ((u16)(pscrl + pcy) << 8) | (pcx & 127));
      addr[0] = b;
      addr[128] = pca;

      pcx++;
      if (pcx >= 80)
      {
        putchar('\r');
        putchar('\n');
      }
    }
  }

  return 0;
}

