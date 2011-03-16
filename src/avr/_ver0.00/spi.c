#include <avr/io.h>
#include <avr/interrupt.h>
#include "pins.h"
#include "mytypes.h"


void spi_init(void)
{
	SPCR = 0b01110000; // prepare SPI
	SPSR = 0b00000001;
}

void spi_send(UBYTE byte)
{
		SPDR = byte;
		while( !( SPSR&(1<<SPIF) ) );
}

