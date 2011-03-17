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
void spi_init(void);

/**
 * SPI data interchange.
 * @return received data
 * @param byte [in] - data to send
 */
UBYTE spi_send(UBYTE byte);

#endif

