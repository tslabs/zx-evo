
#include "stdafx.h"

#pragma pack(1)
typedef struct
{
  u8 sig[3];
  u8 type;
  u16 xs;
  u16 ys;
} DXP;

typedef struct
{
  u32 xs;
  u32 ys;
} DXTH;

typedef struct
{
  u16 c0;
  u16 c1;
  u32 l;
} DXTC;

typedef union
{
  struct
  {
    u16 b:5;
    u16 g:6;
    u16 r:5;
  };
  u16 h;
} RGB565;
#pragma pack()

int _tmain(int argc, wchar_t* argv[])
{
  if (argc < 2)
  {
    printf("Usage: dxt2dxp.exe <filename.dxt>\n");
    exit(1);
  }

  wchar_t drive[16];
  wchar_t dir[1024];
  wchar_t fname[1024];
  wchar_t ext[1024];
  wchar_t fn0[1024];
  wchar_t fn1[1024];
  FILE *f0, *f1;
  u8 *m0;
  u32 m0s;
  u16 xs = 0;
  u16 ys = 0;

  _wsplitpath(argv[1], drive, dir, fname, ext);

  u8 dxt[8];
  f0 = _wfopen(argv[1], L"rb"); if (!f0) exit(1);
  fread(dxt, 1, sizeof(dxt), f0);
  xs = (u16)((DXTH*)&dxt)->xs;
  ys = (u16)((DXTH*)&dxt)->ys;

  m0s = (xs * ys) >> 1;
  m0 = (u8*)malloc(m0s); if (!m0) exit(1);

  for (u16 y = 0; y < (ys >> 2); y++)
  {
    u16 *c0 = (u16*)&m0[y * (xs >> 1)];
    u16 *c1 = (u16*)&m0[y * (xs >> 1) + (m0s >> 2)];
    u8 *l  = (u8*)&m0[y * xs + (m0s >> 1)];

    for (u16 x = 0; x < (xs >> 2); x++)
    {
      fread(dxt, 1, sizeof(dxt), f0);
      DXTC *d = (DXTC*)dxt;
      u16 k0 = d->c0;
      u16 k1 = d->c1;
      u32 l1 = d->l;
      u32 c[4] = {0, 3, 1, 2};

      if (k0 <= k1)
      {
        if (l1 & 0xAAAAAAAA)
        {
          bool is_s0 = 0 != (~l1 & 0x55555555);
          bool is_s1 = 0 != (l1 & 0x55555555);

          RGB565 c0, c1;
          c0.h = k0;
          c1.h = k1;
          u8 a;
          a = c0.r + c1.r; c0.r = a >> 1;
          a = c0.g + c1.g; c0.g = a >> 1;
          a = c0.b + c1.b; c0.b = a >> 1;
          u16 k2 = c0.h;

          if (!is_s0)                 // s2 or s1, s2 - replace c0 with 1/2, assign s2 to c0, s1 not changed
            k0 = k2, c[2] = 0;
          else if (is_s0 && !is_s1)   // s0, s2 - replace c1 with 1/2, assign s2 to c1, s0 not changed
            k1 = k2, c[2] = 3;
          else                        // s0, s1, s2 - assign s2 to 1/3 c0 + 2/3 c1, s0/s1 not changed
            c[2] = 2;
        }
      }

      *c0++ = k0;
      *c1++ = k1;

      u32 l2 = 0;

      for (u8 z = 0; z < 16; z++)
      {
        l2 <<= 2;
        l2 |= c[l1 & 3];
        l1 >>= 2;
      }

      l[0] = (u8)(l2 >> 24);
      l[xs >> 2] = (u8)(l2 >> 16);
      l[xs >> 1] = (u8)(l2 >> 8);
      l[(xs >> 2) * 3] = (u8)l2;
      l++;
    }
  }
  fclose(f0);

  swprintf(fn0, L"%s%s%s.tmp", drive, dir, fname); f0 = _wfopen(fn0, L"wb"); if (!f0) exit(1);
  fwrite(m0, 1, m0s, f0); fclose(f0);
  free(m0);

  if (_wspawnlp(_P_WAIT, L"cmd.exe", L"/c", L"zopfli.exe", L"--zlib", fn0, NULL) < 0) exit(1);
  _wremove(fn0);

  swprintf(fn0, L"%s%s%s.tmp.zlib", drive, dir, fname);
  struct _stat st; _wstat(fn0, &st);
  m0 = (u8*)malloc(st.st_size); if (!m0) exit(1);
  f0 = _wfopen(fn0, L"rb"); if (!f0) exit(1);
  fread(m0, 1, st.st_size, f0);

  swprintf(fn1, L"%s%s%s.dxp", drive, dir, fname);
  f1 = _wfopen(fn1, L"wb"); if (!f1) exit(1);

  DXP d;
  d.sig[0] = 'D'; d.sig[1] = 'X'; d.sig[2] = 'P';
  d.type = 1;   // always packed
  d.xs = xs;
  d.ys = ys;
  fwrite((u8*)&d, 1, sizeof(d), f1);
  fwrite(m0, 1, st.st_size, f1);
  fclose(f0);
  _wremove(fn0);

  free(m0);
  fclose(f1);

  return 0;
}

