#include "std.h"
#include "sysdefs.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "tsconf.h"
#include "sdcard.h"
#include "zc.h"
#include "z80.h"

extern VCTR vid;

    const u8 pwm[32] =
    {
        0,
        10,
        21,
        31,
        42,
        53,
        63,
        74,
        85,
        95,
        106,
        117,
        127,
        138,
        149,
        159,
        170,
        181,
        191,
        202,
        213,
        223,
        234,
        245,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255
    };

// convert CRAM data to precalculated renderer tables
void update_clut(u8 addr)
{
	u16 t = comp.cram[addr];
	u8 r = (t >> 10) & 0x1F;
	u8 g = (t >> 5) & 0x1F;
	u8 b = t & 0x1F;
    int s = t & 0x8000;

    switch (comp.ts.vdac)
    {
        case TS_VDAC_5:
            if (!s)
                goto pwm_set;

            else
            {
                r = r << 3;       // Ccccc000 model
                g = g << 3;
                b = b << 3;
            }
        break;
        
        case TS_VDAC_4:
            if (!s)
                goto pwm_set;       // FIX ME! Here must be clone-specific PWM

            else
            {
                r = ((r << 3) & 0xF0) | (r >> 1);      // CcccCccc model
                g = ((g << 3) & 0xF0) | (g >> 1);
                b = ((b << 3) & 0xF0) | (b >> 1);
            }
        break;
        
        case TS_VDAC_3:
            if (!s)
                goto pwm_set;       // FIX ME! Here must be clone-specific PWM

            else
            {
                r = ((r << 3) & 0xE0) | (r & 0x1C) | (r >> 3);       // CccCccCc model
                g = ((g << 3) & 0xE0) | (g & 0x1C) | (g >> 3);
                b = ((b << 3) & 0xE0) | (b & 0x1C) | (b >> 3);
            }
        break;

        default:
        pwm_set:
            r = pwm[r];
            g = pwm[g];
            b = pwm[b];
    }

	vid.clut[addr] = (r << 16) | (g <<8) | b;
}

void dma_init()
{
  comp.ts.dma.len = comp.ts.dmalen + 1;
  comp.ts.dma.num = comp.ts.dmanum;

  comp.ts.dma.saddr = comp.ts.saddr;
  comp.ts.dma.daddr = comp.ts.daddr;

  comp.ts.dma.m1 = comp.ts.dma.asz ? 0x3FFE00 : 0x3FFF00;
  comp.ts.dma.m2 = comp.ts.dma.asz ? 0x0001FF : 0x0000FF;

  comp.ts.dma.asize = comp.ts.dma.asz ? 512 : 256;

  comp.ts.dma.dstate = DMA_DS_NONE;

  switch (comp.ts.dma.dev)
  {
  case DMA_RAM:
    comp.ts.dma.state = comp.ts.dma.rw ? DMA_ST_BLT : DMA_ST_RAM;
    break;
  case DMA_SPI:
    comp.ts.dma.state = comp.ts.dma.rw ? DMA_ST_SPI_W : DMA_ST_SPI_R;
    break;
  case DMA_IDE:
    comp.ts.dma.state = comp.ts.dma.rw ? DMA_ST_IDE_W : DMA_ST_IDE_R;
    break;
  case DMA_CRAM:
    comp.ts.dma.state = comp.ts.dma.rw ? DMA_ST_CRAM : DMA_ST_FILL;
    break;
  case DMA_SFILE:
    comp.ts.dma.state = comp.ts.dma.rw ? DMA_ST_SFILE : DMA_ST_NOP;
    break;
  default:
    comp.ts.dma.state = DMA_ST_NOP;
    break;
  }
}

void dma_next_burst()
{
  if (comp.ts.dma.s_algn) // source align ?
  {
    comp.ts.saddr += comp.ts.dma.asize; // add align offset to dma source address
    comp.ts.dma.saddr = comp.ts.saddr;  // copy new address to dma structure
  }
  else
    comp.ts.saddr = comp.ts.dma.saddr;  // copy address to dma source address

  if (comp.ts.dma.d_algn) // destination align ?
  {
    comp.ts.daddr += comp.ts.dma.asize; // add align offset to dma destination address
    comp.ts.dma.daddr = comp.ts.daddr;  // copy new address to dma structure
  }
  else
    comp.ts.daddr = comp.ts.dma.daddr;  // copy address to dma source address

  if (comp.ts.dma.num)
  {
    comp.ts.dma.num--;
    comp.ts.dma.len = comp.ts.dmalen + 1;
  }
  else
  {
    comp.ts.intctrl.new_dma = true;
    comp.ts.dma.state = DMA_ST_NOP;
  }
}

