
#include "ft812types.h"
#include "ft812.h"
#include "ft812lib.h"
#include <ts.h>
#include <tslib.h>

// Modes
const FT_MODE ft_modes[] =
{
  // f_mul, f_div, h_fporch, h_sync, h_bporch, h_visible, v_fporch, v_sync, v_bporch, v_visible
  {6,  2, 16, 96,  48,  640,  11, 2, 31, 480},   //  0: 640x480@57Hz (48MHz)
  {8,  2, 24, 40,  128, 640,  9,  3, 28, 480},   //  1: 640x480@74Hz (64MHz)
  {8,  2, 16, 96,  48,  640,  11, 2, 31, 480},   //  2: 640x480@76Hz (64MHz)
  {5,  1, 40, 128, 88,  800,  1,  4, 23, 600},   //  3: 800x600@60Hz (40MHz)
  {10, 2, 40, 128, 88,  800,  1,  4, 23, 600},   //  4: 800x600@60Hz (80MHz)
  {6,  1, 56, 120, 64,  800,  37, 6, 23, 600},   //  5: 800x600@69Hz (48MHz)
  {7,  1, 32, 64,  152, 800,  1,  3, 27, 600},   //  6: 800x600@85Hz (56MHz)
  {8,  1, 24, 136, 160, 1024, 3,  6, 29, 768},   //  7: 1024x768@59Hz (64MHz)
  {9,  1, 24, 136, 144, 1024, 3,  6, 29, 768},   //  8: 1024x768@67Hz (72MHz)
  {10, 1, 16, 96,  176, 1024, 1,  3, 28, 768},   //  9: 1024x768@76Hz (80MHz)
  {7,  1, 24, 56,  124, 640,  1,  3, 38, 1024},  // 10: 1280/2x1024@60Hz (56MHz)
  {9, 1, 110, 40,  220, 1280, 5,  5, 20, 720},   // 11: 1280x720@58Hz (72MHz)
  {9,  1, 93, 40,  187, 1280, 5,  5, 20, 720},   // 12: 1280x720@60Hz (72MHz)
};

u32 *ft_ccmdb;
u16 ft_ccmdp;

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

  while (p[c++] = *s++);

  ft_ccmdp += (c + 3) >> 2;
}

void ft_cp_wait()
{
  while (ft_rreg16(FT_REG_CMDB_SPACE) != 0xFFC);
}

void ft_cp_reset()
{
  ft_wreg8(FT_REG_CPURESET, 1);
  ft_wreg32(FT_REG_CMD_READ, 0);
  ft_wreg32(FT_REG_CMD_WRITE, 0);
  ft_wreg8(FT_REG_CPURESET, 0);
}

void ft_ccmd_write()
{
  ft_write((u8*)ft_ccmdb, FT_REG_CMDB_WRITE, ft_ccmdp << 2);
}

bool ft_load_cfifo(void *h, u16 s)
{
  u8 *m = h;
  s = (s + 3) & 0xFFFFFFFC;

  while (s)
  {
    u16 sp = ft_rreg16(FT_REG_CMDB_SPACE);
    u16 z = min(sp, s);

    if (sp & 3)
    {
      ft_cp_reset();
      return false;
    }

    ft_write(m, FT_REG_CMDB_WRITE, z);

    m += z, s -= z;
  }

  return true;
}

bool ft_load_cfifo_p(void *h, u8 p, u32 s)
{
  u16 m = (u16)h & 0x3FFF;
  s = (s + 3) & 0xFFFFFFFC;

  while (s)
  {
    u16 sp = ft_rreg16(FT_REG_CMDB_SPACE);
    u16 z = min(16384 - m, min(sp, s));

    if (sp & 3)
    {
      ft_cp_reset();
      return false;
    }

    ts_wreg(TS_PAGE3, p);
    ft_write((u8*)(m | 0xC000), FT_REG_CMDB_WRITE, z);

    m += z, s -= z;
    if (m >= 16384) m -= 16384, p++;
  }

  return true;
}

