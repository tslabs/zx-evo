#include <avr/io.h>
#include "mytypes.h"
#include "diskio.h"
#include "zx.h"
#include "setup_avr_spi.h"

#define SDC_X1                  0x40
#define SDC_X4                  0x80
#define SDC_R1B                 0xC0

#define SDC_GO_IDLE_STATE       0
#define SDC_SEND_OP_COND_MMC    1
#define SDC_SEND_IF_COND        (8 | SDC_X4)
#define SDC_SEND_CSD            9
#define SDC_STOP_TRANSMISSION   (12 | SDC_R1B)
#define SDC_SET_BLOCKLEN        16
#define SDC_READ_SINGLE_BLOCK   17
#define SDC_WRITE_BLOCK         24
#define SDC_WRITE_MULTIPLE_BLOCK 25
#define SDC_APP_CMD             55
#define SDC_READ_OCR            (58 | SDC_X4)
#define SDC_SD_SEND_OP_COND     41

#define SD_TOKEN_DATA_START     0xFE
#define SD_TOKEN_WRITE          0xFE
#define SD_TOKEN_WRITE_MULTI    0xFC
#define SD_TOKEN_STOP_TRAN      0xFD

#define SD_CFG_SELECT           SETUP_SDCFG_MODE_SELECT
#define SD_CFG_RELEASE          (SETUP_SDCFG_MODE_SELECT | SETUP_SDCFG_SD_RELEASE)

#define CT_NONE                 0x00
#define CT_MMC                  0x01
#define CT_SD1                  0x02
#define CT_SD2                  0x03
#define CT_SDHC                 0x04
#define CT_BLOCK                0x10

#define SD_CACHE_BASE           0x300000UL
#define SD_CACHE_SLOTS          64
#define SD_CACHE_SLOT_SIZE      512UL
#define SD_CACHE_IO_SIZE        64
#define SD_CACHE_META_BASE      (SD_CACHE_BASE + (SD_CACHE_SLOTS * SD_CACHE_SLOT_SIZE))
#define SD_CACHE_META_ENTRY_SIZE 8
#define SD_CACHE_META_FLAGS_OFS 4
#define SD_CACHE_META_CHUNK_ENTRIES (SD_CACHE_IO_SIZE / SD_CACHE_META_ENTRY_SIZE)
#define SD_CACHE_FLAG_VALID     0x01
#define SD_CACHE_FLAG_DIRTY     0x02

u8 ctype;
u8 sd_rbuf[4];
u8 sdcrc;
DSTATUS sd_status = STA_NOINIT;
LBA_t sd_sector_count = 0;
u8 sd_cache_io[SD_CACHE_IO_SIZE];
u8 sd_cache_next_slot;
u8 sd_cache_fat_valid;
u8 sd_cache_fat_count;
u32 sd_cache_fat_start;
u32 sd_cache_fat_size;

void sd_cs_off()
{
  setup_spi_periph_write(SETUP_REG_SDCFG, SD_CFG_RELEASE);
}

void sd_cs_on()
{
  setup_spi_periph_write(SETUP_REG_SDCFG, SD_CFG_SELECT);
}

void sd_proxy_begin()
{
  setup_spi_start(SETUP_SPI_OP_PERIPH_WR, SETUP_REG_SDDAT);
}

void sd_proxy_end()
{
  setup_spi_end();
}

u8 sd_proxy_xfer(u8 data)
{
  return setup_spi_transfer_byte(data);
}

u8 sd_rd()
{
  u8 r;

  sd_proxy_begin();
  r = sd_proxy_xfer(SETUP_PROXY_DUMMY_BYTE);
  sd_proxy_end();
  return r;
}

void sd_wr(u8 data)
{
  sd_proxy_begin();
  sd_proxy_xfer(data);
  sd_proxy_end();
}

void sd_recv(void *dst, u16 count)
{
  u8 *p = (u8*)dst;

  sd_proxy_begin();
  while (count--) *(p++) = sd_proxy_xfer(SETUP_PROXY_DUMMY_BYTE);
  sd_proxy_end();
}

