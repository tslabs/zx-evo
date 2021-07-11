
#include <avr/io.h>
#include <string.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "spiflash.h"
#include "tsspiffs.h"
#include "config.h"

u32         sf_addr = 0;

SFFMT_TYPE  sffmt_type;

u16         sf_num_blocks;
u16         sf_num_pages;
bool        is_4_byte_addr;

SPIF_STATE  spif_state = SFST_IDLE;
SFFMT_STATE sffmt_state;
SFFMT_STATE sffmt_state_next;
bool        is_busy;
bool        is_error;
u8          progress;
u8          last_cmd;

#define SIZE_OF_PARAMS  256
#define SIZE_OF_REPORT  256

u8          *params;
u16         params_len;
u8          *report;
u16         report_len;
u16         report_cnt;

u16         sffmt_blk_cnt;
u16         sffmt_blk_good;
u16         sffmt_pg_cnt;
u32         sffmt_addr;
u8          sffmt_check_value;

void prepare_report()
{
  cfg_add_field(CFG_TAG_START, report, 0);
  cfg_add_field(CFG_TAG_REP_COMMAND, &last_cmd, sizeof(last_cmd));
  cfg_add_field(CFG_TAG_REP_PROGRESS, &progress, sizeof(progress));
  cfg_add_field(CFG_TAG_REP_ADDRESS, &sffmt_addr, sizeof(sffmt_addr));
  cfg_add_field(CFG_TAG_REP_BLOCKS, &sffmt_blk_cnt, sizeof(sffmt_blk_cnt));
  cfg_add_field(CFG_TAG_REP_BLOCKS_GOOD, &sffmt_blk_good, sizeof(sffmt_blk_good));
  report_len = cfg_add_field(CFG_TAG_END, 0, 0);
  report_cnt = 0;
}

void spif_detect()
{
  sf_num_blocks = 0;
  sf_num_pages = 16;  // default value for 4kB block

  sfi_enable();
  sfi_cs_off();
  sf_command(SPIFL_CMD_ID);
  u8 rc = spi_flash_read(SPIFL_REG_DATA);
  sfi_cs_off();
  sfi_disable();

  if ((rc >= 0x12) && (rc <= 0x17))   // 512k..16M
    is_4_byte_addr = false;
  else if (rc >= 0x18)  // 32M and more
  {
    is_4_byte_addr = true;
    sfi_cmd(SF_CMD_EN4B);
  }
  else
    return;     // wrong flash

  sf_num_blocks = (u16)1 << (rc - 11);
}

void spif_init()
{
  params     = dbuf;
  params_len = 0;
  report     = &dbuf[256];
  report_len = 0;
  report_cnt = 0;
  spif_state = SFST_IDLE;
  last_cmd   = 0;
  progress   = 0;
  is_busy    = false;
  is_error   = false;
}

void spif_start_format(SFFMT_TYPE type)
{
  sffmt_type    = type;
  spif_state    = SFST_FORMAT;
  sffmt_state   = (sffmt_type == SFFM_CHIP) ? SFFMT_ST_FULL_ERASE : SFFMT_ST_1;
  sffmt_blk_cnt = 0;
  sffmt_blk_good = 0;
  sffmt_addr    = 0;
  progress      = 0;
  is_busy       = true;
}

void sfi_cs_off()
{
  PORTF |= _BV(SFI_BIT_NCSO);
}

void sfi_cs_on()
{
  PORTF |= _BV(SFI_BIT_DCLK);
  PORTF &= ~_BV(SFI_BIT_NCSO);
}

// Enable SFI, disable JTAG
void sfi_enable()
{
  u8 m = MCUCSR | _BV(JTD);

  MCUCSR = m; MCUCSR = m;
  DDRF   = (DDRF & ~_BV(SFI_BIT_DATA0)) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_ASDO) | _BV(SFI_BIT_DCLK);
  PORTF |= _BV(SFI_BIT_DATA0) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_DCLK);
}

// Disable SFI, enable JTAG
void sfi_disable()
{
  u8 m = MCUCSR & ~_BV(JTD);

  MCUCSR = m; MCUCSR = m;
}

void sfi_send(u8 d)
{
  #define sfi_send_bit(a)    if (d & (a)) { PORTF = c01; PORTF = c11; } else { PORTF = c00; PORTF = c10; }

  u8 c00 = PORTF & ~(_BV(SFI_BIT_DCLK) | _BV(SFI_BIT_ASDO));
  u8 c01 = c00 | _BV(SFI_BIT_ASDO);
  u8 c10 = c00 | _BV(SFI_BIT_DCLK);
  u8 c11 = c01 | _BV(SFI_BIT_DCLK);

  sfi_send_bit(128);
  sfi_send_bit(64);
  sfi_send_bit(32);
  sfi_send_bit(16);
  sfi_send_bit(8);
  sfi_send_bit(4);
  sfi_send_bit(2);
  sfi_send_bit(1);

  #undef sfi_send_bit
}

