#include <avr/io.h>
#include <avr/interrupt.h>

#include "mytypes.h"
#include "rs232.h"
#include "pins.h"

#define FOSC 11059200// Clock Speed
#define BAUD115200 115200
#define UBRR115200 (((FOSC/16)/BAUD115200)-1)

void rs232_init(void)
{
	// Set baud rate
	UBRR1H = (UBYTE)(UBRR115200>>8);
	UBRR1L = (UBYTE)UBRR115200;
	// Clear reg
	UCSR1A = 0;
	// Enable receiver and transmitter
	UCSR1B = (1<<RXEN)|(1<<TXEN);
	// Set frame format: 8data, 1stop bit
	UCSR1C = (1<<USBS)|(1<<UCSZ0)|(1<<UCSZ1);
	// Set TXD pin
	//RS232TXD_DDR |= (1<<RS232TXD);
}

void rs232_transmit( UBYTE data )
{
	// Wait for empty transmit buffer
	while ( !( UCSR1A & (1<<UDRE)) );
	// Put data into buffer, sends the data
	UDR1 = data;
}

#ifdef LOGENABLE
void to_log(char* ptr)
{
	while( (*ptr)!=0 )
	{
		rs232_transmit(*ptr);
		ptr++;
	}
}
#endif