void sd_send(const void *src, u16 count)
{
  const u8 *p = (const u8*)src;

  sd_proxy_begin();
  while (count--) sd_proxy_xfer(*(p++));
  sd_proxy_end();
}

void sd_skip(u16 count)
{
  sd_proxy_begin();
  while (count--) sd_proxy_xfer(SETUP_PROXY_DUMMY_BYTE);
  sd_proxy_end();
}

u8 sd_wait_busy_long()
{
  u32 i = 0xFFFFFFUL;

  while ((sd_rd() != 0xFF) && --i)
  {
  }
  return i != 0;
}

u8 sd_wait_busy()
{
  u16 i = 65535;

  while ((sd_rd() != 0xFF) && --i)
  {
  }
  return i != 0;
}

u8 sd_wait_dtoken()
{
  u8 d;
  u16 i = 65535;

  while (((d = sd_rd()) == 0xFF) && --i)
  {
  }
  return d == SD_TOKEN_DATA_START;
}

u8 sd_cmd(u8 cmd, u32 arg)
{
  u8 rc;
  u8 base_cmd;
  u16 i;

  sd_skip(2);

  base_cmd = cmd & 0x3F;
  sd_proxy_begin();
  sd_proxy_xfer((base_cmd & 0x3F) | 0x40);
  sd_proxy_xfer((u8)(arg >> 24));
  sd_proxy_xfer((u8)(arg >> 16));
  sd_proxy_xfer((u8)(arg >> 8));
  sd_proxy_xfer((u8)arg);
  if (base_cmd == SDC_GO_IDLE_STATE) sd_proxy_xfer(0x95);
  else if (base_cmd == 8) sd_proxy_xfer(0x87);
  else sd_proxy_xfer(sdcrc);
  sd_proxy_end();

  if (base_cmd == 12) sd_rd();

  i = 10;
  do
  {
    rc = sd_rd();
  } while ((rc & 0x80) && --i);

  if (i == 0) return 0xFF;

  switch (cmd & 0xC0)
  {
    case SDC_X1:
      sd_recv(sd_rbuf, 1);
      break;

    case SDC_X4:
      sd_recv(sd_rbuf, 4);
      break;

    case SDC_R1B:
      if (sd_wait_busy() == 0) return 0xFF;
      break;
  }

  return rc;
}

u8 sd_acmd(u8 cmd, u32 arg)
{
  u8 rc;

  rc = sd_cmd(SDC_APP_CMD, 0);
  if (rc & 0x80) return rc;
  return sd_cmd(cmd, arg);
}

void sd_reset_recover()
{
  sd_cs_off();
  sd_skip(80);

  sd_cs_on();
  sd_skip(520);
  sd_wr(SD_TOKEN_STOP_TRAN);
  sd_wait_busy_long();

  sd_cs_off();
  sd_skip(80);
}

u8 sd_recv_data(u8 *dst, u16 size)
{
  if (sd_wait_dtoken() == 0) return 0;
  sd_recv(dst, size);
  sd_skip(2);
  return 1;
}

