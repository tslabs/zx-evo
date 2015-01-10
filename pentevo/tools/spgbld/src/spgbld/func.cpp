#include "stdafx.h"
#include "defs.h"
#include "func.h"
#include "msg.h"

extern HDR hdr;
extern BLK blk[256];
extern CONF conf;

int parse_arg(int argc, _TCHAR* argv[], _TCHAR* arg, int n)
{
  for (int i=1; i<argc; i++)
    if (!wcscmp(argv[i], arg) && (argc-1) >= (i+n))
      return i + 1;

  return 0;
}

void parse_args(int argc, _TCHAR* argv[])
{
  int i;
  conf.packer = PM_AUTO;

  if (i = parse_arg(argc, argv, L"-b", 2))
  {
    conf.mode = M_BLD;
    conf.in_fname = argv[i];
    conf.out_fname = argv[i+1];
    if (i = parse_arg(argc, argv, L"-c", 1))
      conf.packer = (C_PACK)_wtoi(argv[i]);
    return;
  }

  if (i = parse_arg(argc, argv, L"-u", 1))
  {
    conf.mode = M_UNP;
    conf.in_fname = argv[i];
    return;
  }

  print_help();
  error(ARG);
}

void init_hdr()
{
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.magic, STR(SPG_MAGIC), sizeof(hdr.magic));
  strncpy(hdr.creator, STR(SB_CRT), sizeof(hdr.creator));
  hdr.ver = SPG_VER;
  hdr.subver = SPG_SUBVER;
  hdr.sp = SPG_SP;
  hdr.page3 = SPG_PAGE3;
  hdr.clk = SPG_CLK;
  hdr.ei = SPG_INT;
  hdr.pager = SPG_PGR;
  hdr.resid = SPG_RES;
  time_t t1 = time(NULL);
  tm* t2 = localtime(&t1);
  hdr.day = t2->tm_mday;
  hdr.month = t2->tm_mon + 1;
  hdr.year = t2->tm_year - 100;
  hdr.second = t2->tm_sec;
  hdr.minute = t2->tm_min;
  hdr.hour = t2->tm_hour;
}

u32 num(char *ns)
{
  int n;

  if ((ns[0] == '0') && ns[1] == 'x')
  {
    sscanf(ns, "%x", &n);
    return n;
  }

  if ((ns[0] == '#') || (ns[0] == '$'))
  {
    sscanf(ns + 1, "%x", &n);
    return n;
  }

  else
  {
    sscanf(ns, "%d", &n);
    return n;
  }
}

