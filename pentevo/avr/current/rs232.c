#include <avr/io.h>
#include <avr/interrupt.h>

#include "mytypes.h"
#include "rs232.h"
#include "pins.h"

//if want Log than comment next string
#undef LOGENABLE

#define BAUD115200 115200
#define BAUD256000 256000
#define UBRR115200 (((F_CPU/16)/BAUD115200)-1)
#define UBRR256000 (((F_CPU/16)/BAUD256000)-1)

//Registers for 16550 emulation:

//Divisor Latch LSB
static UBYTE rs232_DLL;
//Divisor Latch MSB
static UBYTE rs232_DLM;
//Interrupt Enable
static UBYTE rs232_IER;
//Interrupt Identification
static UBYTE rs232_ISR;
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
	UCSR1B = _BV(RXEN)|_BV(TXEN);
	// Set frame format: 8data, 1stop bit
	UCSR1C = _BV(USBS)|_BV(UCSZ0)|_BV(UCSZ1);

	//Set default values:
	rs232_DLM = 0;
	rs232_DLL = 0x01;
	rs232_IER = 0;
	rs232_FCR = 0x01; //FIFO always enable
	rs232_ISR = 0x01;
	rs232_LCR = 0;
	rs232_MCR = 0;
	rs232_LSR = 0x60;
	rs232_MSR = 0xA0; //DSR=CD=1, RI=0
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
		if( (rs232_DLM&0x80)!=0 )
		{
			//AVR mode - direct load UBRR
			UBRR1H = 0x7F&rs232_DLM;
			UBRR1L = rs232_DLL;
		}
		else
		{
			//default mode - like 16550
			ULONG i = BAUD115200/ ((((UWORD)rs232_DLM)<<8) + rs232_DLL);
			UWORD rate = ((F_CPU/16)/i)-1;
			// Set baud rate
			UBRR1H = (UBYTE)(rate>>8);
			UBRR1L = (UBYTE)rate;
		}
	}
	else
	{
		// If( ( rs232_DLM==0 ) && ( rs232_DLL==0 ) )
		// set rate to 256000 baud
		UBRR1H = (UBYTE)(UBRR256000>>8);
		UBRR1L = (UBYTE)UBRR256000;
	}
}

//after LCR changing
void rs232_set_format(void)
{
	//set word length and stopbits
	UBYTE format = ((rs232_LCR&0x07)<<1);

	//set parity (only "No parity","Odd","Even" supported)
	switch( rs232_LCR&0x38 )
	{
		case 0x08:
			//odd parity
			format |= _BV(UPM0)|_BV(UPM1);
			break;
		case 0x18:
			//even parity
			format |= _BV(UPM1);
			break;
		//default - parity not used
	}

	UCSR1C = format;
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
			rs232_set_baud();
		}
		else
		{
			//place byte to fifo out
			if ( ( rs232_FO_end != rs232_FO_start ) ||
			     ( rs232_LSR&0x20 ) )
			{
				rs232_FO[rs232_FO_end] = data;
				rs232_FO_end = (rs232_FO_end + 1) & 0x0F;

				//clear fifo empty flag
				rs232_LSR &= ~(0x60);
			}
			else
			{
				//fifo overload
			}
		}
		break;

	case 1:
		if ( rs232_LCR & 0x80 )
		{
			//write to DLM
			rs232_DLM = data;
			rs232_set_baud();
		}
		else
		{
			//bit 7-4 not used and set to '0'
			rs232_IER = data & 0x0F;
		}
		break;

	case 2:
		if( data&1 )
		{
			//FIFO always enable
			if( data&(1<<1) )
			{
				//receive FIFO reset
				rs232_FI_start = rs232_FI_end = 0;
				//set empty FIFO flag
				rs232_LSR &= ~(0x01);
			}
			if( data&(1<<2) )
			{
				//tramsmit FIFO reset
				rs232_FO_start = rs232_FO_end = 0;
				//set fifo is empty flag
				rs232_LSR |= 0x60;
			}
			rs232_FCR = data&0xC9;
		}
		break;

	case 3:
		rs232_LCR = data;
		rs232_set_format();
		break;

	case 4:
		//bit 7-5 not used and set to '0'
		rs232_MCR = data & 0x1F;
		if ( data&(1<<1) )
		{
			//clear RTS
			RS232RTS_PORT &= ~(_BV(RS232RTS));
		}
		else
		{
			//set RTS
			RS232RTS_PORT |= _BV(RS232RTS);
		}
		break;

	case 5:
		//rs232_LSR = data;
		break;

	case 6:
		//rs232_MSR = data;
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
			if ( rs232_LSR&0x01 )
			{
				data = rs232_FI[rs232_FI_start];
				rs232_FI_start = ( rs232_FI_start + 1 ) & 0x0F;

				if( rs232_FI_start == rs232_FI_end )
				{
					//set empty FIFO flag
					rs232_LSR &= ~(0x01);
				}
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
			data = rs232_IER;
		}
		break;

	case 2:
		data = rs232_ISR;
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
		//DSR=CD=1
		data = rs232_MSR;
		//clear flags
		rs232_MSR &= 0xF0;
		break;

	case 7:
		data = rs232_SCR;
		break;
	}