bool ft_load_cfifo_dma(void *h, u8 p, u32 s)
{
  u16 m = (u16)h & 0x3FFF;
  s = (s + 3) & 0xFFFFFFFC;
  ts_set_dma_saddr_p(m, p);

  while (s)
  {
    u16 sp = ft_rreg16(FT_REG_CMDB_SPACE);
    u16 z = min(16384 - m, min(sp, s));

    if (sp & 3)
    {
      ft_cp_reset();
      return false;
    }

    ft_start_write(FT_REG_CMDB_WRITE);

    if (z >> 9)
    {
      ts_set_dma_size(512, z >> 9);
      ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
      ts_dma_wait();
    }

    if (z & 0x1FF)
    {
      ts_set_dma_size(z & 0x1FF, 1);
      ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
      ts_dma_wait();
    }

    ft_spi_unsel();

    m += z, s -= z;
    if (m >= 16384) m -= 16384;
  }

  return true;
}

void ft_load_ram_p(void *h, u8 p, u32 a, u32 s)
{
  u16 m = (u16)h & 0x3FFF;

  while (s)
  {
    u16 z = min(16384 - m, min(16384, s));
    ts_wreg(TS_PAGE3, p);
    ft_write((u8*)(m | 0xC000), a, z);

    m += z; a += z; s -= z;

    if (m >= 16384)
    {
      m -= 16384;
      p++;
    }
  }
}

void ft_load_ram_dma(void *h, u8 p, u32 a, u32 s)
{
  u16 m = (u16)h & 0x3FFF;
  ts_set_dma_saddr_p(m, p);

  while (s)
  {
    u16 z = min(16384 - m, min(16384, s));
    ft_start_write(a);

    if (z >> 9)
    {
      ts_set_dma_size(512, z >> 9);
      ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
      ts_dma_wait();
    }

    if (z & 0x1FF)
    {
      ts_set_dma_size(z & 0x1FF, 1);
      ts_wreg(TS_DMACTR, TS_DMA_RAM_SPI);
      ts_dma_wait();
    }

    ft_spi_unsel();

    m += z; a += z; s -= z;

    if (m >= 16384) m -= 16384;
  }
}

