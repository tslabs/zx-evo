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
u8 spi_send(u8 byte);

#endif

