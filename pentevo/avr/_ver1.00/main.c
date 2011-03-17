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
#include "rs232.h"
#include "rtc.h"
#include "atx.h"

//fpga compressed data
extern const char fpga[] PROGMEM; // linker symbol

ULONG indata;

//buffer for depacking FPGA configuration
//you can USED for other purposed after setup FPGA
UBYTE dbuf[DBSIZE];

void put_buffer(UWORD size)
{
	// writes specified length of buffer to the output
	UBYTE * ptr = dbuf;

	do
	{
		spi_send( *(ptr++) );

	} while(--size);
}

void hardware_init(void)
{
	//Initialized AVR pins

	cli(); // disable interrupts

	// configure pins

	PORTG = 0b11111111;
	DDRG  = 0b00000000;

//	PORTF = 0b11110000; // ATX off (zero output), fpga config/etc inputs
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

int main()
{
start:

	hardware_init();

#ifdef LOGENABLE
	rs232_init();
#endif

	wait_for_atx_power();

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
	TCCR2 = 0b01110011; // FOC2=0, {WGM21,WGM20}=01, {COM21,COM20}=11, {CS22,CS21,CS20}=011
	                    // clk/64 clocking,
	                    // 1/512 overflow rate, total 11.059/32768 = 337.5 Hz interrupt rate
	TIFR = (1<<TOV2);
	TIMSK = (1<<TOIE2);


	//init some device
    ps2keyboard_count = 11;
	ps2mouse_count = 12;
	ps2mouse_initstep = 0;
	ps2mouse_resp_count = 0;

	//enable mouse
	zx_mouse_reset(1);

	//set external interrupt
	//INT4 - PS2 Keyboard  (falling edge)
	//INT5 - PS2 Mouse     (falling edge)
	//INT6 - SPI  (falling edge)
	//INT7 - RTC  (falling edge)
	EICRB = (1<<ISC41)+(0<<ISC40) + (1<<ISC51)+(0<<ISC50) + (1<<ISC61)+(0<<ISC60) + (1<<ISC71)+(0<<ISC70); // set condition for interrupt
	EIFR = (1<<INTF4)|(1<<INTF5)|(1<<INTF6)|(1<<INTF7); // clear spurious ints there
	EIMSK |= (1<<INT4)|(1<<INT5)|(1<<INT6)|(1<<INT7); // enable

	rtc_init();
	zx_init();

#ifdef LOGENABLE
	to_log("zx_init OK\r\n");
#endif


	sei(); // globally go interrupting

	//main loop
	do
    {
        ps2keyboard_task();
		ps2mouse_task();
        zx_task(ZX_TASK_WORK);
		zx_mouse_task();

		if ( ps2_flags&SPI_INT_FLAG )
		{
			//get status byte
			UBYTE status;
			nSPICS_PORT &= ~(1<<nSPICS);
			nSPICS_PORT |= (1<<nSPICS);
			status = spi_send(0);
			zx_wait_task( status );
		}
    }
	while( atx_power_task() );

	goto start;
}
