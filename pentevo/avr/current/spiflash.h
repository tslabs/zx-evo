#ifndef __SPIFLASH_H__
#define __SPIFLASH_H__

/**
 * @file
 * @brief SPI Flash interface.
 * @author TS-Labs
 *
 * SF interface.
 */

#define SPIFL_VER                     0x01

enum
{
    SF_ST_INACT = 0,
    SF_ST_IDLE,
    SF_ST_BUSY,
    SF_ST_FBUSY,
    SF_ST_ERR
};

#define SFI_BIT_NCSO                  5
#define SFI_BIT_ASDO                  4
#define SFI_BIT_DCLK                  7
#define SFI_BIT_DATA0                 6

// Registers are located at F0-FF addresses, but masked with 0x0F in functions

/** SF interface registers. */
#define SPIFL_REG_EXTSW               0x00      // W
#define SPIFL_REG_STAT                0x01      // R
#define SPIFL_REG_CTRL                0x01      // W
#define SPIFL_REG_A0                  0x02      // RW
#define SPIFL_REG_A1                  0x03      // RW
#define SPIFL_REG_A2                  0x04      // RW
#define SPIFL_REG_DATA                0x08      // RW
#define SPIFL_REG_VER                 0x0F      // R

/** SF status bits. */
#define SPIFL_STAT_NULL               0x00
#define SPIFL_STAT_BSY                0x01
#define SPIFL_STAT_ERR                0x02
#define SPIFL_STAT_ACT                0x80
                                      
/** SF control bits. */               
#define SPIFL_MASK_EN                 0x80
#define SPIFL_MASK_CMD                0x0F
                                      
/** SF commands. */                   
#define SPIFL_CMD_NOP                 0x00
#define SPIFL_CMD_DONE                0x01
#define SPIFL_CMD_ID                  0x02
#define SPIFL_CMD_READ                0x03
#define SPIFL_CMD_WRITE               0x04
#define SPIFL_CMD_ERSBLK              0x05
#define SPIFL_CMD_ERSSEC              0x06

/** SPI Flash commands. */
#define SF_CMD_RD                     0x03
#define SF_CMD_RDFAST                 0x0B
#define SF_CMD_RDSTAT                 0x05
#define SF_CMD_RDID                   0xAB
#define SF_CMD_RDID2                  0x9F  // for only Altera EPCS128
#define SF_CMD_WR                     0x02
#define SF_CMD_WRSTAT                 0x01
#define SF_CMD_WREN                   0x06
#define SF_CMD_WRDIS                  0x04
#define SF_CMD_ERBULK                 0xC7
#define SF_CMD_ERSECT                 0xD8

/** SPI Flash status. */
#define SF_STAT_MASK_WIP              0x01
#define SF_STAT_MASK_BPRT             0x1C  // for 4-128 Mbit, add here other flashes

/** SF read. */
u8 spi_flash_read(u8 index);
/** SF write. */
void spi_flash_write(u8 index, u8 data);

#endif //__SPIFLASH_H__
