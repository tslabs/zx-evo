#include <avr/io.h>
#include <avr/interrupt.h>

#include "mytypes.h"
#include "rs232.h"
#include "pins.h"

//if want Log than comment next string
#undef LOGENABLE
//#define LOGENABLE

#define FOSC 11059200 // Clock Speed
#define BAUD115200 115200
#define UBRR115200 (((FOSC/16)/BAUD115200)-1)

//Registers for 16550 emulation:

//Divisor Latch LSB
static UBYTE rs232_DLL;
//Divisor Latch MSB
static UBYTE rs232_DLM;
//Interrupt Enable
static UBYTE rs232_IER;
//Interrupt Identification
static UBYTE rs232_IIR;
//FIFO Control
static UBYTE rs232_FCR;
//Line Control
static UBYTE rs232_LCR;
//Modem Control
static UBYTE rs232_MCR;
//Line Status
static UBYTE rs232_LSR;
//Modem Status
static UBYTE rs232_MSR;
//Scratch Pad
static UBYTE rs232_SCR;
//Fifo In
static UBYTE rs232_FI[16];
static UBYTE rs232_FI_start;
static UBYTE rs232_FI_end;
//Fifo Out
static UBYTE rs232_FO[16];
static UBYTE rs232_FO_start;
static UBYTE rs232_FO_end;

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

	//Set default values:
	rs232_IER = 0;
	rs232_FCR = 0;
	rs232_IIR = 0x01;
	rs232_LCR = 0;
	rs232_MCR = 0;
	rs232_LSR = 0x60;
	rs232_MSR = 0;
	rs232_SCR = 0xFF;
	rs232_FI_start = rs232_FI_end = 0;
	rs232_FO_start = rs232_FO_end = 0;
}

void rs232_transmit( UBYTE data )
{
	// Wait for empty transmit buffer
	while ( !( UCSR1A & (1<<UDRE)) );
	// Put data into buffer, sends the data
	UDR1 = data;
}

//#ifdef LOGENABLE
void to_log(char* ptr)
{
	while( (*ptr)!=0 )
	{
		rs232_transmit(*ptr);
		ptr++;
	}
}
//#endif


//after DLL or DLM changing
void rs232_set_baud(void)
{
	if ( rs232_DLM | rs232_DLL )
	{
		ULONG i = BAUD115200/ ((((UWORD)rs232_DLM)<<8) + rs232_DLL);
		UWORD rate = ((FOSC/16)/i)-1;
		// Set baud rate
		UBRR1H = (UBYTE)(rate>>8);
		UBRR1L = (UBYTE)rate;
	}
}

void rs232_zx_write(UBYTE index, UBYTE data)
{
#ifdef LOGENABLE
	char log_write[] = "A..D..W\r\n";
	log_write[1] = ((index >> 4) <= 9 )?'0'+(index >> 4):'A'+((index >> 4)-10);
	log_write[2] = ((index & 0x0F) <= 9 )?'0'+(index & 0x0F):'A'+(index & 0x0F)-10;
	log_write[4] = ((data >> 4) <= 9 )?'0'+(data >> 4):'A'+((data >> 4)-10);
	log_write[5] = ((data & 0x0F) <= 9 )?'0'+(data & 0x0F):'A'+((data & 0x0F)-10);
	to_log(log_write);
#endif
	switch( index )
	{
	case 0:
		if ( rs232_LCR & 0x80 )
		{
			rs232_DLL = data;
		}
		else
		{
			//place byte to fifo out
			//if ( rs232_FO_end )
			{
				rs232_FO[rs232_FO_end] = data;
				rs232_FO_end = (rs232_FO_end + 1) & 0x0F;
			}
		}
		break;

	case 1:
		if ( rs232_LCR & 0x80 )
		{
			//write to DLM
			rs232_DLM = data;
		}
		else
		{
			//bit 7-4 not used and set to '0'
			rs232_IER = data & 0x0F;
		}
		break;

	case 2:
		rs232_FCR = data;
		break;

	case 3:
		rs232_LCR = data;
		break;

	case 4:
		//bit 7-5 not used and set to '0'
		rs232_MCR = data & 0x1F;
		break;

	case 5:
		rs232_LSR = data;
		break;

	case 6:
		rs232_MSR = data;
		break;

	case 7:
		rs232_SCR = data;
		break;
	}
}

UBYTE rs232_zx_read(UBYTE index)
{
	UBYTE data = 0;
	switch( index )
	{
	case 0:
		if ( rs232_LCR & 0x80 )
		{
			data = rs232_DLL;
		}
		else
		{
			//get byte from fifo in
			if ( rs232_FI_start != rs232_FI_end )
			{
				data = rs232_FI[rs232_FI_start];
				rs232_FI_start = ( rs232_FI_start + 1 ) & 0x0F;
			}
		}
		break;

	case 1:
		if ( rs232_LCR & 0x80 )
		{
			data = rs232_DLM;
		}
		else
		{
			data = rs232_IIR;
		}
		break;

	case 2:
		data = rs232_FCR;
		break;

	case 3:
		data = rs232_LCR;
		break;

	case 4:
		data = rs232_MCR;
		break;

	case 5:
		data = rs232_LSR;
		break;

	case 6:
		data = rs232_MSR;
		break;

	case 7:
		data = rs232_SCR;
		break;
	}
#ifdef LOGENABLE
	static UBYTE last = 0;
	if ( (last!=6) || (last!=index) )
	{
		char log_read[] = "A..D..R\r\n";
		log_read[1] = ((index >> 4) <= 9 )?'0'+(index >> 4):'A'+((index >> 4)-10);
		log_read[2] = ((index & 0x0F) <= 9 )?'0'+(index & 0x0F):'A'+(index & 0x0F)-10;
		log_read[4] = ((data >> 4) <= 9 )?'0'+(data >> 4):'A'+((data >> 4)-10);
		log_read[5] = ((data & 0x0F) <= 9 )?'0'+(data & 0x0F):'A'+((data & 0x0F)-10);
		to_log(log_read);
	}
	last = index;
#endif

	return data;
}
