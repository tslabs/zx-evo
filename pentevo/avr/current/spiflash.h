#ifndef __SPIFLASH_H__
#define __SPIFLASH_H__

/**
 * @file
 * @brief SPI Flash interface.
 * @author TS-Labs
 *
 * SPI Flash interface.
 */
enum
{
  SPIFL_VER = 0x01
};

enum
{
  SF_ST_NONE = 0,
  SF_ST_IDLE,
  SF_ST_BUSY,
  SF_ST_FBUSY,
  SF_ST_ERR
};

#define SFI_BIT_NCSO                  5
#define SFI_BIT_ASDO                  4
#define SFI_BIT_DCLK                  7
#define SFI_BIT_DATA0                 6

extern u32 sf_addr;
extern u16 sf_num_blocks;

// Register number is masked with 0x0F
/** SFI registers. */
enum
{
  SPIFL_REG_EXTSW = 0x00,     // W
  SPIFL_REG_STAT  = 0x01,     // R
  SPIFL_REG_CMD   = 0x01,     // W
  SPIFL_REG_A0    = 0x02,     // RW
  SPIFL_REG_A1    = 0x03,     // RW
  SPIFL_REG_A2    = 0x04,     // RW
  SPIFL_REG_DATA  = 0x08,     // RW
  SPIFL_REG_PRGRS = 0x09,     // R
  SPIFL_REG_PARAM = 0x0A,     // W
  SPIFL_REG_VER   = 0x0F,     // R
};

/** SFI status bits. */
enum
{
  SPIFL_STAT_NULL = 0x00,
  SPIFL_STAT_BUSY = 0x01,
  SPIFL_STAT_ERR  = 0x02,
};

/** SFI commands. */
enum
{
  SPIFL_CMD_NOP    = 0x00,
  SPIFL_CMD_ENA    = 0x01,
  SPIFL_CMD_DIS    = 0x02,
  SPIFL_CMD_END    = 0x03,
  SPIFL_CMD_ID     = 0x04,
  SPIFL_CMD_READ   = 0x05,
  SPIFL_CMD_WRITE  = 0x06,
  SPIFL_CMD_ERSCHP = 0x07,
  SPIFL_CMD_ERSSEC = 0x08,
  SPIFL_CMD_BREAK  = 0x09,
  SPIFL_CMD_FORMAT = 0x0A,
  SPIFL_CMD_FMTCHK = 0x0B,
  SPIFL_CMD_FMTFST = 0x0C,
  SPIFL_CMD_BSLOAD = 0x0D,
};

/** SPI Flash commands. */
enum
{
  SF_CMD_RD     = 0x03,
  SF_CMD_RDFAST = 0x0B,
  SF_CMD_RDSTAT = 0x05,
  SF_CMD_RDID   = 0xAB,
  SF_CMD_RDID2  = 0x9F,  // for only Altera EPCS128
  SF_CMD_WR     = 0x02,
  SF_CMD_WRSTAT = 0x01,
  SF_CMD_WREN   = 0x06,
  SF_CMD_WRDIS  = 0x04,
  SF_CMD_ERCHIP = 0xC7,
  SF_CMD_ERSECT = 0x20,
};

/** SPI Flash status. */
enum
{
  SF_STAT_BUSY  = 0x01,
  SF_STAT_WEL   = 0x02,
};

typedef enum
{
  SFFMT_ST_1,
  SFFMT_ST_2,
  SFFMT_ST_3,
  SFFMT_ST_TEST_55,
  SFFMT_ST_TEST_AA,
  SFFMT_ST_WRITE_SIG,
  SFFMT_ST_WRITE_SIG2,
  SFFMT_ST_FULL_ERASE,
  SFFMT_ST_ERASE,
  SFFMT_ST_ERASE2,
  SFFMT_ST_WRITE,
  SFFMT_ST_WRITE2,
  SFFMT_ST_CHECK,
  SFFMT_ST_BLOCK_DONE
} SFFMT_STATE;

typedef enum
{
  SFST_IDLE,
  SFST_FORMAT
} SPIF_STATE;

typedef enum
{
  SFFM_NORM,
  SFFM_TEST,
  SFFM_FAST
} SFFMT_TYPE;

/** SF interface enable. */
void sfi_enable();
/** SF interface disable. */
void sfi_disable();
/** SF CS disable. */
void sfi_cs_off();
/** SF CS enable. */
void sfi_cs_on();
/** SF command. */
void sf_command(u8);
/** SF read. */
u8 spi_flash_read(u8 index);
/** SF write. */
void spi_flash_write(u8 index, u8 data);

void spif_init();
void spif_detect();
void spif_task();

#endif //__SPIFLASH_H__
