
// Low level disk I/O module for ZC
// (c) TS-Labs
//
// -------------------------------------------

#include "defs.h"
#include "diskio.h"
#include "pffconf.h"

// Global vars
// -------------------------------------------
u8 ctype;
u8 sd_rbuf[4];
u8 sdcrc;

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
u8 sd_acmd(u8 cmd, u32 arg)
{
  u8 rc;
  if ((rc = sd_cmd(APP_CMD, 0)) & 0x80) abort(rc);
  rc = sd_cmd(cmd, arg);

exit:
  return rc;
}

u8 sd_cmd(u8 cmd, u32 arg)
{
  u8 rc;
  u16 i;

  // send command packet
  sd_wr((cmd & 0x3F) | 0x40);
  sd_wr(((u8*)&arg)[3]);
  sd_wr(((u8*)&arg)[2]);
  sd_wr(((u8*)&arg)[1]);
  sd_wr(((u8*)&arg)[0]);
  sd_wr(sdcrc);
  sd_skip(2);   // skip junk

  // receive command response
  i = 10; while (((rc = sd_rd()) & 0x80) && --i);

  if (cmd & 0xC0)   // special command responses
  {
    if (cmd & CMD_R7)
      sd_recv(sd_rbuf, 4);

    if (cmd & CMD_R1b)
    {
      i = 65000; while ((sd_rd() != 0xFF) && --i);
      if (!i) abort(0xFF);
    }
  }

exit:
  return rc;
}

// FAT-FS functions
// -------------------------------------------

// Initialize Disk Drive
DSTATUS disk_initialize (void)
{
  DSTATUS rc;
  u16 i = 65000;
  ctype = CT_NONE;

  sd_cs_off(); sd_skip(512 + 10); sd_cs_on();

  sdcrc = 0x95;
  if (sd_cmd(GO_IDLE_STATE, 0) > 1) abort(STA_NOINIT_IDLE);

  // SDv2
  sdcrc = 0x87;
  if (sd_cmd(SEND_IF_COND, 0x1AA) == 1)
  {
    if ((sd_rbuf[2] != 0x01) || (sd_rbuf[3] != 0xAA)) abort(STA_NOINIT_VOLT);  // non compatible voltage range

    while ((sd_acmd(SEND_OP_COND_SD, 0x40000000)) && --i);  // wait for leaving idle state (ACMD41 with HCS bit)
    if (!i) abort(STA_NOINIT_OPSD);

    if (!sd_cmd(READ_OCR, 0) == 0) abort(STA_NOINIT_OCR);  // check CCS bit in the OCR
    ctype = (sd_rbuf[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;     // SDv2 (HC or SC)
  }

  // SDv1 or MMCv3
  else
  {
    // SDv1
    if (sd_acmd(SEND_OP_COND_SD, 0) <= 1)
    {
      ctype = CT_SD1;
      while ((sd_acmd(SEND_OP_COND_SD, 0)) && --i);  // wait for leaving idle state
    }

    // MMCv3
    else
    {
      ctype = CT_MMC;
      while ((sd_cmd(SEND_OP_COND_MMC, 0)) && --i);  // wait for leaving idle state
    }

    if (!i)
    {
      ctype = CT_NONE;
      abort(STA_NOINIT_LEAVE);
    }

    if (sd_cmd(SET_BLOCKLEN, 512))  // set R/W block length to 512
      ctype = CT_NONE;
  }

  rc = STA_INIT;   // set success status

exit:
  sd_cs_off();
  return rc;
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
  DRESULT rc = RES_ERROR;
  u16 rsz = min(count, 512 - offset);
  u16 ssz = offset;

  sd_cs_on();
  if (!(ctype & CT_BLOCK)) sector *= 512;
  if (sd_cmd(READ_MULTIPLE_BLOCK, sector)) goto exit;

  while (count)
  {
    u16 i = 65000;
    u8 d;

    while (((d = sd_rd()) == 0xFF) && --i);
    if (d != 0xFE) goto exit;

    sd_skip(ssz);
    sd_recv(buff, rsz);
    sd_skip(512 - ssz - rsz);
    sd_skip(2);     // Skip CRC
    buff += rsz;
    count -= rsz;
    rsz = min(count, 512);
    ssz = 0;
  }

  if (sd_cmd(STOP_TRANSMISSION, 0)) goto exit;
  rc = RES_OK;

exit:
  sd_cs_off();
  return rc;
}

// Write Partial Sector
DRESULT disk_writep
(
  const BYTE* buff,   // Pointer to the data to be written, NULL:Initiate/Finalize write operation
  DWORD sector        // Sector number (LBA) or Number of bytes to send
)
{
  DRESULT rc = RES_ERROR;
  static u16 write_cnt;

  // return RES_OK;

  if (!buff)
  {
    if (sector)  // Initiate write process
    {
      sd_cs_on();
      if (!(ctype & CT_BLOCK)) sector *= 512;
      if (sd_cmd(WRITE_BLOCK, sector)) goto exit;

      // data block header
      sd_wr(0xFF); sd_wr(0xFE);
      write_cnt = 512;
      rc = RES_OK;
      goto ret;
    }

    else  // Finalize write process
    {
      u16 i = 65000;

      sd_skip(write_cnt + 2);   // left bytes and CRC
      sd_rd();    // dummy byte read for ZC latency
      if ((sd_rd() & 0x1F) != 0x05) goto exit;

      // receive data resp and wait for end of write process in timeout of 300ms
      while ((sd_rd() != 0xFF) && --i);
      if (!i) goto exit;

      sd_cs_off();
      rc = RES_OK;
    }
  }
  else  // Send data to the disk
  {
    u16 cnt = min((u16)sector, write_cnt);
    write_cnt -= cnt;
    sd_send((u8*)buff, cnt);
    rc = RES_OK;
    goto ret;
  }


exit:
  sd_cs_off();

ret:
  return rc;
}
