
// Low level disk I/O module for ZC
// (c) TSL 2014
//
// -------------------------------------------
// This module uses some IAR-specific things,
// so it is not fully portable
// -------------------------------------------

#include "defs.h"
#include "diskio.h"
#include "pffconf.h"

// Global vars
// -------------------------------------------
u8 ctype;

// Low-level functions
// -------------------------------------------

void sd_cs_off() __naked
{
  __asm
    ld a, #0x03
    out (ZC_SD_CFG), a
    ret
  __endasm;
}

void sd_cs_on() __naked
{
  __asm
    ld a, #0x01
    out (ZC_SD_CFG), a
    ret
  __endasm;
}

u8 sd_rd() __naked
{
  __asm
    in a, (ZC_SD_DAT)
    ld l, a
    ret
  __endasm;
}

void sd_wr(u8 a) __naked
{
  a;  // to avoid SDCC warning
  __asm
    ld hl, #2
    add hl, sp
    ld a, (hl)
    out (ZC_SD_DAT), a
    ret
  __endasm;
}

void sd_rel() __naked
{
  __asm
    ld a, #0x03
    out (ZC_SD_CFG), a
    ld a, #0xFF
    out (ZC_SD_DAT), a
    ret
  __endasm;
}

void sd_recv(void *a, u16 b) __naked
{
  a;  // to avoid SDCC warning
  b;  // to avoid SDCC warning
  __asm
    ld a, #2
    out (#254), a
    call 3$
    ld a, #0
    out (#254), a
    ret
3$:
    ld hl, #4
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

    ld bc, #ZC_SD_DAT
    inc d
1$: dec d
    jr z, 2$
    inir
    jr 1$
2$: inc e
    dec e
    ret z
    ld b, e
    inir
    ret
  __endasm;
}

void sd_send(void *a, u16 b) __naked
{
  a;  // to avoid SDCC warning
  b;  // to avoid SDCC warning
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

    ld bc, #ZC_SD_DAT
    inc d
1$: dec d
    jr z, 2$
    otir
    jr 1$
2$: inc e
    dec e
    ret z
    ld b, e
    otir
    ret
  __endasm;
}

// Skip some bytes
void sd_skip(u16 a)
{
  a;  // to avoid SDCC warning
  __asm
    ld a, #4
    out (#254), a
    call 8$
    ld a, #0
    out (#254), a
    ret
8$:
    ld hl, #4
    add hl, sp
    ld e, (hl)
    inc hl
    ld d, (hl)
    ex de, hl
    ld bc, #-16
4$:    
    ld a, h
    or a
    jr nz, 2$
    ld a, l
    cp #16
    jr c, 1$
2$:
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    in a, (#ZC_SD_DAT)
    add hl, bc
    jr 4$
1$:
    ld a, l
    or a
    ret z
3$:
    in a, (#ZC_SD_DAT)
    dec l
    jr nz, 3$
  __endasm;
}

// Send command packet
u8 sd_cmd(u8 cmd, u32 arg)
{
  u8 rc;
  u8 n;

  // ACMD<n> is the command sequence of CMD55, CMD<n>
  if (cmd & 0x80)
  {
    cmd &= 0x7F;
    if ((rc = sd_cmd(APP_CMD, 0)) > 1) goto exit;
  }

  // select the card
  sd_cs_off(); sd_wr(0xFF);
  sd_cs_on(); sd_wr(0xFF);

  // send command packet
  sd_wr(cmd);
  sd_wr(((u8*)&arg)[3]);
  sd_wr(((u8*)&arg)[2]);
  sd_wr(((u8*)&arg)[1]);
  sd_wr(((u8*)&arg)[0]);
  sd_wr((cmd == GO_IDLE_STATE) ? 0x95 : ((cmd == SEND_IF_COND) ? 0x87 : 0x01));   // real CRCs for certain commands of dummy + stop

  // receive command response
  n = 10;
  while (((rc = sd_rd()) & 0x80) && --n);

exit:
  return rc;
}

// FAT-FS functions
// -------------------------------------------

// Initialize Disk Drive
DSTATUS disk_initialize (void)
{
  DSTATUS stat = STA_NOINIT;
  u8 buf[4];
  u8 cmd;
  u16 i = (u16)-1;
  ctype = CT_NONE;

  sd_cs_off();
  sd_skip(512 + 10);   // to clean the pipes

  if (sd_cmd(GO_IDLE_STATE, 0) != 1) goto exit;

  // SDv2
  if (sd_cmd(SEND_IF_COND, 0x1AA) == 1)
  {
    sd_recv(buf, 4);    // get trailing return value of R7 resp

    // non compatible voltage range
    if ((buf[2] != 0x01) || (buf[3] != 0xAA)) goto exit;

    // wait for leaving idle state (ACMD41 with HCS bit)
    while ((sd_cmd(SEND_OP_COND_SD, 1UL << 30) != 0) && --i);
    if (!i) goto exit;

    // check CCS bit in the OCR
    if (!sd_cmd(READ_OCR, 0) == 0) goto exit;

    sd_recv(buf, 4);    // get trailing return value of R7 resp
    ctype = (buf[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;     // SDv2 (HC or SC)
  }

  // SDv1 or MMCv3
  else
  {
    // SDv1
    if (sd_cmd(SEND_OP_COND_SD, 0) <= 1)
    {
      ctype = CT_SD1;
      cmd = SEND_OP_COND_SD;
    }

    // MMCv3
    else
    {
      ctype = CT_MMC;
      cmd = SEND_OP_COND_MMC;
    }

    // wait for leaving idle state
    while ((sd_cmd(cmd, 0) != 0) && --i);
    if (!i)
    {
      ctype = CT_NONE;
      goto exit;
    }

    // set R/W block length to 512
    if (sd_cmd(SET_BLOCKLEN, 512) != 0)
      ctype = CT_NONE;
  }

    stat = 0;   // set success status

exit:
    sd_rel();   // release SPI
    return stat;
}

// Read Partial Sector
DRESULT disk_readp
(
    BYTE* buff,     // Pointer to the destination object (NULL: Forward data to the stream)
    DWORD sector,   // Sector number (LBA)
    UINT offset,    // Offset in the sector
    UINT count      // Byte count to read
)
{
  DRESULT res = RES_ERROR;
  u16 i;
  u8 d;

  *(u16*)0x4000 = (u16)buff;
  *(u32*)0x4002 = (u16)sector;
  *(u16*)0x4006 = (u16)offset;
  *(u16*)0x4008 = (u16)count;
  
  if (!(ctype & CT_BLOCK)) sector *= 512;

  while (count)
  {
    u16 rsz;

    // send read command and wait for data packet
    if (sd_cmd(READ_SINGLE_BLOCK, sector) != 0) goto exit;
    i = (u16)-1;
    while (((d = sd_rd()) == 0xFF) && --i);
    if (d != 0xFE) goto exit;

    if (offset)
    {
      rsz = min(count, 512 - offset);
      sd_skip(offset);
      offset = 0;
    }
    else
      rsz = min(count, 512);

    sd_recv(buff, rsz);
    sd_skip(512 - offset - rsz);
    sd_skip(2);     // Skip CRC
    *(u16*)0x5800 = (u16)buff;  // !!!
    buff += rsz;
    count -= rsz;
    sector += (ctype & CT_BLOCK) ? 1 : 512;
  }

  res = RES_OK;
exit:
  sd_rel();   // release SPI

  return res;
}

// Write Partial Sector
DRESULT disk_writep
(
  const BYTE* buff,   // Pointer to the data to be written, NULL:Initiate/Finalize write operation
  DWORD sector        // Sector number (LBA) or Number of bytes to send
)
{
  DRESULT res = RES_ERROR;
  static u16 write_cnt;
  u16 i;

  // return RES_OK;

  // Send data to the disk
  if (buff)
  {
    u16 cnt = min((u16)sector, write_cnt);
    write_cnt -= cnt;
    sd_send((u8*)buff, cnt);
    res = RES_OK;
    goto ret;
  }

  else
  {
    // Initiate write process
    if (sector)
    {
      if (!(ctype & CT_BLOCK)) sector *= 512;
      if (sd_cmd(WRITE_BLOCK, sector) != 0) goto exit;

      // data block header
      sd_wr(0xFF); sd_wr(0xFE);
      write_cnt = 512;
      res = RES_OK;
      goto ret;
    }

    // Finalize write process
    else
    {
      sd_skip(write_cnt + 2);   // left bytes and CRC

      sd_rd();    // dummy byte read for ZC latency
      if ((sd_rd() & 0x1F) != 0x05) goto exit;

      // receive data resp and wait for end of write process in timeout of 300ms
      i = (u16)-1;
      while ((sd_rd() != 0xFF) && --i);
      if (!i) goto exit;

      res = RES_OK;
    }
  }

exit:
  sd_rel();   // release SPI

ret:
  return res;
}