u8 sfi_recv()
{
  #define sfi_recv_bit(a)    PORTF = c0; PORTF = c1; if (PINF & _BV(SFI_BIT_DATA0)) d |= (a)

  u8 d = 0;
  u8 c0 = PORTF & ~_BV(SFI_BIT_DCLK);
  u8 c1 = c0 | _BV(SFI_BIT_DCLK);

  sfi_recv_bit(128);
  sfi_recv_bit(64);
  sfi_recv_bit(32);
  sfi_recv_bit(16);
  sfi_recv_bit(8);
  sfi_recv_bit(4);
  sfi_recv_bit(2);
  sfi_recv_bit(1);

  #undef sfi_recv_bit

  return d;
}

void sfi_cmd_ha(u8 c)
{
  sfi_cs_on();
  sfi_send(c);
  if (is_4_byte_addr) sfi_send((u8)(sf_addr >> 24));
  sfi_send((u8)(sf_addr >> 16));
  sfi_send((u8)(sf_addr >> 8));
  sfi_send((u8)(sf_addr));
}

void sfi_cmd(u8 c)
{
  sfi_cs_on();
  sfi_send(c);
  sfi_cs_off();
}

u8 sfi_cmd_r(u8 c)
{
  sfi_cs_on();
  sfi_send(c);
  c = sfi_recv();
  sfi_cs_off();
  return c;
}

void sfi_cmd_w(u8 c, u8 d)
{
  sfi_cs_on();
  sfi_send(c);
  sfi_send(d);
  sfi_cs_off();
}

void sfi_wren()
{
  sfi_cmd(SF_CMD_WREN);
}

u8 sf_status()
{
  if (is_busy)
    return SPIFL_STAT_BUSY;
  else if (sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_BUSY)
    return SPIFL_STAT_BUSY;
  else if (is_error)
    return SPIFL_STAT_ERR;
  else
    return SPIFL_STAT_NULL;
}

// Execute SF command
void sf_command(u8 cmd)
{
  is_error = false;

  if (cmd == SPIFL_CMD_REPORT)
    prepare_report();

  else if (is_busy)
    switch (cmd)
    {
      case SPIFL_CMD_BREAK:
        is_busy = false;
        // +++ terminate command
        break;

      default:
        is_error = true;
    }

  else
  {
    switch (cmd)
    {
      case SPIFL_CMD_ENA:
        sfi_enable();
        break;

      case SPIFL_CMD_DIS:
        sfi_disable();
        break;

      case SPIFL_CMD_END:
        sfi_cs_off();
        return;

      case SPIFL_CMD_DETECT:
        spif_detect();
        return;

      case SPIFL_CMD_ID:
        sfi_cmd_ha(SF_CMD_RDID);  // 3 dummy bytes
        // must be terminated after data transfer
        break;

      case SPIFL_CMD_READ:
        sfi_cmd_ha(SF_CMD_RD);
        // must be terminated after data transfer
        break;

      case SPIFL_CMD_WRITE:
        sfi_wren();
        sfi_cmd_ha(SF_CMD_WR);
        // must be terminated after data transfer
        break;

      case SPIFL_CMD_ERSCHP:
        sfi_wren();
        sfi_cmd(SF_CMD_ERCHIP);
        break;

      case SPIFL_CMD_ERSSEC:
        sfi_wren();
        sfi_cmd_ha(SF_CMD_ERSECT);
        sfi_cs_off();
        break;

      case SPIFL_CMD_F_CHIP:
        spif_start_format(SFFM_CHIP);
        break;

      case SPIFL_CMD_F_BLK:
        spif_start_format(SFFM_BLK);
        break;

      case SPIFL_CMD_F_CHK:
        spif_start_format(SFFM_CHK);
        break;

      case SPIFL_CMD_BSLOAD:
          is_cold_reset       = false;
          flags_register     |= FLAG_HARD_RESET;
        break;

      default:
        is_error = true;
        break;
    }

    last_cmd = cmd;
  }

  params_len = 0;
}

u8 spi_flash_read(u8 index)
{
  switch (index)
  {
    // SF data
    case SPIFL_REG_DATA:
      sf_addr++;
      return sfi_recv();

    // SF status
    case SPIFL_REG_STAT:
      return sf_status();

    // SF version
    case SPIFL_REG_VER:
      return SPIFL_VER;

    // Report
    case SPIFL_REG_REPORT:
    {
      u8 rc = report[report_cnt];
      if (report_cnt < (report_len - 1))
        report_cnt++;
      return rc;
    }
  }

  return 0xFF;
}

void spi_flash_write(u8 index, u8 data)
{
  switch (index)
  {
    // SF data
    case SPIFL_REG_DATA:
      sf_addr++;
      sfi_send(data);
      break;

    // SF command
    case SPIFL_REG_CMD:
      sf_command(data);
      break;

    // select another Glu Ext register
    case SPIFL_REG_EXTSW:
      ext_type_gluk = data;
      break;

    // SF addr
    case SPIFL_REG_ADDR:
      sf_addr <<= 8;
      sf_addr |= data;
      break;

    // Command parameter
    case SPIFL_REG_PARAM:
      params[params_len] = data;
      if (params_len < (SIZE_OF_PARAMS - 1))
        params_len++;
      break;
  }
}