u32 sd_csd_sector_count(const u8 *csd)
{
  u32 csize;
  u8 read_bl_len;
  u16 csize_mult;
  u32 block_len;
  u32 mult;
  u32 blocknr;

  if ((csd[0] & 0xC0) == 0x40)
  {
    csize = (((u32)csd[7] & 0x3F) << 16) | ((u32)csd[8] << 8) | csd[9];
    return (csize + 1UL) << 10;
  }

  read_bl_len = csd[5] & 0x0F;
  csize = (((u32)csd[6] & 0x03) << 10) | ((u32)csd[7] << 2) | ((csd[8] & 0xC0) >> 6);
  csize_mult = (u16)(((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7));
  block_len = 1UL << read_bl_len;
  mult = 1UL << (csize_mult + 2);
  blocknr = (csize + 1UL) * mult;
  return (blocknr * block_len) >> 9;
}

u8 sd_read_csd()
{
  u8 csd[16];

  sd_sector_count = 0;
  if (sd_cmd(SDC_SEND_CSD, 0)) return 0;
  if (sd_recv_data(csd, 16) == 0) return 0;
  sd_sector_count = sd_csd_sector_count(csd);
  return sd_sector_count ? 1 : 0;
}

u8 sd_read_sector(u8 *dst, LBA_t sector)
{
  if ((ctype & CT_BLOCK) == 0) sector *= 512UL;
  if (sd_cmd(SDC_READ_SINGLE_BLOCK, sector)) return 0;
  return sd_recv_data(dst, 512);
}

u8 sd_write_data_block(const u8 *src, u8 token)
{
  u8 resp;
  u8 retry = 16;

  sd_wr(0xFF);
  sd_wr(token);
  sd_send(src, 512);
  sd_wr(0xFF);
  sd_wr(0xFF);

  do
  {
    resp = sd_rd();
  } while ((resp == 0xFF) && --retry);

  if ((resp & 0x1F) != 0x05) return 0;

  return sd_wait_busy_long();
}

u8 sd_write_sector(const u8 *src, LBA_t sector)
{
  if ((ctype & CT_BLOCK) == 0) sector *= 512UL;
  if (sd_cmd(SDC_WRITE_BLOCK, sector)) return 0;
  return sd_write_data_block(src, SD_TOKEN_WRITE);
}

u8 sd_write_multi(const u8 *src, LBA_t sector, UINT count)
{
  if ((ctype & CT_BLOCK) == 0) sector *= 512UL;
  if (sd_cmd(SDC_WRITE_MULTIPLE_BLOCK, sector)) return 0;

  while (count--)
  {
    if (sd_write_data_block(src, SD_TOKEN_WRITE_MULTI) == 0)
    {
      sd_wr(SD_TOKEN_STOP_TRAN);
      sd_wait_busy_long();
      return 0;
    }

    src += 512;
  }

  sd_wr(SD_TOKEN_STOP_TRAN);
  return sd_wait_busy_long();
}

u16 sd_cache_get_le16(const u8 *p)
{
  return (u16)p[0] | ((u16)p[1] << 8);
}

u32 sd_cache_get_le32(const u8 *p)
{
  return (u32)p[0] |
         ((u32)p[1] << 8) |
         ((u32)p[2] << 16) |
         ((u32)p[3] << 24);
}

void sd_cache_put_le32(u8 *p, u32 value)
{
  p[0] = (u8)value;
  p[1] = (u8)(value >> 8);
  p[2] = (u8)(value >> 16);
  p[3] = (u8)(value >> 24);
}

u32 sd_cache_meta_addr(u8 slot)
{
  return SD_CACHE_META_BASE + ((u32)slot * SD_CACHE_META_ENTRY_SIZE);
}

void sd_cache_meta_clear_io()
{
  u8 i;

  for (i = 0; i < SD_CACHE_IO_SIZE; i++) sd_cache_io[i] = 0;
}

void sd_cache_meta_get(u8 slot, LBA_t *sector, u8 *flags)
{
  setup_spi_dram_read_block(sd_cache_meta_addr(slot), sd_cache_io, SD_CACHE_META_ENTRY_SIZE);
  *sector = (LBA_t)sd_cache_get_le32(sd_cache_io);
  *flags = sd_cache_io[SD_CACHE_META_FLAGS_OFS];
}

void sd_cache_meta_set(u8 slot, LBA_t sector, u8 flags)
{
  sd_cache_put_le32(sd_cache_io, (u32)sector);
  sd_cache_io[SD_CACHE_META_FLAGS_OFS] = flags;
  sd_cache_io[5] = 0;
  sd_cache_io[6] = 0;
  sd_cache_io[7] = 0;
  setup_spi_dram_write_block(sd_cache_meta_addr(slot), sd_cache_io, SD_CACHE_META_ENTRY_SIZE);
}

void sd_cache_meta_set_flags(u8 slot, u8 flags)
{
  sd_cache_io[0] = flags;
  setup_spi_dram_write_block(sd_cache_meta_addr(slot) + SD_CACHE_META_FLAGS_OFS, sd_cache_io, 1);
}

void sd_cache_reset()
{
  u8 i;

  sd_cache_meta_clear_io();
  for (i = 0; i < SD_CACHE_SLOTS; i += SD_CACHE_META_CHUNK_ENTRIES)
  {
    setup_spi_dram_write_block(SD_CACHE_META_BASE + ((u32)i * SD_CACHE_META_ENTRY_SIZE), sd_cache_io, SD_CACHE_IO_SIZE);
  }

  sd_cache_next_slot = 0;
  sd_cache_fat_valid = 0;
  sd_cache_fat_count = 0;
  sd_cache_fat_start = 0;
  sd_cache_fat_size = 0;
}

void sd_cache_parse_bpb(LBA_t sector, const u8 *buf)
{
  u16 bytes_per_sector;
  u16 reserved;
  u16 fat16_size;
  u32 fat_size;
  u8 fats;

  if (sd_cache_fat_valid) return;
  if ((buf[510] != 0x55) || (buf[511] != 0xAA)) return;

  bytes_per_sector = sd_cache_get_le16(buf + 11);
  if (bytes_per_sector != 512) return;

  reserved = sd_cache_get_le16(buf + 14);
  fats = buf[16];
  fat16_size = sd_cache_get_le16(buf + 22);
  fat_size = fat16_size ? fat16_size : sd_cache_get_le32(buf + 36);

  if (reserved == 0) return;
  if ((fats == 0) || (fats > 4)) return;
  if (fat_size == 0) return;

  sd_cache_fat_start = sector + reserved;
  sd_cache_fat_size = fat_size;
  sd_cache_fat_count = fats;
  sd_cache_fat_valid = 1;
}

u8 sd_cache_is_fat_sector(LBA_t sector)
{
  u8 i;
  u32 start;
  u32 end;

  if (sd_cache_fat_valid == 0) return 0;

  for (i = 0; i < sd_cache_fat_count; i++)
  {
    start = sd_cache_fat_start + ((u32)i * sd_cache_fat_size);
    end = start + sd_cache_fat_size;
    if ((sector >= start) && (sector < end)) return 1;
  }

  return 0;
}

u8 sd_cache_range_has_fat(LBA_t sector, UINT count)
{
  while (count--)
  {
    if (sd_cache_is_fat_sector(sector)) return 1;
    sector++;
  }

  return 0;
}

u32 sd_cache_slot_addr(u8 slot)
{
  return SD_CACHE_BASE + ((u32)slot * SD_CACHE_SLOT_SIZE);
}

u8 sd_cache_find_slot(LBA_t sector, u8 *slot)
{
  u8 base;
  u8 i;
  u8 *p;

  for (base = 0; base < SD_CACHE_SLOTS; base += SD_CACHE_META_CHUNK_ENTRIES)
  {
    setup_spi_dram_read_block(SD_CACHE_META_BASE + ((u32)base * SD_CACHE_META_ENTRY_SIZE), sd_cache_io, SD_CACHE_IO_SIZE);

    for (i = 0; i < SD_CACHE_META_CHUNK_ENTRIES; i++)
    {
      p = sd_cache_io + ((u16)i * SD_CACHE_META_ENTRY_SIZE);
      if ((p[SD_CACHE_META_FLAGS_OFS] & SD_CACHE_FLAG_VALID) && ((LBA_t)sd_cache_get_le32(p) == sector))
      {
        *slot = base + i;
        return 1;
      }
    }
  }

  return 0;
}

u8 sd_write_dram_sector(u8 slot, LBA_t sector)
{
  u8 resp;
  u8 retry = 16;
  u16 ofs;
  u32 addr;

  if ((ctype & CT_BLOCK) == 0) sector *= 512UL;
  if (sd_cmd(SDC_WRITE_BLOCK, sector)) return 0;

  addr = sd_cache_slot_addr(slot);
  sd_wr(0xFF);
  sd_wr(SD_TOKEN_WRITE);

  for (ofs = 0; ofs < 512; ofs += SD_CACHE_IO_SIZE)
  {
    setup_spi_dram_read_block(addr + ofs, sd_cache_io, SD_CACHE_IO_SIZE);
    sd_send(sd_cache_io, SD_CACHE_IO_SIZE);
  }

  sd_wr(0xFF);
  sd_wr(0xFF);

  do
  {
    resp = sd_rd();
  } while ((resp == 0xFF) && --retry);

  if ((resp & 0x1F) != 0x05) return 0;
  return sd_wait_busy_long();
}

u8 sd_cache_flush_slot(u8 slot)
{
  LBA_t sector;
  u8 flags;

  sd_cache_meta_get(slot, &sector, &flags);
  if ((flags & SD_CACHE_FLAG_VALID) == 0) return 1;
  if ((flags & SD_CACHE_FLAG_DIRTY) == 0) return 1;
  if (sd_write_dram_sector(slot, sector) == 0) return 0;
  sd_cache_meta_set_flags(slot, flags & (u8)~SD_CACHE_FLAG_DIRTY);
  return 1;
}

u8 sd_cache_flush_all()
{
  u8 i;

  for (i = 0; i < SD_CACHE_SLOTS; i++)
  {
    if (sd_cache_flush_slot(i) == 0) return 0;
  }

  return 1;
}

u8 sd_cache_alloc_slot(LBA_t sector, u8 *slot)
{
  u8 victim;

  if (sd_cache_find_slot(sector, slot)) return 1;

  victim = sd_cache_next_slot;
  sd_cache_next_slot++;
  if (sd_cache_next_slot >= SD_CACHE_SLOTS) sd_cache_next_slot = 0;

  if (sd_cache_flush_slot(victim) == 0) return 0;

  *slot = victim;
  sd_cache_meta_set(victim, sector, SD_CACHE_FLAG_VALID);
  return 1;
}

u8 sd_cache_read_sector(BYTE *buff, LBA_t sector)
{
  u8 slot;

  if (sd_cache_find_slot(sector, &slot) == 0) return 0;
  setup_spi_dram_read_block(sd_cache_slot_addr(slot), buff, 512);
  return 1;
}

u8 sd_cache_store_sector(const BYTE *buff, LBA_t sector, u8 dirty)
{
  u8 slot;

  if (sd_cache_alloc_slot(sector, &slot) == 0) return 0;
  setup_spi_dram_write_block(sd_cache_slot_addr(slot), buff, 512);
  sd_cache_meta_set(slot, sector, dirty ? (SD_CACHE_FLAG_VALID | SD_CACHE_FLAG_DIRTY) : SD_CACHE_FLAG_VALID);
  return 1;
}

DSTATUS disk_initialize(BYTE pdrv)
{
  DSTATUS rc;
  u16 i = 65000;

  if (pdrv != 0) return STA_NOINIT;

  rc = STA_NOINIT;
  ctype = CT_NONE;
  sd_sector_count = 0;
  sd_status = STA_NOINIT;
  sdcrc = 0x01;
  sd_cache_reset();

  sd_cs_off();
  sd_skip(512 + 10);
  sd_cs_on();

  if (sd_cmd(SDC_GO_IDLE_STATE, 0) > 1)
  {
    sd_reset_recover();
    sd_cs_on();
    if (sd_cmd(SDC_GO_IDLE_STATE, 0) > 1) goto exit;
  }

  if (sd_cmd(SDC_SEND_IF_COND, 0x1AA) == 1)
  {
    if ((sd_rbuf[2] != 0x01) || (sd_rbuf[3] != 0xAA)) goto exit;

    while ((sd_acmd(SDC_SD_SEND_OP_COND, 0x40000000UL)) && --i)
    {
    }
    if (i == 0) goto exit;

    if (sd_cmd(SDC_READ_OCR, 0) != 0) goto exit;
    ctype = (sd_rbuf[0] & 0x40) ? (CT_SDHC | CT_BLOCK) : CT_SD2;
  }
  else
  {
    if (sd_acmd(SDC_SD_SEND_OP_COND, 0) <= 1)
    {
      ctype = CT_SD1;
      while ((sd_acmd(SDC_SD_SEND_OP_COND, 0)) && --i)
      {
      }
    }
    else
    {
      ctype = CT_MMC;
      while ((sd_cmd(SDC_SEND_OP_COND_MMC, 0)) && --i)
      {
      }
    }

    if (i == 0)
    {
      ctype = CT_NONE;
      goto exit;
    }

    if (sd_cmd(SDC_SET_BLOCKLEN, 512))
    {
      ctype = CT_NONE;
      goto exit;
    }
  }

  if (sd_read_csd() == 0)
  {
    ctype = CT_NONE;
    goto exit;
  }

  rc = 0;

exit:
  sd_cs_off();
  sd_status = rc;
  return rc;
}

DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != 0) return STA_NOINIT;
  return sd_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
  if (pdrv != 0) return RES_PARERR;
  if (count == 0) return RES_PARERR;
  if (sd_status & STA_NOINIT) return RES_NOTRDY;

  sd_cs_on();
  while (count--)
  {
    if ((sd_cache_fat_valid == 0) || (sd_cache_is_fat_sector(sector) == 0) || (sd_cache_read_sector(buff, sector) == 0))
    {
      if (sd_read_sector(buff, sector) == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }

      sd_cache_parse_bpb(sector, buff);
      if (sd_cache_is_fat_sector(sector))
      {
        if (sd_cache_store_sector(buff, sector, 0) == 0)
        {
          sd_cs_off();
          return RES_ERROR;
        }
      }
    }

    buff += 512;
    sector++;
  }

  sd_cs_off();
  return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
  if (pdrv != 0) return RES_PARERR;
  if (count == 0) return RES_PARERR;
  if (sd_status & STA_NOINIT) return RES_NOTRDY;

  sd_cs_on();

  if ((sd_cache_fat_valid == 0) || (sd_cache_range_has_fat(sector, count) == 0))
  {
    while (count--)
    {
      if (count)
      {
        if (sd_write_multi(buff, sector, count + 1) == 0)
        {
          sd_cs_off();
          return RES_ERROR;
        }

        sd_cs_off();
        return RES_OK;
      }

      if (sd_write_sector(buff, sector) == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }

      buff += 512;
      sector++;
    }

    sd_cs_off();
    return RES_OK;
  }

  while (count--)
  {
    if (sd_cache_is_fat_sector(sector))
    {
      if (sd_cache_store_sector(buff, sector, 1) == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }
    }
    else
    {
      if (sd_write_sector(buff, sector) == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }
    }

    buff += 512;
    sector++;
  }

  sd_cs_off();
  return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  if (pdrv != 0) return RES_PARERR;
  if (sd_status & STA_NOINIT) return RES_NOTRDY;

  switch (cmd)
  {
    case CTRL_SYNC:
      sd_cs_on();
      if (sd_cache_flush_all() == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }
      if (sd_wait_busy() == 0)
      {
        sd_cs_off();
        return RES_ERROR;
      }
      sd_cs_off();
      return RES_OK;

    case GET_SECTOR_COUNT:
      *(LBA_t*)buff = sd_sector_count;
      return RES_OK;

    case GET_SECTOR_SIZE:
      *(WORD*)buff = 512;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *(DWORD*)buff = 1;
      return RES_OK;

    default:
      return RES_PARERR;
  }
}

DWORD get_fattime()
{
  return ((DWORD)(2026 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);
}
