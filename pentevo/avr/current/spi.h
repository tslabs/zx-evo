#ifndef SPI_H
#define SPI_H

/**
 * @file
 * @brief SPI interface support.
 * @author http://www.nedopc.com
 *
 * SPI interface to FPGA.
 */

/** Init SPI interface in legacy LSB-first mode. Used for FPGA bitstream upload. */
#define spi_set_lsb() {SPCR=0b01110000;SPSR=0b00000001;}

/** Init SPI interface in MSB-first mode. Used by SETUP_CONF runtime SPI protocol. */
#define spi_set_msb() {SPCR=0b01010000;SPSR=0b00000001;}

/** Init spi interface. */
#define spi_init() spi_set_lsb()

/**
 * SPI data interchange.
 * @return received data
 * @param byte [in] - data to send
 */
#define spi_send(byte) ({SPDR=(byte);while(!(SPSR&(1<<SPIF)));SPDR;})

#endif

