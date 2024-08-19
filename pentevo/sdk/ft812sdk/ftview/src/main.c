
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <tslib.h>
#include <wc_api.h>
#include "tsconf.h"
#include <esp_spi_defs.h>

#include <wc_api.c>
#include <esp32.c>

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

// const is to get rid of vars runtime init - const located in RAM anyway
const u8 state_ft = 0;
const u8 state_esp = 0;

#define FS_BUF_SIZE 0x1000
u8 fs_buf[FS_BUF_SIZE]; // at 0xAE00 (_PAGE2 to get page)
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
  EXT_AVI,
  EXT_XM,
  EXT_XMZ,
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

const WC_TX_WINDOW err_no_esp =
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
  /* window text                   */  "\x0E\x0C\x01" "No ESP32 detected!",
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
  /* window text                   */  "\x0E\x0C\x01" "FT812 Viewer v1.2, (c)2024 TSL"
};

// --- FT812 --------------------------------
bool vdac2_init()
{
  if ((ts_rreg(TS_STATUS) & 7) != 7)
    return false;

  ft_init(FT_MODE_1024_768_59);
  return ft_rreg8(FT_REG_ID) == 0x7C;
}

bool esp_init()
{
  esp_cmd(ESP_CMD_NOP);
  esp_rd_end();
  esp_wr_end();

  // +++ detect

  return true;
}

void show_jpg_png(u8 is_jpg)
{
  u32 sz = filesize;
  u16 xs = 1024; u16 ys = 768;
  u8 fmt = FT_RGB565;
  u16 ofs = 0;

  wc_vmode(0x87);
  wc_load512(fs_buf, FS_BUF_SIZE >> 9);

  if (is_jpg) // JPEG
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

void show_avi()
{
  u32 sz = filesize;

  wc_vmode(0x87);

  ft_wreg32(FT_REG_FREQUENCY, 64000000UL);

  ft_ccmd_start(cmdl);
  ft_Dlstart();
  ft_ClearColorRGB(128, 128, 160);
  ft_Clear(1, 1, 1);
  ft_Display();
  ft_ccmd(FT_CCMD_SWAP);
  ft_ccmd_write();
  ft_cp_wait();
  // delay(50000);

#define CHUNK_SIZE 4096
#define FIFO_ADDR 0x080000
#define FIFO_SIZE 0x080000

  ft_MediaFifo(FIFO_ADDR, FIFO_SIZE);
  ft_ccmd_write();
  ft_cp_wait();

  u32 waddr = FIFO_ADDR;
  while (waddr < (FIFO_ADDR + FIFO_SIZE - CHUNK_SIZE))
  {
    wc_load512(fs_buf, CHUNK_SIZE >> 9);
    ft_load_ram_dma(fs_buf, *(u8*)_PAGE2, waddr, CHUNK_SIZE);
    waddr += CHUNK_SIZE;
  }

  ft_PlayVideo(OPT_FULLSCREEN | OPT_SOUND | OPT_MEDIAFIFO | OPT_NOTEAR);
  ft_ccmd_write();

  while (sz)
  {
    ft_wreg32(REG_MEDIAFIFO_WRITE, waddr);

    wc_load512(fs_buf, CHUNK_SIZE >> 9);

    while (1)
    {
      u32 raddr = ft_rreg32(REG_MEDIAFIFO_READ);
      u32 free = FIFO_SIZE - ((waddr >= raddr) ? (waddr - raddr) : (FIFO_SIZE - (raddr - waddr)));
      if (free >= CHUNK_SIZE) break;
    }

    ft_load_ram_dma(fs_buf, *(u8*)_PAGE2, waddr, CHUNK_SIZE);
    waddr += CHUNK_SIZE;
    if (waddr >= (FIFO_ADDR + FIFO_SIZE)) waddr = FIFO_ADDR;
  }

    // ft_cp_wait_free(CHUNK_SIZE);
    // u16 s = min(sz, CHUNK_SIZE);
    // wc_load512(fs_buf, CHUNK_SIZE >> 9);
    // ft_load_ram_dma(fs_buf, *(u8*)_PAGE2, s);
    // sz -= s;
}

void play_xm()
{
  esp_cmd(ESP_CMD_XM_STOP);
  esp_wait_busy(10000);
  esp_cmd(ESP_CMD_KILL_OBJECTS);
  esp_wait_busy(10000);
  
  u8 h = esp_create_obj(filesize, (file_ext == EXT_XM) ? OBJ_TYPE_XM : OBJ_TYPE_ZIP);

  u32 sz = filesize;
  u32 offs = 0;

  while (sz)
  {
    u32 s = min(sz, FS_BUF_SIZE);
    u8 sec = (s + 511) >> 9;

    wc_load512(fs_buf, sec);

    esp_wr_reg32(ESP_REG_DATA_SIZE, s);
    esp_wr_reg32(ESP_REG_DATA_OFFSET, offs);
    esp_cmd(ESP_CMD_WRITE_OBJECT);
    esp_wait_status(ESP_ST_DATA_M2S, 2000);
    esp_send_dma((u16)fs_buf, *(u8*)_PAGE2, 512, sec);

    offs += s;
    sz -= s;
  }

  if (file_ext == EXT_XMZ)
  {
    esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_XM);
    esp_wr_reg32(ESP_REG_DATA_SIZE, filesize);
    esp_cmd(ESP_CMD_UNZIP);
    esp_wait_busy(10000);
  }
  
  esp_cmd(ESP_CMD_XM_INIT);
  esp_wait_busy(10000);
  esp_cmd(ESP_CMD_XM_PLAY);
  esp_wait_busy(10000);
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

void error_ft()
{
  wc_api_u16(_PRWOW, (u16)&err_no_ft);
  wait_esc_key();
  wc_api_u16(_RRESB, (u16)&err_no_ft);
  wc_exit(WC_EXIT);
}

void error_esp()
{
  wc_api_u16(_PRWOW, (u16)&err_no_esp);
  wait_esc_key();
  wc_api_u16(_RRESB, (u16)&err_no_esp);
  wc_exit(WC_EXIT);
}

void main_start()
{
  // Audio player
  if (file_ext == EXT_XM || file_ext == EXT_XMZ)
  {
    if (!state_esp)
      *(u8*)&state_esp = esp_init() ? 1 : 2;

    if (state_esp != 1)
      error_esp();
    else
      play_xm();
  }

  // Viewer
  else
  {
    if (!state_ft)
      *(u8*)&state_ft = vdac2_init() ? 1 : 2;

    if (state_ft != 1)
      error_ft();

    else switch (file_ext)
    {
      case EXT_JPG:
      case EXT_PNG:
        show_jpg_png(file_ext == EXT_JPG);
      break;

      case EXT_DXP:
        show_dxp();
      break;

      case EXT_DLS:
        show_dls();
      break;

      case EXT_AVI:
        show_avi();
      break;
    }
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

    if ((file_ext == EXT_XM) || (file_ext == EXT_XMZ))
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
      main_start();
    break;

    case WC_CALL_MENU:
      about();
    break;

    default:
      wc_exit(WC_EXIT);
  }
}
