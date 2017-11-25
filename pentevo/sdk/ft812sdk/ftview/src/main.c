
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <ts.h>
#include <tslib.h>
#include <wc_api.h>
#include <wc_api.c>

typedef struct
{
  u8 sig[3];
  u8 type;
  u16 xs;
  u16 ys;
} DXP;

typedef struct
{
  u16 sig;
  u16 len;
  u8 prec;
  u8 ys[2];
  u8 xs[2];
} SOF;

typedef struct
{
  u32 sig;
  u8 xs[4];
  u8 ys[4];
  u8 bits;
  u8 ctype;
} IHDR;

typedef struct
{
  u32 len;
  u32 sig;
} tRNS;

const bool is_1st_run = true;   // to get rid of vars runtime init - const located in RAM

#define FS_BUF_SIZE 0x1E00
u8 fs_buf[FS_BUF_SIZE];
u8 cmdl[0x180];

u16 ret_sp;
u32 filesize;
u8 file_ext;
u8 call_type;

// --- Extensions ---------------------------
enum
{
  EXT_JPG,
  EXT_PNG,
  EXT_DLS,
  EXT_DXP,
};

// --- Windows ------------------------------
const WC_TX_WINDOW err_no_ft =
{
  /* window with header and text   */  WC_WIND_HDR_TXT,
  /* cursor color mask             */  0,
  /* X,Y (position)                */  24, 10,
  /* W,H (size)                    */  32, 5,
  /* paper/ink (window color)      */  0x2F,
  /* -reserved-                    */  0,
  /* window restore buffer address */  0,
  /* separators                    */  0, 0,
  /* header text                   */  "\x0E" " Error! ",
  /* footer text                   */  "",
  /* window text                   */  "\x0E\x0C\x01" "No VDAC2 detected!",
};

const WC_TX_WINDOW win_about =
{
  /* window with header and text   */  WC_WIND_HDR_TXT,
  /* cursor color mask             */  0,
  /* X,Y (position)                */  22, 10,
  /* W,H (size)                    */  36, 5,
  /* paper/ink (window color)      */  0x4F,
  /* -reserved-                    */  0,
  /* window restore buffer address */  0,
  /* separators                    */  0, 0,
  /* header text                   */  "\x0E" " About ",
  /* footer text                   */  "",
  /* window text                   */  "\x0E\x0C\x01" "FT812 Viewer v1.0, (c)2017 TSL"
};

// --- FT812 --------------------------------
bool vdac2_init()
{
  if ((ts_rreg(TS_STATUS) & 7) != 7)
    return false;

  ft_init(FT_MODE_7);
  return ft_rreg8(FT_REG_ID) == 0x7C;
}

void show_jpg_png(u8 pt)
{
  u32 sz = filesize;
  u16 xs = 1024; u16 ys = 768;
  u8 fmt = FT_RGB565;
  u16 ofs = 0;

  wc_vmode(0x87);
  wc_load512(fs_buf, FS_BUF_SIZE >> 9);

  if (pt) // JPEG
  {
    int i;
    for (i = 0; i < (FS_BUF_SIZE - sizeof(SOF)); i++)
    {
      SOF *sof = (SOF*)&fs_buf[i];

      if ((sof->sig & 0xF0FF) == 0xC0FF)  // SOFn
      {
        xs = ((u16)sof->xs[0] << 8) + sof->xs[1];
        ys = ((u16)sof->ys[0] << 8) + sof->ys[1];
        break;
      }
    }
  }
  else    // PNG
  {
    int i;
    for (i = 0; i < (FS_BUF_SIZE - sizeof(IHDR)); i++)
    {
      IHDR *ihdr = (IHDR*)&fs_buf[i];

      if (ihdr->sig == 0x52444849)
      {
        xs = ((u16)ihdr->xs[2] << 8) + ihdr->xs[3];
        ys = ((u16)ihdr->ys[2] << 8) + ihdr->ys[3];

        switch (ihdr->ctype)
        {
          case 0:
            fmt = FT_L8;
          break;

          case 3:
            fmt = FT_PALETTED565;
            ofs = 512;

            for (i = 0; i < (FS_BUF_SIZE - sizeof(tRNS)); i++)
            {
              tRNS *trns = (tRNS*)&fs_buf[i];

              if ((trns->len == 0x01000000) && (trns->sig == 0x534E5274))
              {
                fmt = FT_PALETTED4444;
                break;
              }
            }
          break;

          case 6:
            fmt = FT_ARGB4;
          break;
        }
        break;
      }
    }
  }

  ft_ccmd_start(cmdl);
  ft_Dlstart();
  ft_Clear(1, 1, 1);
  if (ofs) ft_PaletteSource(FT_RAM_G);
  ft_SetBitmap(FT_RAM_G + ofs, fmt, xs, ys);
  ft_Begin(FT_BITMAPS);
  ft_Vertex2ii((1024 - min(1024, xs)) >> 1, (768 - min(768, ys)) >> 1, 0, 0);
  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_LoadImage(FT_RAM_G, FT_OPT_NODL);
  ft_ccmd_write();

  while (sz)
  {
    u16 s;
    s = min(sz, FS_BUF_SIZE);
    if (!ft_load_cfifo(fs_buf, s)) return;
    sz -= s;
    if (sz) wc_load512(fs_buf, FS_BUF_SIZE >> 9);
  }
  ft_cp_wait();
}