#ifdef LOGENABLE
	static UBYTE last = 0;
	if ( last!=index )
	{
		char log_read[] = "A..D..R\r\n";
		log_read[1] = ((index >> 4) <= 9 )?'0'+(index >> 4):'A'+((index >> 4)-10);
		log_read[2] = ((index & 0x0F) <= 9 )?'0'+(index & 0x0F):'A'+(index & 0x0F)-10;
		log_read[4] = ((data >> 4) <= 9 )?'0'+(data >> 4):'A'+((data >> 4)-10);
		log_read[5] = ((data & 0x0F) <= 9 )?'0'+(data & 0x0F):'A'+((data & 0x0F)-10);
		to_log(log_read);
		last = index;
	}
#endif
	return data;
}

void rs232_task(void)
{
	//send data
	if( (rs232_LSR&0x20)==0 )
	{
		if ( UCSR1A&_BV(UDRE) )
		{
			UDR1 = rs232_FO[rs232_FO_start];
			rs232_FO_start = (rs232_FO_start+1)&0x0F;

			if( rs232_FO_start == rs232_FO_end )
			{
				//set fifo is empty flag
				rs232_LSR |= 0x60;
			}
		}
	}

	//receive data
	if( UCSR1A&_BV(RXC) )
	{
		BYTE b = UDR1;
		if( (rs232_FI_end == rs232_FI_start) &&
		    (rs232_LSR&0x01) )
		{
			//set overrun flag
			rs232_LSR|=0x02;
		}
		else
		{
			//receive data
		   	rs232_FI[rs232_FI_end] = b;
		   	rs232_FI_end = (rs232_FI_end+1)&0x0F;
		   	//set data received flag
		   	rs232_LSR |= 0x01;
		}
	}

	//statuses
	if( UCSR1A&_BV(FE) )
	{
		//frame error
		rs232_LSR |= 0x08;
	}
	else
	{
		rs232_LSR &= ~(0x08);
	}

	if( UCSR1A&_BV(UPE) )
	{
		//parity error
		rs232_LSR |= 0x04;
	}
	else
	{
		rs232_LSR &= ~(0x04);
	}

	if( RS232CTS_PIN&_BV(RS232CTS) )
	{
		//CTS clear
		if( (rs232_MSR&0x10)!=0 )
		{
#ifdef LOGENABLE
			to_log("CTS\r\n");
#endif
			//CTS changed - set flag
			rs232_MSR |= 0x01;
		}
		rs232_MSR &= ~(0x10);
	}
	else
	{
		//CTS set
		if( (rs232_MSR&0x10)==0 )
		{
#ifdef LOGENABLE
			to_log("CTS\r\n");
#endif
			//CTS changed - set flag
			rs232_MSR |= 0x01;
		}
		rs232_MSR |= 0x10;
	}
}
