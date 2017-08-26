
#include "ft812types.h"
#include "ft812.h"
#include "ft812lib.h"

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

void ft_wr8(u16 a, u8 v) __naked
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

void ft_wr16(u16 a, u16 v) __naked
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

u8 ft_rd8(u16 a) __naked
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

u16 ft_rd16(u16 a) __naked
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

    ld a, #FT_RAM_REG >> 16 | 0x80
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
  while (ft_rd8(FT_REG_ID) != FT_ID);
  
  ft_wr16(FT_REG_HSYNC0 , mode->h_fporch);
  ft_wr16(FT_REG_HSYNC1 , mode->h_fporch + mode->h_sync);
  ft_wr16(FT_REG_HOFFSET, mode->h_fporch + mode->h_sync + mode->h_bporch);
  ft_wr16(FT_REG_HCYCLE , mode->h_fporch + mode->h_sync + mode->h_bporch + mode->h_visible);
  ft_wr16(FT_REG_HSIZE  , mode->h_visible);
  ft_wr16(FT_REG_VSYNC0 , mode->v_fporch - 1);
  ft_wr16(FT_REG_VSYNC1 , mode->v_fporch + mode->v_sync - 1);
  ft_wr16(FT_REG_VOFFSET, mode->v_fporch + mode->v_sync + mode->v_bporch - 1);
  ft_wr16(FT_REG_VCYCLE , mode->v_fporch + mode->v_sync + mode->v_bporch + mode->v_visible);
  ft_wr16(FT_REG_VSIZE  , mode->v_visible);
  
  ft_wr8(FT_REG_PCLK_POL, 0);
  ft_wr8(FT_REG_CSPREAD, 0);
  ft_wr8(FT_REG_PCLK, mode->f_div);
  ft_wr8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wr8(FT_REG_INT_EN, 1);
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

    ld c, #SPI_DATA
l1: outi
    outi
    outi
    outi
    dec de
    ld a, d
    or e
    jr nz, l1

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_write(u32 ft_addr, void *addr, u16 size)
{
  ft_start_write(ft_addr);
  ft_finish_write(addr, size);
}

void ft_write_dl(void *addr, u16 size)
{
  ft_start_write(FT_RAM_DL);
  ft_finish_write(addr, size);
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

    ld c, #SPI_DATA
l1: ini
    ini
    ini
    ini
    dec de
    ld a, d
    or e
    jr nz, l1

    ld a, #SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret
  __endasm;
}

void ft_read(u32 ft_addr, void *addr, u16 size)
{
  ft_start_read(ft_addr);
  ft_finish_read(addr, size);
}

void ft_swap()
{
  ft_wr8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
}

void ft_wait_int()
{
  while (!(ft_rd8(FT_REG_INT_FLAGS) & FT_INT_SWAP));
}
