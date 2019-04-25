
#include <defs.h>
#include <ts.h>
#include <tslib.h>
#include "zifi.h"

// Init interface
bool zifi_init()
{
  zifi_wr(ZF_CR, ZF_CMD_CLRFIFO_IN | ZF_CMD_CLRFIFO_OUT); // clear FIFOs
  zifi_wr(ZF_CR, ZF_CMD_SETAPI_1); // set API=1
  zifi_wr(ZF_CR, ZF_CMD_GETVER); // get max API
  u8 api = zifi_rd(ZF_CR);

  return api > 0;
}

// Set input FIFO threshold
void zifi_set_recv_threshold(u8 val)
{
  zifi_wr(ZF_ZIBTR, val);
}

// Set input FIFO timeout
void zifi_set_recv_timeout(u8 val)
{
  zifi_wr(ZF_ZITOR, val);
}

// Get input FIFO used size
u8 zifi_input_buffer_used()
{
  return zifi_rd(ZF_ZIFR);
}

// Get output FIFO free size
u8 zifi_output_buffer_free()
{
  return zifi_rd(ZF_ZOFR);
}

//
u16 zifi_receive(void *buf, u16 len)
{
  return 0;
}

//
void zifi_send(void *buf, u16 len)
{
}

//
bool zifi_connect_ap(const char *name, const char *pwd)
{
  return true;
}

//
bool zifi_disconnect_ap()
{
  return true;
}

//
bool zifi_is_connected_ap()
{
  return true;
}

//
bool zifi_connect_server(const char *name)
{
  return true;
}

//
bool zifi_disconnect_server()
{
  return true;
}

//
bool zifi_is_connected_server()
{
  return true;
}

//
bool zifi_get_fw_version(char *buf)
{
  return true;
}

//
bool zifi_get_ap_list()
{
  return true;
}

// Register read
u8 zifi_rd(u8 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld b, (hl)
    ld c, #ZF_LOW
    in l, (c)
    ret
  __endasm;
}

// Register write
void zifi_wr(u8 a, u8 v) __naked
{
  a;  // to avoid SDCC warning
  v;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld b, (hl)
    inc hl
    ld c, #ZF_LOW
    ld a, (hl)
    out (c), a
    ret
  __endasm;
}

// Wait (unconditional)
void zifi_wait(u32 del)
{
  volatile u32 a = del;
  while (a--);
}

// Read input FIFO raw
void zifi_rd_fifo(void *buf, u8 c)
{
  c, buf;    // to avoid SDCC warning

  __asm
    ld hl, #2
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld b, (hl)
    ld c, #ZF_LOW
    ex de, hl
    inir
    ret
  __endasm;
}

// void zifi_rd_fifo_dma(void *buf, u8 c)
// {
  // ts_wreg(TS_DMAWPA, ZF_DR);
  // ts_wreg(TS_DMAWPD, TS_WPD_COM);
  // ts_set_dma_daddr_p((u16)buf, 0xF6);
  // ts_set_dma_daddr_p(0xC000, 0xF6);
  // ts_set_dma_size(c, 1);
  // ts_dma_start(TS_DMA_WTP_RAM);
  // ts_dma_wait();
// }