#define ss_inc() ss = comp.ts.dma.s_algn ? ((ss & comp.ts.dma.m1) | ((ss + 2) & comp.ts.dma.m2)) : ((ss + 2) & 0x3FFFFF)
#define dd_inc() dd = comp.ts.dma.d_algn ? ((dd & comp.ts.dma.m1) | ((dd + 2) & comp.ts.dma.m2)) : ((dd + 2) & 0x3FFFFF)

u32 dma_ram(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;
  u32 &dd = comp.ts.dma.daddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    if (comp.ts.dma.dstate == DMA_DS_NONE) // data is empty ?
    { // read data
      u16 *s = (u16*)(ss + RAM_BASE_M);
      comp.ts.dma.data = *s;
      comp.ts.dma.dstate = DMA_DS_DATA;
    }
    else
    { // write data
      u16 *d = (u16*)(dd + RAM_BASE_M);
      *d = comp.ts.dma.data;
      comp.ts.dma.dstate = DMA_DS_NONE;
      comp.ts.dma.len--;
      ss_inc();
      dd_inc();
    }
  }
  return 0;
}

u32 dma_blt(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;
  u32 &dd = comp.ts.dma.daddr;

  u16 *s, *d;
  u16 data;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    switch (comp.ts.dma.dstate)
    {
    case DMA_DS_NONE: // read data
      s = (u16*)(ss + RAM_BASE_M);
      comp.ts.dma.data = *s;
      comp.ts.dma.dstate = DMA_DS_DATA;
      break;
    case DMA_DS_DATA: // blitting data
      d = (u16*)(dd + RAM_BASE_M);
      data = 0;
      if (comp.ts.dma.asz)
      {
        data |= (comp.ts.dma.data & 0xFF00) ? (comp.ts.dma.data & 0xFF00) : (*d & 0xFF00);
        data |= (comp.ts.dma.data & 0x00FF) ? (comp.ts.dma.data & 0x00FF) : (*d & 0x00FF);
      }
      else
      {
        data |= (comp.ts.dma.data & 0xF000) ? (comp.ts.dma.data & 0xF000) : (*d & 0xF000);
        data |= (comp.ts.dma.data & 0x0F00) ? (comp.ts.dma.data & 0x0F00) : (*d & 0x0F00);
        data |= (comp.ts.dma.data & 0x00F0) ? (comp.ts.dma.data & 0x00F0) : (*d & 0x00F0);
        data |= (comp.ts.dma.data & 0x000F) ? (comp.ts.dma.data & 0x000F) : (*d & 0x000F);
      }
      comp.ts.dma.data = data;
      comp.ts.dma.dstate = DMA_DS_BLIT;
      break;
    case DMA_DS_BLIT: // write data
      d = (u16*)(dd + RAM_BASE_M);
      *d = comp.ts.dma.data;
      comp.ts.dma.dstate = DMA_DS_NONE;
      comp.ts.dma.len--;
      ss_inc();
      dd_inc();
      break;
    }
  }
  return 0;
}

u32 dma_spi_r(u32 n)
{
  u32 &dd = comp.ts.dma.daddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *d = (u16*)(dd + RAM_BASE_M);
    u16 data = Zc.Rd(0x57);
    data |= Zc.Rd(0x57) << 8;
    *d = data;
    comp.ts.dma.len--;
    dd_inc();
  }

  return 0;
}

u32 dma_spi_w(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *s = (u16*)(ss + RAM_BASE_M);
    u16 data = *s;
    Zc.Wr(0x57, data & 0xFF);
    Zc.Wr(0x57, data >> 8);
    comp.ts.dma.len--;
    ss_inc();
  }

  return 0;
}

u32 dma_ide_r(u32 n)
{
  u32 &dd = comp.ts.dma.daddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *d = (u16*)(dd + RAM_BASE_M);
    *d = hdd.read_data();
    comp.ts.dma.len--;
    dd_inc();
  }

  return 0;
}

