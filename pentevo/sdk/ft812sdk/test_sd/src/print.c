
// colors
#define C_NORM "\a7"
#define C_PROC "\a9"
#define C_MENU "\aF"
#define C_OK   "\aC"
#define C_BUTN "\aC"
#define C_ERR  "\aA"
#define C_WARN "\aE"
#define C_ACHT "\aB"
#define C_CRIT "\aA"
#define C_HEAD "\aD"
#define C_SEL  "\aE"
#define C_INFO "\aC"
#define C_DATA "\aF"
#define C_QUST "\aE"
#define C_FRAM "\a7"

u8 pcx, pcx0, pcy, pca, pcst;

#define xy(a, b) { pcx = (a); pcy = (b); }
#define x0(a) { pcx0 = (a); }
#define y(a) { pcy = (a); pcx = pcx0; }

void cls()
{
  ts_wreg(TS_BORDER, 0xF0);
  ts_wreg(TS_VCONFIG, 0x83);
  ts_wreg(TS_VPAGE, 0xF6);
  ts_wreg(TS_PAGE3, 0xF6);
  memset((u8*)0xC000, 0, 0x4000);
  
  xy(0, 0);
  pcx0 = 0;
  pcst = 0;
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
    if ((b == '\r') || (b == '\n'))       // CR | LF
    {
      pcx = pcx0;
      pcy++;
    }
    else if (b == '\a')                   // ink
      pcst = 1;
    else                                  // draw char
    {
      u8 *a = (u8*)0xC000;
      u16 o = (pcy << 8) + pcx;
      a[o] = b;
      a[o + 128] = pca;
      pcx++;
    }
  }
  return 0;
}

void hexstr(u8 *ptr, u8 n)
{
  u8 b;
  for (b = 0; b < n; b++)
  {
    printf("%02X ", ptr[b]);
  }
}

void hexdump(u8 lim)
{
  u8 a;
  u16 i = 0;

  printf(C_DATA "\n");

  for (a = 0; a < lim; a++)
  {
    printf("\n");
    hexstr(&sdbuf[i], 16);
    i += 16;
  }

  printf("\n");
}

void dump()
{
  u8 *p = (u8*)0x8000;
  u8 xx, yy;

  for (yy = 0; yy < 36; yy++)
  {
    if (pcy >= 36) break;

    pca = 12;
    printf("\r\n%04X  ", (int)p);

    for (xx = 6; xx < (6 + 16 * 3); xx += 3)
    {
      pca = (*p == 0xCC) ? 1 : 15;
      printf("%02X ", *p++);
    }
  }
}
