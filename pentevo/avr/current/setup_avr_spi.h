#pragma once

/*
 * SETUP minimal AVR-controlled fork SPI map.
 *
 * External controller side:
 *   - AVR/host is SPI master.
 *   - FPGA is SPI slave.
 *   - SPI mode 0, LSB first inside each byte, target clock 6 MHz.
 *   - Every FPGA transaction starts with a 24-bit header while AVR CS is low.
 *
 * Header format:
 *   bit 23..22 = operation / space
 *   bit 21..0  = byte address
 *
 * Header byte order on the wire:
 *   byte0 = {op[1:0], addr[21:16]}
 *   byte1 = addr[15:8]
 *   byte2 = addr[7:0]
 *
 * After the header, bytes stream until AVR CS goes high.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SETUP_SPI_HEADER_LEN          3u
#define SETUP_SPI_ADDR_MASK           0x003FFFFFul
#define SETUP_SPI_ADDR_BITS           22u

#define SETUP_SPI_OP_DRAM_WR          0x00u
#define SETUP_SPI_OP_DRAM_RD          0x01u
#define SETUP_SPI_OP_PERIPH_WR        0x02u
#define SETUP_SPI_OP_PERIPH_RD        0x03u

#define SETUP_SPI_HDR0(op, addr)      ((uint8_t)((((uint8_t)(op) & 0x03u) << 6) | (((uint32_t)(addr) >> 16) & 0x3Fu)))
#define SETUP_SPI_HDR1(addr)          ((uint8_t)(((uint32_t)(addr) >> 8) & 0xFFu))
#define SETUP_SPI_HDR2(addr)          ((uint8_t)((uint32_t)(addr) & 0xFFu))

#define SETUP_SPI_PUT_HEADER(buf, op, addr) \
  do \
  { \
    (buf)[0] = SETUP_SPI_HDR0((op), (addr)); \
    (buf)[1] = SETUP_SPI_HDR1((addr)); \
    (buf)[2] = SETUP_SPI_HDR2((addr)); \
  } while (0)

/*
 * DRAM access.
 *
 * Operation:
 *   SETUP_SPI_OP_DRAM_WR / SETUP_SPI_OP_DRAM_RD
 * Address:
 *   22-bit byte address, auto-increments after every transferred byte.
 *
 * FPGA internal DRAM is 16-bit:
 *   dram_word_addr = byte_addr[21:1]
 *   byte_addr[0] = 0 -> low byte  [7:0]
 *   byte_addr[0] = 1 -> high byte [15:8]
 */
#define SETUP_DRAM_BASE               0x000000ul
#define SETUP_DRAM_SIZE               0x400000ul
#define SETUP_DRAM_END                0x3FFFFFul
#define SETUP_DRAM_ADDR(addr)         ((uint32_t)(addr) & SETUP_SPI_ADDR_MASK)
#define SETUP_DRAM_WORD_ADDR(addr)    (((uint32_t)(addr) >> 1) & 0x001FFFFFul)
#define SETUP_DRAM_BYTE_LOW(addr)     ((((uint32_t)(addr) & 1u) == 0u) ? 1u : 0u)
#define SETUP_DRAM_BYTE_HIGH(addr)    ((((uint32_t)(addr) & 1u) != 0u) ? 1u : 0u)

/*
 * Peripheral space.
 *
 * Operation:
 *   SETUP_SPI_OP_PERIPH_WR / SETUP_SPI_OP_PERIPH_RD
 */
#define SETUP_PERIPH_REG_BASE         0x000000ul
#define SETUP_PERIPH_REG_END          0x0000FFul
#define SETUP_PERIPH_CRAM_BASE        0x000100ul
#define SETUP_PERIPH_CRAM_END         0x0001FFul
#define SETUP_PERIPH_PROXY_ADDR       0x000057ul
#define SETUP_PERIPH_SPI_CFG_ADDR     0x000077ul

/*
 * Current fork HDL note:
 *   avr_regs.v decodes CRAM window at 0x100..0x1FF and uses addr[8:1]
 *   as CRAM index. This exposes indices 0x80..0xFF as written in that HDL.
 *   If HDL is changed to use addr[7:1], SETUP_CRAM_ADDR(index) becomes the
 *   expected full 0..127 pair window, or the window must be extended to 0x2FF.
 */