u32 dma_ide_w(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *s = (u16*)(ss + RAM_BASE_M);
    hdd.write_data(*s);
    comp.ts.dma.len--;
    ss_inc();
  }

  return 0;
}

u32 dma_fill(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;
  u32 &dd = comp.ts.dma.daddr;

  if (comp.ts.dma.dstate == DMA_DS_NONE && n > 0) // no data and have free memcyc
  {
    u16 *s = (u16*)(ss + RAM_BASE_M);
    comp.ts.dma.data = *s;            // read data
    comp.ts.dma.dstate = DMA_DS_DATA; // set data state as valid
    ss_inc();
    n--;
  }

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *d = (u16*)(dd + RAM_BASE_M);
    *d = comp.ts.dma.data;
    comp.ts.dma.len--;
    dd_inc();
  }
  return 0;
}

u32 dma_cram(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;
  u32 &dd = comp.ts.dma.daddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *s = (u16*)(ss + RAM_BASE_M);
    u8 d = (dd >> 1) & 0xFF;
    comp.cram[d] = *s;
    update_clut(d);
    comp.ts.dma.len--;
    ss_inc();
    dd_inc();
  }

  return 0;
}

u32 dma_sfile(u32 n)
{
  u32 &ss = comp.ts.dma.saddr;
  u32 &dd = comp.ts.dma.daddr;

  for (; n > 0; n--)
  {
    if (!comp.ts.dma.len) // burst is empty
    {
      dma_next_burst();     // get next burst
      if (!comp.ts.dma.len) // no new burst
        return n;
    }

    u16 *s = (u16*)(ss + RAM_BASE_M);
    u8 d = (dd >> 1) & 0xFF;
    comp.sfile[d] = *s;
    comp.ts.dma.len--;
    ss_inc();
    dd_inc();
  }

  return 0;
}

DMA_TASK DMATask[] = {
  { dma_ram },
  { dma_blt },
  { dma_spi_r },
  { dma_spi_w },
  { dma_ide_r },
  { dma_ide_w },
  { dma_fill },
  { dma_cram },
  { dma_sfile }
};

void dma(u32 tacts)
{
  // get new task for dma
  if (comp.ts.dma.state == DMA_ST_INIT)
    dma_init();

  // if no task for dma
  if (comp.ts.dma.state == DMA_ST_NOP)
    return;

  // do task
  tacts -= DMATask[comp.ts.dma.state].task(tacts);
  vid.memdmacyc[vid.line] += (u16)tacts;
}

// TS Engine

SPRITE_t *spr = (SPRITE_t*)comp.sfile;
u32 snum;

void init_tmap_layer()
{
  if (!(comp.ts.t0_en || comp.ts.t1_en))
  {
    comp.ts.tsu.state = comp.ts.tsu.render ? TSS_SPR_RENDER : TSS_NOP;
    return;
  }
  u32 y = (u32)vid.line + 17 - vid.raster.u_brd; // calculate y position of TileMap reader task
  u32 y_line = y & 0x07; // calculate line for current TileMap row
  u32 y0 = (y - y_line + comp.ts.t0_yoffs) & 0x1FF; // calculate y graphic position for layer 0
  u32 y1 = (y - y_line + comp.ts.t1_yoffs) & 0x1FF; // calculate y graphic position for layer 1
  comp.ts.tsu.tmbpos[0] = (u16)((y0 & 0x18) | y_line) << 3; // calculate TileMap buffer position for layer 0
  comp.ts.tsu.tmbpos[1] = (u16)((y1 & 0x18) | y_line) << 3 | 0x100; // calculate TileMap buffer position for layer 1
  u8 *ptr = page_ram(comp.ts.tmpage);
  comp.ts.tsu.tmap[0] = (TILE_t*)(page_ram(comp.ts.tmpage) + ((y0 & 0x1F8) << 5) + (y_line << 4)); // calculate TileMap pointer for layer 0
  comp.ts.tsu.tmap[1] = (TILE_t*)(page_ram(comp.ts.tmpage) + ((y1 & 0x1F8) << 5) + (y_line << 4) + 0x80); // calculate TileMap pointer for layer 1
  comp.ts.tsu.tmsize = comp.ts.t0_en << 3; // Set TileMap iteration for layer 0
  return;
}

