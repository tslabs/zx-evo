#ifndef SPI_H
#define SPI_H

/**
 * @file
 * @brief SPI interface support.
 * @author http://www.nedopc.com
 *
 * SPI interface to FPGA.
 */

/** Init spi interface. */
#define spi_init() {SPCR=0b01110000;SPSR=0b00000001;}

/**
 * SPI data interchange.
 * @return received data
 * @param byte [in] - data to send
 */
#define spi_send(byte) ({SPDR=(byte);while(!(SPSR&(1<<SPIF)));SPDR;})

#endif