#define SETUP_CRAM_ADDR(index)        (SETUP_PERIPH_CRAM_BASE + (((uint32_t)(index) & 0x7Fu) << 1))
#define SETUP_CRAM_LOW_ADDR(index)    (SETUP_CRAM_ADDR(index) + 0u)
#define SETUP_CRAM_HIGH_ADDR(index)   (SETUP_CRAM_ADDR(index) + 1u)

/*
 * Preserved config/video register numbers.
 */
#define SETUP_REG_VCONF               0x000000ul
#define SETUP_REG_VPAGE               0x000001ul
#define SETUP_REG_GXOFFSL             0x000002ul
#define SETUP_REG_GXOFFSH             0x000003ul
#define SETUP_REG_GYOFFSL             0x000004ul
#define SETUP_REG_GYOFFSH             0x000005ul
#define SETUP_REG_PALSEL              0x000007ul
#define SETUP_REG_XBORDER             0x00000Ful
#define SETUP_REG_SYSCONF             0x000020ul
#define SETUP_REG_SDDAT               SETUP_PERIPH_PROXY_ADDR
#define SETUP_REG_SDCFG               SETUP_PERIPH_SPI_CFG_ADDR

/* Removed/not implemented in this fork: HSINT, VSINTL, VSINTH, MEMCONF,
 * CACHECONF, INTMASK, DMA, FDD/VG93, TSU/tile/sprite registers.
 */

/* VCONF bits used by the minimal video fork. */
#define SETUP_VCONF_MODE_MASK         0x03u
#define SETUP_VCONF_MODE_256C         0x02u
#define SETUP_VCONF_MODE_TEXT         0x03u
#define SETUP_VCONF_VDAC2_MSEL        0x04u
#define SETUP_VCONF_NOGFX             0x20u
#define SETUP_VCONF_RRES_SHIFT        6u
#define SETUP_VCONF_RRES_MASK         0xC0u
#define SETUP_VCONF_DEFAULT_TEXT      0x83u

#define SETUP_VPAGE_DEFAULT_TEXT      0x00u
#define SETUP_PALSEL_DEFAULT          0x0Fu
#define SETUP_XBORDER_DEFAULT         0x00u

/* SYSCONF bits connected in top.v. */
#define SETUP_SYSCONF_VGA_ON          0x01u
#define SETUP_SYSCONF_60HZ            0x10u

/*
 * Text mode memory layout for VPAGE = 0x00:
 *   symbols/attrs -> DRAM page 0
 *   font          -> DRAM page 1
 *
 * Text fetch addresses inside video_mode.v:
 *   char codes:  {vpage[0], row[8:3], 1'b0, col[7:2]}
 *   attributes:  {vpage[0], row[8:3], 1'b1, col[7:2]}
 *   font:        {~vpage[0], 3'b000, char[7:0], row[2:1]}
 */
#define SETUP_TEXT_DEFAULT_VPAGE      0x00u
#define SETUP_TEXT_CHARS_PAGE         0u
#define SETUP_TEXT_FONT_PAGE          1u

/*
 * 256c mode memory layout:
 *   video byte address = {VPAGE[7:4], row[8:0], col[7:0]}
 */
#define SETUP_256C_PAGE_SHIFT         17u
#define SETUP_256C_PAGE_SIZE          0x020000ul
#define SETUP_256C_PAGE_ADDR(page)    ((((uint32_t)(page) & 0x0Fu) << SETUP_256C_PAGE_SHIFT) & SETUP_SPI_ADDR_MASK)

/*
 * 0x77 write format from avr_regs.v.
 * Internal spi_cs_n = {~D4, ~D3, ~D2, D1} = {ESP, SD2, FT, SD}.
 *
 * Write-side target select values below select one target and release SD when
 * the target is not SD. Target CS state remains latched after 0x57 transaction.
 */
#define SETUP_SDCFG_MODE_SELECT       0x01u
#define SETUP_SDCFG_SD_RELEASE        0x02u
#define SETUP_SDCFG_FT_SELECT         0x04u
#define SETUP_SDCFG_SD2_SELECT        0x08u
#define SETUP_SDCFG_ESP_SELECT        0x10u
#define SETUP_SDCFG_MODE_VALUE        0x80u
#define SETUP_SDCFG_ESP_FT_SPI_DIS    0x80u

