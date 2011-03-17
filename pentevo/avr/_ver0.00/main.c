#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>


#include "mytypes.h"
#include "depacker_dirty.h"
#include "getfaraddress.h"
#include "pins.h"

#include "ps2.h"
#include "zx.h"
#include "spi.h"





extern const char fpga[] PROGMEM; // linker symbol


ULONG indata;
UBYTE dbuf[DBSIZE];





void InitHardware(void);


int main()
{
	UBYTE j;


	InitHardware();




	PORTF |= (1<<PF3); // turn POWER on

	j=50;
	do _delay_ms(20); while(--j); //1 sec delay


	//begin configuring, led ON
	PORTB &= ~(1<<LED);


	spi_init();



	DDRF |= (1<<nCONFIG); // pull low for a time
	_delay_us(40);
	DDRF &= ~(1<<nCONFIG);
	while( !(PINF & (1<<nSTATUS)) ); // wait ready
	indata = (ULONG)GET_FAR_ADDRESS(fpga); // prepare for data fetching
	depacker_dirty();


	//LED off
	PORTB |= (1<<LED);



	// start timer (led dimming and timeouts for ps/2)
	TCCR2 = 0b01110011;
	TIFR = (1<<TOV2);
	TIMSK = (1<<TOIE2);


        ps2_count = 11;

	EICRB = (1<<ISC41) + (0<<ISC40); // falling edge INT4
	EIFR = (1<<INTF4); // clear spurious ints there
	EIMSK |= (1<<INT4); // enable int4

	zx_init();

	sei(); // globally go interrupting


        for(;;)
        {
        	ps2_task();
        	zx_task();
        }












}

void InitHardware(void)
{

	cli(); // disable interrupts



	// configure pins

	PORTG = 0b11111111;
	DDRG  = 0b00000000;

	PORTF = 0b11110000; // ATX off (zero output), fpga config/etc inputs
	DDRF  = 0b00001000;

	PORTE = 0b11111111;
	DDRE  = 0b00000000; // inputs pulled up

	PORTD = 0b11111111;
	DDRD  = 0b00000000; // same

	PORTC = 0b11011111;
	DDRC  = 0b00000000; // PWRGOOD input, other pulled up

	PORTB = 0b11110001;
	DDRB  = 0b10000111; // LED off, spi outs inactive

	PORTA = 0b11111111;
	DDRA  = 0b00000000; // pulled up


	ACSR = 0x80; // DISABLE analog comparator
}


void put_buffer(UWORD size)
{ // writes specified length of buffer to the output
	UBYTE * ptr;
	ptr=dbuf;

	do
	{
		spi_send( *(ptr++) );

	} while(--size);
}