u32 read_tmap_layer(u32 n)
{
  u8 &layer  = comp.ts.tsu.layer;
  u16 &tmbpos = comp.ts.tsu.tmbpos[comp.ts.tsu.layer];

  for (; n > 0; n--) // while have free memcycles
  {
    if (!comp.ts.tsu.tmsize) // No iteration for active layer
    {
      layer = 1 - layer; // switch layer between 0 <=> 1
      if (layer) // active layer is 1 ?
      {
        comp.ts.tsu.tmsize = comp.ts.t1_en << 3; // set TileMap iteration for this layer if it enabled
        return read_tmap_layer(n); // read TileMap for layer 1
      }
      // Set new TSU state
      comp.ts.tsu.state = comp.ts.tsu.render ? TSS_SPR_RENDER : TSS_NOP;
      return n;
    }
    comp.ts.tsu.tmbuf[tmbpos].data = comp.ts.tsu.tmap[layer][0]; // read Tile data
    comp.ts.tsu.tmap[layer]++;

    // Prepare palette for rendering
    comp.ts.tsu.tmbuf[tmbpos].pal = comp.ts.tsu.tmbuf[tmbpos].data.pal << 4;

    // Set first line for rendering
    if (comp.ts.tsu.tmbuf[tmbpos].data.yflp)
      comp.ts.tsu.tmbuf[tmbpos].line = 7;
    else
      comp.ts.tsu.tmbuf[tmbpos].line = 0;

    // Set first position for rendering
    if (comp.ts.tsu.tmbuf[tmbpos].data.xflp)
      comp.ts.tsu.tmbuf[tmbpos].offset = 7, comp.ts.tsu.tmbuf[tmbpos].pos_dir = -1;
    else
      comp.ts.tsu.tmbuf[tmbpos].offset = 0, comp.ts.tsu.tmbuf[tmbpos].pos_dir = 1;

    comp.ts.tsu.tmsize--;
    tmbpos++;
    vid.memtstcyc[vid.line]++;
  }
  return 0;
}

void init_tile()
{
  while (comp.ts.tsu.tnum < comp.ts.tsu.tmax) // have tiles in this line, which must be processed
  {
    comp.ts.tsu.tm = comp.ts.tsu.tmbptr[comp.ts.tsu.tnum & 0x3F];
    comp.ts.tsu.tnum++;

    comp.ts.tsu.pos = comp.ts.tsu.next_pos; // set next position for the graphic line buffer
    comp.ts.tsu.next_pos = (comp.ts.tsu.next_pos + 8) & 0x1FF; // calculate new next position

    if (comp.ts.tsu.tz_en || comp.ts.tsu.tm.data.tnum) // draw this tile ?
    {
      comp.ts.tsu.line = (comp.ts.tsu.y ^ comp.ts.tsu.tm.line) & 0x07; // calculate line for render
      comp.ts.tsu.gptr = page_ram(comp.ts.tsu.gpage) + ((comp.ts.tsu.tm.data.tnum & 0xFC0) << 5) + (comp.ts.tsu.line << 8) + ((comp.ts.tsu.tm.data.tnum & 0x3F) << 2); // calculate graphic pointer for this tile
      comp.ts.tsu.pal = comp.ts.tsu.tpal | comp.ts.tsu.tm.pal; // Merge tile palette bits and palette selector bits
      comp.ts.tsu.pos = (comp.ts.tsu.pos + comp.ts.tsu.tm.offset) & 0x1FF; // calculate start position in graphic line buffer for render this tile
      comp.ts.tsu.pos_dir = comp.ts.tsu.tm.pos_dir; // save position direction
      comp.ts.tsu.gsize = 2; // set 2 graphic iteration for render this tile
      break;
    }
  }
}