// Format:
// 1. Erase block
// 2. Check for 0xFF
// 3. Write with 0x55
// 4. Check for 0x55
// 5. Erase block
// 6. Check for 0xFF
// 7. Write with 0xAA
// 8. Check for 0xAA
// 9. Erase block
// 10. Check for 0xFF
// 11. Write chunk signature
void spif_format()
{
  switch (sffmt_state)
  {
    case SFFMT_ST_1:
      sffmt_state      = SFFMT_ST_ERASE;
      sffmt_state_next = SFFMT_ST_TEST_55;
      break;

    case SFFMT_ST_TEST_55:
      sffmt_check_value = 0x55;
      sffmt_pg_cnt      = 0;
      sffmt_state       = SFFMT_ST_WRITE;
      sffmt_state_next  = SFFMT_ST_2;
      break;

    case SFFMT_ST_2:
      sffmt_state      = SFFMT_ST_ERASE;
      sffmt_state_next = SFFMT_ST_TEST_AA;
      break;

    case SFFMT_ST_TEST_AA:
      sffmt_check_value = 0xAA;
      sffmt_pg_cnt      = 0;
      sffmt_state       = SFFMT_ST_WRITE;
      sffmt_state_next  = SFFMT_ST_3;
      break;

    case SFFMT_ST_3:
      sffmt_state      = SFFMT_ST_ERASE;
      sffmt_state_next = SFFMT_ST_WRITE_SIG;
      break;

    case SFFMT_ST_WRITE:
      sf_addr = sffmt_addr + (u32)(sffmt_pg_cnt << 8);
      sfi_wren();
      sfi_cmd_ha(SF_CMD_WR);
      for (u16 i = 0; i < 256; i++)
        sfi_send(sffmt_check_value);
      sfi_cs_off();
      sffmt_state = SFFMT_ST_WRITE2;
      break;

    case SFFMT_ST_WRITE2:
      if (!(sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_BUSY))
      {
        sffmt_pg_cnt++;

        if (sffmt_pg_cnt < sf_num_pages)
          sffmt_state = SFFMT_ST_WRITE;
        else
          sffmt_state = SFFMT_ST_CHECK;
      }
      break;

    case SFFMT_ST_WRITE_SIG:
      sf_addr = sffmt_addr;
      sfi_wren();
      sfi_cmd_ha(SF_CMD_WR);
      {
        const u32 sig = TSF_MAGIC;
        const u8  *s  = (u8*)&sig;
        for (u8 i = 0; i < 4; i++)
          sfi_send(s[i]);
      }
      sfi_cs_off();
      sffmt_blk_good++;
      sffmt_state = SFFMT_ST_WRITE_SIG2;
      break;

    case SFFMT_ST_WRITE_SIG2:
      if (!(sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_BUSY))
        sffmt_state = SFFMT_ST_BLOCK_DONE;
      break;

    case SFFMT_ST_FULL_ERASE:
      sfi_wren();
      sfi_cmd(SF_CMD_ERCHIP);
      sffmt_state = SFFMT_ST_ERASE2;
      break;

    case SFFMT_ST_ERASE:
      sf_addr = sffmt_addr;
      sfi_wren();
      sfi_cmd_ha(SF_CMD_ERSECT);
      sfi_cs_off();
      sffmt_state = SFFMT_ST_ERASE2;
      break;

    case SFFMT_ST_ERASE2:
      if (!(sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_BUSY))
      {
        if (sffmt_type == SFFM_CHK)
        {
          sffmt_check_value = 0xFF;
          sffmt_state       = SFFMT_ST_CHECK;
        }
        else
          sffmt_state = SFFMT_ST_WRITE_SIG;
      }
      break;

    case SFFMT_ST_CHECK:
    {
      bool is_ok = true;

      sf_addr = sffmt_addr;
      sfi_cmd_ha(SF_CMD_RD);
      for (u16 i = 0; i < 4096; i++)
        if (sfi_recv() != sffmt_check_value)
        {
          is_ok = false;
          break;
        }
      sfi_cs_off();

      if (is_ok)
        sffmt_state = sffmt_state_next;
      else
        sffmt_state = SFFMT_ST_BLOCK_DONE;
    }
    break;

    case SFFMT_ST_BLOCK_DONE:
      sffmt_blk_cnt++;
      progress    = ((u32)sffmt_blk_cnt * 255) / sf_num_blocks;

      if (sffmt_blk_cnt < sf_num_blocks)
      {
        sffmt_addr += sf_num_pages << 8;
        sffmt_state = (sffmt_type == SFFM_CHIP) ? SFFMT_ST_WRITE_SIG : SFFMT_ST_1;
      }
      else
      {
        is_busy    = false;
        spif_state = SFST_IDLE;
      }
    break;
  }
}

void spif_task()
{
  switch (spif_state)
  {
    case SFST_FORMAT:
      spif_format();
      break;

    default:
      break;
  }
}