void load_ini(_TCHAR *name)
{
  char t[256];
  char v[256];
  char v1[256];
  char v2[256];
  int a = 0;
  int b = 0;
    int nblk = 0;

  FILE* f = _wfopen(name, L"rt");
  if (!f)
    error(INI);

  while (!feof(f))
  {
    *t = 0;
    fscanf(f, "%256[^ =\n\r\t]%*[ =]%256[^\n\r]%*c", t, v);
    if (!*t)
    {
      fscanf(f, "%*c"); continue;
    }

    if (t[0] == ';')
      continue;

    strlwr(t);

    // description string
    if (!strcmp(t, STR(F_DESC)))
    {
      strncpy(hdr.desc, v, 32);
      printf("Description:\t%s\n", v);
      continue;
    }

    // stack address
    if (!strcmp(t, STR(F_SP)))
    {
      hdr.sp = (u16)num(v);
      printf("SP:\t\t#%04X\n", hdr.sp);
      continue;
    }

    // resident address
    if (!strcmp(t, STR(F_RES)))
    {
      hdr.resid = (u16)num(v);
      printf("Resident at:\t#%04X\n", hdr.resid);
      continue;
    }

    // pager address
    if (!strcmp(t, STR(F_PAGER)))
    {
      hdr.pager = (u16)num(v);
      if (hdr.pager)
        printf("Pager at:\t#%04X\n", hdr.pager);
      else
        printf("Pager:\t\tNot used\n");
      continue;
    }

    // start address
    if (!strcmp(t, STR(F_START)))
    {
      hdr.start = (u16)num(v);
      printf("Start:\t\t#%04X\n", hdr.start);
      continue;
    }

    // page3
    if (!strcmp(t, STR(F_PAGE3)))
    {
      hdr.page3 = (u8)num(v);
      printf("Page #C000:\t#%02X\n", hdr.page3);
      continue;
    }

    // clock
    if (!strcmp(t, STR(F_CLOCK)))
    {
      hdr.clk = (u8)num(v);
      printf("Clock:\t\t");
      switch (hdr.clk)
      {
        case 0: { printf("3.5"); break; }
        case 1: { printf("7"); break; }
        case 2: { printf("14"); break; }
        case 3: { printf("14+"); break; }
      }
      printf(" MHz\n");
      continue;
    }

    // INT
    if (!strcmp(t, STR(F_INT)))
    {
      hdr.ei = num(v);
      printf("Interrupts:\t");
      if (hdr.ei)
        printf("EI\n");
      else
        printf("DI\n");
      continue;
    }

    // block
    if (!strcmp(t, STR(F_BLK)))
    {
      int size;
      int offset = 0;

      sscanf(v, "%[^ ,\t]%*[ ,\t]%[^ ,\t]%*[ ,\t]%s", v1, v2, v);
      a = num(v1);
      b = num(v2);

      if (a & 0x1FF)
        error(ALGN);

      if ((a < 0xC000) || (a > 0xFE00))
        error(ADDR);

      a -= 0xC000;

      printf("Block:\t\tStart: #%04X  Page: #%02X  File: %s\n", a, b, v);

      struct stat st;
      stat(v, &st);
      size = st.st_size;

      if (size < 0)
      {
        printf("%s: ", v);
        error(FNFD);
      }

      if (size == 0)
      {
        printf("%s: ", v);
        error(ZERO);
      }

      while (size)
      {
        int curr_size = min(size, 0x4000 - a);
        strncpy(blk[nblk].fname, v, sizeof(blk[nblk].fname));
        hdr.blk[nblk].addr = a >> 9;
        hdr.blk[nblk].page = b++;
        blk[nblk].offset = offset;
        blk[nblk].size = curr_size;

        a = 0;
        offset += curr_size;
        size -= curr_size;

        if (++nblk == 256)
          goto end_of_ini;
      }

      continue;
    }

    // unknown tag
    printf("%s: ", t);
    error(UNK);
  }

end_of_ini:
  if (!nblk)
    error(0BLK);

  conf.n_blocks = nblk;
  hdr.num_blk = conf.n_blocks;
  hdr.blk[conf.n_blocks - 1].last = 1;

  fclose(f);
}

void rand_name(char* name)
{
  for (int i=0; i<11; i++)
  {
    name[i] = 65 + (rand() & 15);
  }
  name[11] ='.', name[12] ='t', name[13] ='m', name[14] ='p', name[15] = 0;
}

void store_block(int i, char* fn, int s)
{
  memset(blk[i].data, 0, sizeof(blk[i].data));
  blk[i].size = s;
  hdr.blk[i].size = sz(blk[i].size);
  FILE* f = fopen(fn, "rb");
  fread(blk[i].data, 1, s, f);
  fclose(f);
}

void pack_blocks()
{
  struct stat st;
  int s1, s2, sz1, sz2;
  C_PACK p;

  for (int i=0; i<conf.n_blocks; i++)
  {
    char f1n[16], f2n[16], f3n[16];
    rand_name(f1n); rand_name(f2n); rand_name(f3n);

    // add errors check here!!!
    FILE* f1 = fopen(f1n, "wb");
    fwrite(blk[i].data, 1, blk[i].size, f1);
    fclose(f1);

    s1 = s2 = 16384;
    sz1 = sz2 = 31;

    if (conf.packer == PM_AUTO || conf.packer == PM_MLZ)
    {
      if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-mlz", f1n, f2n, NULL) < 0) error(MHMT);
      stat(f2n, &st); s1 = st.st_size; sz1 = sz(st.st_size);
    }

    if (conf.packer == PM_AUTO || conf.packer == PM_HST)
    {
      if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-hst", f1n, f3n, NULL) < 0) error(MHMT);
      stat(f3n, &st); s2 = st.st_size; sz2 = sz(st.st_size);
    }

    remove(f1n);

    if (sz(blk[i].size) <= min(sz1, sz2))   // if unpacked chunk has the same size as packed, leave it unpacked
      p = PM_NONE;

    else
      p = (sz1 <= sz2) ? PM_MLZ : PM_HST;   // if MegaLZ gives the same chunk size as Hrust, use it as faster

    hdr.blk[i].comp = p;

    switch (p)
    {
      case PM_MLZ:
      {
        store_block(i, f2n, s1);
        break;
      }
      case PM_HST:
      {
        store_block(i, f3n, s2);
        break;
      }
    }

    remove(f2n); remove(f3n);
  }
}