void show_dxp()
{
  u32 sz, o;
  u16 xo, yo;
  DXP *d;

  wc_vmode(0x87);
  wc_load512(fs_buf, FS_BUF_SIZE >> 9);

  sz = filesize - sizeof(DXP);
  d = (DXP*)fs_buf;
  o = (u32)d->xs * ((u32)d->ys >> 2);
  xo = (1024 - d->xs) >> 1;
  yo = (768 - d->ys) >> 1;

  ft_ccmd_start(cmdl);
  ft_Dlstart();
  ft_Clear(1, 1, 1);
  // ft_ccmd(ft_SaveContext());

  ft_BitmapHandle(0);
  ft_SetBitmap(FT_RAM_G + o, FT_L2, d->xs, d->ys);
  ft_BitmapHandle(1);
  ft_SetBitmap(FT_RAM_G, FT_RGB565, d->xs >> 2, d->ys >> 2);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, d->xs, d->ys);

  ft_Begin(FT_BITMAPS);
  ft_ColorA(255);
  ft_BlendFunc(FT_ONE, FT_ZERO);
  ft_Vertex2ii(xo, yo, 0, 0);

  ft_ColorMask(1, 1, 1, 0);
  ft_BitmapTransformA(64);
  ft_BitmapTransformE(64);
  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ZERO);
  ft_Vertex2ii(xo, yo, 1, 0);
  ft_BlendFunc(FT_DST_ALPHA, FT_ONE);
  ft_Vertex2ii(xo, yo, 1, 1);

  // ft_RestoreContext();
  // ft_SetBase(10);
  // ft_Number(0, 0, 18, 0, filesize);

  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();

  {
    u8 *bptr = fs_buf + sizeof(DXP);
    u16 bsz = FS_BUF_SIZE - sizeof(DXP);

    if (d->type == 0)
    {
      u32 gptr = FT_RAM_G;

      while (sz)
      {
        u16 s = min(sz, bsz);
        ft_write(bptr, gptr, s);
        gptr += s, sz -= s, bptr = fs_buf, bsz = FS_BUF_SIZE;

        if (sz) wc_load512(fs_buf, FS_BUF_SIZE >> 9);
      }
    }
    else
    {
      ft_ccmd_start(cmdl);
      ft_Inflate(FT_RAM_G);
      ft_ccmd_write();

      while (sz)
      {
        u16 s = min(sz, bsz);
        if (!ft_load_cfifo(bptr, s)) return;
        sz -= s, bptr = fs_buf, bsz = FS_BUF_SIZE;

        if (sz) wc_load512(fs_buf, FS_BUF_SIZE >> 9);
      }

      ft_cp_wait();
    }
  }
}

void show_dls()
{
  wc_vmode(0x87);
  wc_load512(fs_buf, FS_BUF_SIZE >> 9);
  ft_write_dl(fs_buf, filesize >> 2);
  ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
}

// --- Aux functions ------------------------
void wait_esc_key()
{
  while (1)
  {
    __asm
      halt
    __endasm;

    if (wc_api__bool(_ESC))
      break;
  }
}

void about()
{
  wc_api_u16(_PRWOW, (u16)&win_about);
  wait_esc_key();
  wc_api_u16(_RRESB, (u16)&win_about);
  wc_exit(WC_EXIT);
}

void error()
{
  wc_api_u16(_PRWOW, (u16)&err_no_ft);
  wait_esc_key();
  wc_api_u16(_RRESB, (u16)&err_no_ft);
  wc_exit(WC_EXIT);
}

void pic_viewer()
{
  // at the first run initialize and detect FT812
  if (is_1st_run)
  {
    if (vdac2_init())
      *(bool*)&is_1st_run = false;
    else
      error();
  }

  // choose which viewer to use
  switch (file_ext)
  {
    case EXT_JPG:
      show_jpg_png(1);
    break;

    case EXT_PNG:
      show_jpg_png(0);
    break;

    case EXT_DXP:
      show_dxp();
    break;

    case EXT_DLS:
      show_dls();
    break;
  }

  // poll keys
  while (1)
  {
    __asm
      halt
    __endasm;

    if (wc_api__bool(_PGD))   // PgDn - next file
      wc_exit(WC_NEXT_FILE);

    if (wc_api__bool(_PGU))   // PgUp - previous file
      wc_exit(WC_PREV_FILE);

    if (wc_api__bool(_ESC))   // Esc - exit
      wc_exit(WC_EXIT);
  }
}

// ------------------------------------------
void main()
{
  __asm
    ld (_ret_sp), sp
    ld (_call_type), a
    ex af, af
    ld (_file_ext), a
    ld (_filesize), hl
    ld (_filesize + 2), de
  __endasm;

  switch (call_type)
  {
    case WC_CALL_EXT:
      pic_viewer();
    break;

    case WC_CALL_MENU:
      about();
    break;

    default:
      wc_exit(WC_EXIT);
  }
}