void init_sprite()
{
  while (comp.ts.tsu.snum < 85)
  {
    if (comp.ts.tsu.leap) // if previous sprite is last in current layer
    {
      comp.ts.tsu.state = (comp.ts.tsu.layer < 2) ? TSS_TILE_RENDER : TSS_NOP; // Set new TSU state
      return;
    }

    comp.ts.tsu.spr = spr[comp.ts.tsu.snum]; // load sprite into tsu
    comp.ts.tsu.leap = comp.ts.tsu.spr.leap; // copy leap flag
    comp.ts.tsu.snum++;

    if (comp.ts.tsu.spr.act) // sprite is active
    {
      u16 ysize = (comp.ts.tsu.spr.ys + 1) << 3; // calculate sprite size by y
      u16 y = (vid.yctr - comp.ts.tsu.spr.y) & 0x1FF;
      if (y < ysize) // part of sprite present at current line
      {
        u16 xsize = (comp.ts.tsu.spr.xs + 1) << 3; // calculate sprite size by x
        comp.ts.tsu.pos = comp.ts.tsu.spr.xflp ? ((comp.ts.tsu.spr.x + xsize - 1) & 0x1FF) : comp.ts.tsu.spr.x;
        comp.ts.tsu.pos_dir = comp.ts.tsu.spr.xflp ? -1 : 1;
        comp.ts.tsu.line = comp.ts.tsu.spr.yflp ? (ysize - y - 1) : y;
        comp.ts.tsu.gptr = page_ram(comp.ts.tsu.gpage) + ((comp.ts.tsu.spr.tnum & 0xFC0) << 5) + (comp.ts.tsu.line << 8) + ((comp.ts.tsu.spr.tnum & 0x3F) << 2); // calculate graphic pointer for this sprite
        comp.ts.tsu.pal = comp.ts.tsu.spr.pal << 4; // Set prepared palette
        comp.ts.tsu.gsize = (comp.ts.tsu.spr.xs + 1) << 1;
        break;
      }
    }
  }
}

void render_tile()
{
  u8 c;

  c = comp.ts.tsu.gptr[0]; comp.ts.tsu.gptr++; // read color from graphic page for active layer

  if (c & 0xF0) vid.tsline[vid.line & 1][comp.ts.tsu.pos] = comp.ts.tsu.pal | (c >> 4); // draw 0-3 pixels
  comp.ts.tsu.pos += comp.ts.tsu.pos_dir; comp.ts.tsu.pos &= 0x1FF; // go to the next position in graphic line buffer
  if (c & 0x0F) vid.tsline[vid.line & 1][comp.ts.tsu.pos] = comp.ts.tsu.pal | (c & 0x0F); // draw 4-7 pixels
  comp.ts.tsu.pos += comp.ts.tsu.pos_dir; comp.ts.tsu.pos &= 0x1FF; // go to the next position in graphic line buffer

  c = comp.ts.tsu.gptr[0]; comp.ts.tsu.gptr++;

  if (c & 0xF0) vid.tsline[vid.line & 1][comp.ts.tsu.pos] = comp.ts.tsu.pal | (c >> 4);
  comp.ts.tsu.pos += comp.ts.tsu.pos_dir; comp.ts.tsu.pos &= 0x1FF;
  if (c & 0x0F) vid.tsline[vid.line & 1][comp.ts.tsu.pos] = comp.ts.tsu.pal | (c & 0x0F);
  comp.ts.tsu.pos += comp.ts.tsu.pos_dir; comp.ts.tsu.pos &= 0x1FF;
}

void init_tile_layer()
{
  if (!(comp.ts.tsu.layer ? comp.ts.t1_en : comp.ts.t0_en) || comp.ts.notsu) // is current layer not active or flag notsu is set
  {
    comp.ts.tsu.state = TSS_SPR_RENDER; // set next state
    comp.ts.tsu.layer++; // set next layer
    return;
  }
  comp.ts.tsu.y = (vid.yctr + (comp.ts.tsu.layer ? comp.ts.t1_yoffs : comp.ts.t0_yoffs)) & 0x1FF; // calculate y position in active tile layer
  u32 x = (comp.ts.tsu.layer ? comp.ts.t1_xoffs_d : comp.ts.t0_xoffs_d); // calculate x position in active tile layer
  comp.ts.tsu.tnum = (x >> 3) & 0x3F; // set first number of tile for render
  comp.ts.tsu.tmax = comp.ts.tsu.tnum + 46; // set end number of tile for render
  comp.ts.tsu.tmbptr = comp.ts.tsu.tmbuf + ((comp.ts.tsu.y & 0x18) << 3) + (comp.ts.tsu.layer << 8); // pointer to the TileMap line in buffer
  comp.ts.tsu.gpage = (comp.ts.tsu.layer ? comp.ts.t1gpage[2] : comp.ts.t0gpage[2]) & 0xF8; // get graphic page for active layer
  comp.ts.tsu.tpal = (comp.ts.tsu.layer ? comp.ts.t1pal : comp.ts.t0pal) << 6; // Prepared Tile palette selector
  comp.ts.tsu.tz_en = comp.ts.tsu.layer ? comp.ts.t1z_en : comp.ts.t0z_en; // get tz_en flag of active layer
  comp.ts.tsu.pos = (0 - (x & 7)) & 0x1FF; // calculate start position in graphic line buffer
  comp.ts.tsu.next_pos = comp.ts.tsu.pos; // next position in graphic line buffer set equal as pos (it will change in init_tile())
  comp.ts.tsu.gsize = 0;
}