#define SETUP_SDCFG_TARGET_SD         0x00u
#define SETUP_SDCFG_TARGET_NONE       SETUP_SDCFG_SD_RELEASE
#define SETUP_SDCFG_TARGET_FT         (SETUP_SDCFG_SD_RELEASE | SETUP_SDCFG_FT_SELECT)
#define SETUP_SDCFG_TARGET_SD2        (SETUP_SDCFG_SD_RELEASE | SETUP_SDCFG_SD2_SELECT)
#define SETUP_SDCFG_TARGET_ESP        (SETUP_SDCFG_SD_RELEASE | SETUP_SDCFG_ESP_SELECT)

/* 0x77 readback active-target mask bits from ~spi_cs_n. */
#define SETUP_SDCFG_ACTIVE_SD         0x01u
#define SETUP_SDCFG_ACTIVE_FT         0x02u
#define SETUP_SDCFG_ACTIVE_SD2        0x04u
#define SETUP_SDCFG_ACTIVE_ESP        0x08u
#define SETUP_SDCFG_READ_SPI_MODE     0x80u
#define SETUP_SDCFG_READ_SPI_MODE_ESP 0x40u
#define SETUP_SDCFG_READ_ESPCS_INT    0x80u

/*
 * SPI proxy window 0x57.
 *
 * Sequence:
 *   1. Peripheral write SETUP_REG_SDCFG with one SETUP_SDCFG_TARGET_* value.
 *   2. Peripheral write/stream SETUP_REG_SDDAT.
 *   3. Bytes after the 24-bit header are directly proxied:
 *        AVR SCK  -> target SCK
 *        AVR MOSI -> target MOSI
 *        target MISO -> AVR MISO
 *   4. AVR CS high ends only this proxy transaction. Target CS remains latched
 *      by 0x77 until SETUP_REG_SDCFG is written again.
 *
 * For SD reads, transmit 0xFF dummy bytes and sample returned bytes.
 */
#define SETUP_PROXY_DUMMY_BYTE        0xFFu

/*
 * ROM/flash programmer window in peripheral space.
 * Operation:
 *   SETUP_SPI_OP_PERIPH_WR / SETUP_SPI_OP_PERIPH_RD
 * Address:
 *   0x200000..0x3FFFFF, auto-increments after every byte.
 *
 * ROM bus mapping in FPGA:
 *   a[15:0]  = rom_addr[15:0]
 *   rompg*   = rom_addr[20:16]
 *   d[7:0]   = driven by FPGA only on ROM write, Z on read/idle.
 */
#define SETUP_ROM_PERIPH_BASE         0x200000ul
#define SETUP_ROM_PERIPH_END          0x3FFFFFul
#define SETUP_ROM_SIZE                0x200000ul
#define SETUP_ROM_ADDR_MASK           0x1FFFFFul
#define SETUP_ROM_ADDR(addr)          (SETUP_ROM_PERIPH_BASE | ((uint32_t)(addr) & SETUP_ROM_ADDR_MASK))
#define SETUP_ROM_PAGE(addr)          (((uint32_t)(addr) >> 16) & 0x1Fu)
#define SETUP_ROM_A16(addr)           (((uint32_t)(addr) >> 16) & 0x01u)
#define SETUP_ROM_A17(addr)           (((uint32_t)(addr) >> 17) & 0x01u)
#define SETUP_ROM_A18(addr)           (((uint32_t)(addr) >> 18) & 0x01u)
#define SETUP_ROM_A19(addr)           (((uint32_t)(addr) >> 19) & 0x01u)
#define SETUP_ROM_A20(addr)           (((uint32_t)(addr) >> 20) & 0x01u)

/* Host driver should provide these primitive operations in its own source:
 *   cs low
 *   transfer one byte full-duplex
 *   cs high
 *
 * Recommended read rule for DRAM/ROM/peripheral reads:
 *   after the 24-bit header, clock one dummy byte before treating returned
 *   data as valid if the host cannot pause SCK after the header.
 */

#ifdef __cplusplus
}
#endif