void ft_spi_sel() __naked
{
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_spi_unsel() __naked
{
  __asm
    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_cmdp(u8 a, u8 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld a, (hl)
    inc hl
    out (SPI_DATA), a
    ld a, (hl)
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_cmd(u8 a)
{
  ft_cmdp(a, 0);
}

void ft_wreg8(u16 a, u8 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16 | 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    ld a, (hl)
    out (SPI_DATA), a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_wreg16(u16 a, u16 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16 | 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    ld a, (hl)
    inc hl
    out (SPI_DATA), a
    ld a, (hl)
    out (SPI_DATA), a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_wreg32(u16 a, u32 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16 | 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    ld a, (hl)
    inc hl
    out (SPI_DATA), a
    ld a, (hl)
    inc hl
    out (SPI_DATA), a
    ld a, (hl)
    inc hl
    out (SPI_DATA), a
    ld a, (hl)
    out (SPI_DATA), a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

u8 ft_rreg8(u16 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a   // dummy (FT812)
    in a, (SPI_DATA)    // dummy (ZC)
    in a, (SPI_DATA)
    ld l, a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

u16 ft_rreg16(u16 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a   // dummy (FT812)
    in a, (SPI_DATA)    // dummy (ZC)
    in a, (SPI_DATA)
    ld l, a
    in a, (SPI_DATA)
    ld h, a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

u32 ft_rreg32(u16 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl

    ld a, #FT_RAM_REG >> 16
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a   // dummy (FT812)
    in a, (SPI_DATA)    // dummy (ZC)
    in a, (SPI_DATA)
    ld l, a
    in a, (SPI_DATA)
    ld h, a
    in a, (SPI_DATA)
    ld e, a
    in a, (SPI_DATA)
    ld d, a

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

// power up FT812, set up clocks and interrupts
void ft_init(u8 m)
{
  const FT_MODE *mode = &ft_modes[m];

  ft_cmd(FT_CMD_PWRDOWN);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_SLEEP);
  ft_cmd(FT_CMD_CLKEXT);
  ft_cmdp(FT_CMD_CLKSEL, mode->f_mul | 0xC0);
  ft_cmd(FT_CMD_ACTIVE);
  while (ft_rreg8(FT_REG_ID) != FT_ID);

  ft_wreg16(FT_REG_HSYNC0 , mode->h_fporch);
  ft_wreg16(FT_REG_HSYNC1 , mode->h_fporch + mode->h_sync);
  ft_wreg16(FT_REG_HOFFSET, mode->h_fporch + mode->h_sync + mode->h_bporch);
  ft_wreg16(FT_REG_HCYCLE , mode->h_fporch + mode->h_sync + mode->h_bporch + mode->h_visible);
  ft_wreg16(FT_REG_HSIZE  , mode->h_visible);
  ft_wreg16(FT_REG_VSYNC0 , mode->v_fporch - 1);
  ft_wreg16(FT_REG_VSYNC1 , mode->v_fporch + mode->v_sync - 1);
  ft_wreg16(FT_REG_VOFFSET, mode->v_fporch + mode->v_sync + mode->v_bporch - 1);
  ft_wreg16(FT_REG_VCYCLE , mode->v_fporch + mode->v_sync + mode->v_bporch + mode->v_visible);
  ft_wreg16(FT_REG_VSIZE  , mode->v_visible);

  ft_wreg8(FT_REG_PCLK_POL, 0);
  ft_wreg8(FT_REG_CSPREAD, 0);
  ft_wreg8(FT_REG_PCLK, mode->f_div);
  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);
}

void ft_start_write(u32 a) __naked
{
  a;  // to avoid SDCC warning

  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld a, (hl)
    or #0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    ret
  __endasm;
}

void ft_finish_write(void *a, u16 s) __naked
{
  s;  // to avoid SDCC warning
  a;  // to avoid SDCC warning

  __asm
    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld a, (hl)
    inc hl
    ld h, (hl)
    ld l, a
    ex de, hl

    ld bc, #SPI_DATA
    inc d
1$: dec d
    jr z, 2$
    otir
    jr 1$
2$: inc e
    dec e
    jr z, 3$
    ld b, e
    otir

3$: ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_write(void *addr, u32 ft_addr, u16 size)
{
  ft_start_write(ft_addr);
  ft_finish_write(addr, size);
}

void ft_write_dl(void *addr, u16 size)
{
  ft_start_write(FT_RAM_DL);
  ft_finish_write(addr, size << 2);
}

void ft_start_read(u32 a) __naked
{
  a;  // to avoid SDCC warning

  __asm
    ld a, #SPI_FT_CS_ON
    out (SPI_CTRL), a

    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld a, (hl)
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a
    in a, (SPI_DATA)
    ret
  __endasm;
}

void ft_finish_read(void *a, u16 s) __naked
{
  s;  // to avoid SDCC warning
  a;  // to avoid SDCC warning

  __asm
    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld a, (hl)
    inc hl
    ld h, (hl)
    ld l, a
    ex de, hl

    ld bc, #SPI_DATA
    inc d
1$: dec d
    jr z, 2$
    inir
    jr 1$
2$: inc e
    dec e
    jr z, 3$
    ld b, e
    inir

3$: ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_read(void *addr, u32 ft_addr, u16 size)
{
  ft_start_read(ft_addr);
  ft_finish_read(addr, size);
}

void ft_swap()
{
  ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
}

void ft_wait_swap()
{
  while (!(ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP));
}