u32 render_tile_layer(u32 n)
{
  for (; n > 0; n--) // while have free tacts
  {
    if (!comp.ts.tsu.gsize) // if this end of graphics iteration for current tile
    {
      init_tile(); // get new tile
      if (!comp.ts.tsu.gsize) // no new tile
      {
        comp.ts.tsu.state = TSS_SPR_RENDER; // set next task
        comp.ts.tsu.layer++; // set next layer
        return n;
      }
    }
    render_tile();
    vid.memtstcyc[vid.line]++;
    comp.ts.tsu.gsize--;
  }
  return 0;
}

void init_sprite_layer()
{
  if (!comp.ts.tsu.layer) // Begin render of first graphic layer in current line
  {
    memset(&vid.tsline[vid.line & 1], 0, 512); // Clear graphic line buffer before render
    comp.ts.tsu.snum = 0; // Reset Sprite number
  }

  if (!comp.ts.s_en || comp.ts.notsu) // not active Sprite layer or flag notsu is set
  {
    comp.ts.tsu.state = (comp.ts.tsu.layer < 2) ? TSS_TILE_RENDER : TSS_NOP; // Set next TSU state
    return;
  }

  comp.ts.tsu.gpage = comp.ts.sgpage;
  comp.ts.tsu.leap = false;
  comp.ts.tsu.gsize = 0;
}


u32 render_sprite_layer(u32 n)
{
  for (; n > 0; n--) // while have free tacts
  {
    if (!comp.ts.tsu.gsize) // if this end of graphic iteration for current sprite
    {
      init_sprite(); // get new sprite
      if (!comp.ts.tsu.gsize) // no new sprite
      {
        comp.ts.tsu.state = (comp.ts.tsu.layer < 2) ? TSS_TILE_RENDER : TSS_NOP; // set next task
        return n;
      }
    }
    render_tile();
    vid.memtsscyc[vid.line]++;
    comp.ts.tsu.gsize--;
  }
  return n;
}

TSU_TASK TSUTask[] = {
  { init_tmap_layer, read_tmap_layer },
  { init_tile_layer, render_tile_layer },
  { init_sprite_layer, render_sprite_layer }
};

u32 render_ts(u32 tacts)
{
  if (comp.ts.tsu.state == TSS_NOP)
  {
    comp.ts.tsu.prev_state = TSS_NOP;
    return tacts;
  }

  // Have new TSU state ?
  if (comp.ts.tsu.prev_state != comp.ts.tsu.state)
  {
    if (comp.ts.tsu.state == TSS_INIT) // Start of new line
    {
      comp.ts.tsu.tmap_read = ((u32)vid.line + 17 >= vid.raster.u_brd && (u32)vid.line + 9 < vid.raster.d_brd); // need to read TileMap in this line ?
      comp.ts.tsu.render = ((u32)vid.line + 1 >= vid.raster.u_brd && (u32)vid.line + 1 < vid.raster.d_brd); // need render graphic in this line ?

      // Set first state at this line
      if (comp.ts.tsu.render) comp.ts.tsu.state = TSS_SPR_RENDER; // set first task for render graphic
      if (comp.ts.tsu.tmap_read) comp.ts.tsu.state = TSS_TMAP_READ; // need processed this task first (overlapped state)
      if (comp.ts.tsu.state == TSS_INIT) // no task for this line ?
      {
        comp.ts.tsu.state = TSS_NOP; // set state as no operation in this line
        return tacts;
      }
      comp.ts.tsu.layer = 0; // Any task begin at layer 0
    }
    comp.ts.tsu.prev_state = comp.ts.tsu.state; // Save current state
    TSUTask[comp.ts.tsu.state].init_task(); // initialization task for current state
    if (comp.ts.tsu.prev_state != comp.ts.tsu.state) return render_ts(tacts); // if state changed process it
  }

  tacts = TSUTask[comp.ts.tsu.state].task(tacts); // do task
  if (comp.ts.tsu.prev_state != comp.ts.tsu.state) // if state changed process it
    tacts = render_ts(tacts);

  return tacts; // return free tacts
}