void save_spg(_TCHAR* name)
{
  FILE* f = _wfopen(name, L"wb");

  // save header
  fwrite(&hdr, 1, sizeof(hdr), f);

  // save blocks
  for (int i = 0; i < conf.n_blocks; i++)
  {
    fwrite(&blk[i].data, 1, (hdr.blk[i].size + 1) << 9, f);
  }

  fclose(f);
}

void load_files()
{
  for (int i = 0; i < conf.n_blocks; i++)
  {
    FILE* f = fopen(blk[i].fname, "rb");
    if (!f) error(FNFD);
    
    hdr.blk[i].size = sz(blk[i].size);
      fseek(f, blk[i].offset, SEEK_SET);
    fread(blk[i].data, 1, blk[i].size, f);
    fclose(f);
  }
}

void unpack_spg(_TCHAR* name)
{
  _TCHAR fu[1024];
  char fa[1024];
  char fa1[1024];
  sprintf(fa, "%ls", name);

  // Read input SPG
  FILE* f = _wfopen(name, L"rb");
  if (!f) error(FERR);

  struct _stat st;
  _wstat(name, &st);
  void *fbuf = malloc(st.st_size);

  if (!fbuf)
    error(MALC);

  fread(fbuf, 1, st.st_size, f);
  fclose(f);

  // Create INI
  swprintf(fu, sizeofarr(fu), L"%s.ini", name);
  f = _wfopen(fu, L"wt");
  if (!f) error(FERR);

  HDR *hdr = (HDR*)fbuf;
  fprintf(f, "Desc = %s\n", hdr->desc);
  fprintf(f, "Start = 0x%0X\n", hdr->start);
  fprintf(f, "Stack = 0x%0X\n", hdr->sp);
  fprintf(f, "Resident = 0x%0X\n", hdr->resid);
  fprintf(f, "Page3 = %u\n", hdr->page3);
  fprintf(f, "Clock = %u\n", hdr->clk);
  fprintf(f, "INT = %u\n", hdr->ei);
  fprintf(f, "Pager = 0x%0X\n", hdr->pager);

  int nblk;
  for (int i = 0; i < hdr->num_blk; i++)
  {
    fprintf(f, "Block = 0x%0X, %u, %s.%03u\n", hdr->blk[i].addr * 512 + 0xC000, hdr->blk[i].page, fa, i);
    nblk = i + 1;
    // if (hdr->blk[i].last) break;
  }

  fclose(f);

  // Save files
  u8 *data = (u8*)fbuf + sizeof(HDR);

  for (int i = 0; i < nblk; i++)
  {
    int size = (hdr->blk[i].size + 1) * 512;
    void *addr = data;
    data += size;

    if (hdr->blk[i].comp == PM_NONE)
    {
      swprintf(fu, sizeofarr(fu), L"%s.%03u", name, i);
      f = _wfopen(fu, L"wb");
    }

    else
    {
      rand_name(fa);
      sprintf(fa1, "%ls.%03u", name, i);
      f = fopen(fa, "wb");
    }

    if (!f) error(FERR);
    fwrite(addr, 1, size, f);
    fclose(f);

    if (hdr->blk[i].comp != PM_NONE)
    {
      switch (hdr->blk[i].comp)
      {
        case PM_MLZ:
          if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-d", "-mlz", fa, fa1, NULL) < 0) error(MHMT);
        break;

        case PM_HST:
          if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-d", "-hst", fa, fa1, NULL) < 0) error(MHMT);
        break;
      }

      remove(fa);
    }
  }

  free(fbuf);
}