// This used to debug SFILE operations
void sfile_dump()
{
	FILE *f;

	f = fopen("dump.bin", "wb");
	fwrite(comp.sfile, 1, 512, f);
	fclose(f);

	f = fopen("dump.txt", "w");
	fprintf(f, "sgpage:%d\n\n", comp.ts.sgpage);
	for(int i=0; i<85; i++)
	{
		SPRITE_t s = spr[i];
		fprintf(f, "%d\tx:%d\ty:%d\tact:%d\tleap:%d\txs:%d\tys:%d\ttnum:%5d\txf:%d\tyf:%d\tpal:%d\r", i, s.x, s.y, s.act, s.leap, (s.xs+1)*8, (s.ys+1)*8, s.tnum, s.xflp, s.yflp, s.pal);
	}
	fclose(f);
}

// TS-Config init
void tsinit(void)
{
	comp.ts.page[0] = 0;
	comp.ts.page[1] = 5;
	comp.ts.page[2] = 2;
	comp.ts.page[3] = 0;

	comp.ts.fmaddr = 0;
	comp.ts.im2vect[INT_FRAME] = 0xFF;
	comp.ts.im2vect[INT_LINE]  = 0xFD;
	comp.ts.im2vect[INT_DMA]   = 0xFB;
	comp.ts.intmask = 1;
	comp.ts.intctrl.frame_t = 0;

	for (u16 i = 0; i < TS_CACHE_SIZE; i++)
		cpu.tscache_addr[i] = -1;

	comp.ts.fddvirt = 0;
	comp.ts.vdos = 0;
	comp.ts.vdos_m1 = 0;

	comp.ts.sysconf = 1;		// turbo 7MHz for TS-Conf
	set_clk();
	comp.ts.memconf = 0;
	comp.ts.dma.state = DMA_ST_NOP;		// disable DMA on startup
	comp.ts.cacheconf = 0;  // disable cache

	comp.ts.hsint = 2;
	comp.ts.vsint = 0;

	comp.ts.vpage = comp.ts.vpage_d = 5;
	comp.ts.vconf = comp.ts.vconf_d = 0;
	comp.ts.tsconf = 0;
	comp.ts.palsel = comp.ts.palsel_d = 15;
	comp.ts.g_xoffs = 0;
	comp.ts.g_yoffs = 0;
}

void TSFrameINT(bool vdos)
{
  // Frame INT
  if (!comp.ts.intctrl.frame_pend)
  {
    bool f1 = (cpu.t - comp.ts.intctrl.frame_t) < comp.ts.intctrl.frame_len; // INT signal in current frame
    bool f2 = (comp.ts.intctrl.frame_t + comp.ts.intctrl.frame_len) > conf.frame; // INT signal is transferred from the previous frame
    bool new_frame = cpu.t < comp.ts.intctrl.last_cput; // is it new frame ?

    if (f1 || (f2 && new_frame))
    {
      comp.ts.intctrl.frame_pend = comp.ts.intframe;
      comp.ts.intctrl.frame_cnt = cpu.t - comp.ts.intctrl.frame_t + (f1 ? 0 : conf.frame);
    }
  }
  else if (vdos) { /* No Operation */ }
  else if (comp.ts.intctrl.frame_pend && ((comp.ts.intctrl.frame_cnt + (cpu.t - comp.ts.intctrl.last_cput)) < comp.ts.intctrl.frame_len))
  {
    comp.ts.intctrl.frame_cnt += (cpu.t - comp.ts.intctrl.last_cput);
  }
  else
    comp.ts.intctrl.frame_pend = false;

  comp.ts.intctrl.last_cput = cpu.t;
}
