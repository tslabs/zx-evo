#include <avr/io.h>
#include <avr/interrupt.h>

#include "mytypes.h"
#include "rs232.h"
#include "zx.h"
#include "pins.h"
#include "fifo.h"

// if want Log than comment next string
#undef LOGENABLE

#define BAUD115200 115200
#define BAUD256000 256000
#define UBRR115200 (((F_CPU/16)/BAUD115200)-1)
#define UBRR256000 (((F_CPU/16)/BAUD256000)-1)

// Registers for 16550 emulation
static u8 rs232_DLL; //Divisor Latch LSB
static u8 rs232_DLM; //Divisor Latch MSB
static u8 rs232_IER; //Interrupt Enable
static u8 rs232_ISR; //Interrupt Identification
static u8 rs232_FCR; //FIFO Control
static u8 rs232_LCR; //Line Control
static u8 rs232_MCR; //Modem Control
static u8 rs232_LSR; //Line Status
static u8 rs232_MSR; //Modem Status
static u8 rs232_SCR; //Scratch Pad
static u8 rs232_FI[16];
static u8 rs232_FO[16];
FIFO rs232_in(rs232_FI, sizeof(rs232_FI));
FIFO rs232_out(rs232_FO, sizeof(rs232_FO));

static u8 zf_api;
static u8 zf_err;
static u8 zf_FI[255];
static u8 zf_FO[255];
FIFO zf_in(zf_FI, sizeof(zf_FI));
FIFO zf_out(zf_FO, sizeof(zf_FO));

void wifi_init(void)
{
	// Set baud rate
	UBRR0H = (u8)(UBRR115200 >> 8);
	UBRR0L = (u8)UBRR115200;
	UCSR0A = 0;   // Clear reg
	UCSR0B = _BV(RXEN)|_BV(TXEN);   // Enable receiver and transmitter
	UCSR0C = _BV(USBS)|_BV(UCSZ0)|_BV(UCSZ1);   // Set frame format: 8data, 1stop bit

	// Set default values
  zf_api = 0;
  zf_err = 0;
  zf_in.clear();
  zf_out.clear();
}

void kondr_init(void)
{
	// Set baud rate
	UBRR1H = (u8)(UBRR115200>>8);
	UBRR1L = (u8)UBRR115200;
	UCSR1A = 0;   // Clear reg
	UCSR1B = _BV(RXEN)|_BV(TXEN);   // Enable receiver and transmitter
	UCSR1C = _BV(USBS)|_BV(UCSZ0)|_BV(UCSZ1);   // Set frame format: 8data, 1stop bit

	// Set default values
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
  rs232_in.clear();
  rs232_out.clear();
}

void rs232_init(void)
{
  wifi_init();
  kondr_init();
}

void rs232_transmit(u8 data)
{
	// Wait for empty transmit buffer
	while (!(UCSR1A & (1<<UDRE)));
	// Put data into buffer, sends the data
	UDR1 = data;
}

//#ifdef LOGENABLE
void to_log(char* ptr)
{
	while((*ptr)!=0)
	{
		rs232_transmit(*ptr);
		ptr++;
	}
}
//#endif


//after DLL or DLM changing
void rs232_set_baud(void)
{
	if (rs232_DLM | rs232_DLL)
	{
		if ((rs232_DLM & 0x80)!=0)
		{
			//AVR mode - direct load UBRR
			UBRR1H = 0x7F & rs232_DLM;
			UBRR1L = rs232_DLL;
		}
		else
		{
			//default mode - like 16550
			u32 i = BAUD115200/ ((((u16)rs232_DLM)<<8) + rs232_DLL);
			u16 rate = ((F_CPU/16)/i)-1;
			// Set baud rate
			UBRR1H = (u8)(rate>>8);
			UBRR1L = (u8)rate;
		}
	}
	else
	{
		// if ((rs232_DLM==0) && (rs232_DLL==0))
		// set rate to 256000 baud
		UBRR1H = (u8)(UBRR256000>>8);
		UBRR1L = (u8)UBRR256000;
	}
}

//after LCR changing
void rs232_set_format(void)
{
	//set word length and stopbits
	u8 format = ((rs232_LCR & 0x07)<<1);

	//set parity (only "No parity","Odd","Even" supported)
	switch (rs232_LCR & 0x38)
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

void rs232_zx_write(u8 index, u8 data)
{
#ifdef LOGENABLE
	char log_write[] = "A..D..W\r\n";
	log_write[1] = ((index >> 4) <= 9)?'0'+(index >> 4):'A'+((index >> 4)-10);
	log_write[2] = ((index & 0x0F) <= 9)?'0'+(index & 0x0F):'A'+(index & 0x0F)-10;
	log_write[4] = ((data >> 4) <= 9)?'0'+(data >> 4):'A'+((data >> 4)-10);
	log_write[5] = ((data & 0x0F) <= 9)?'0'+(data & 0x0F):'A'+((data & 0x0F)-10);
	to_log(log_write);
#endif

  // ZiFi
  if (index == ZF_CR_ER_REG)
  // command write
  {
    // set API mode
    if ((data & ZF_SETAPI_MASK) == ZF_SETAPI)
    {
      zf_api = data & ~ZF_SETAPI_MASK;
      if (zf_api > ZF_VER)
        zf_api = 0;
      zf_err = ZF_OK_RES;
    }

    else if (zf_api)
    {
      // clear FIFOs
      if ((data & ZF_CLRFIFO_MASK) == ZF_CLRFIFO)
      {
        if (data & ZF_CLRFIFO_IN)
          zf_in.clear();

        if (data & ZF_CLRFIFO_OUT)
          zf_out.clear();
      }

      // get API version
      else if ((data & ZF_GETVER_MASK) == ZF_GETVER)
        zf_err = ZF_VER;
    }
  }

  else if (index <= ZF_DR_REG_LIM)
  {
    // data write
    if (zf_api)
      zf_out.put_byte(data);
  }

  // Kondratyev
  else switch (index)
	{
    case UART_DAT_DLL_REG:
      if (rs232_LCR & 0x80)
      {
        rs232_DLL = data;
        rs232_set_baud();
      }

      else
      {
        if (rs232_out.free)
          //place byte to fifo out
          rs232_out.put_byte(data);

        rs232_LSR &= ~(0x60);
      }
		break;

    case UART_IER_DLM_REG:
      if (rs232_LCR & 0x80)
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

    case UART_FCR_ISR_REG:
      if (data & 1)
      {
        //FIFO always enable
        if (data & (1<<1))
        {
          //receive FIFO reset
          rs232_in.clear();
          //set empty FIFO flag and clear overrun flag
          rs232_LSR &= ~(0x03);
        }
        if (data & (1<<2))
        {
          //tramsmit FIFO reset
          rs232_out.clear();
          //set fifo is empty flag
          rs232_LSR |= 0x60;
        }
        rs232_FCR = data & 0xC9;
      }
		break;

    case UART_LCR_REG:
      rs232_LCR = data;
      rs232_set_format();
		break;

    case UART_MCR_REG:
      //bit 7-5 not used and set to '0'
      rs232_MCR = data & 0x1F;
      if (data & (1<<1))
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

    case UART_LSR_REG:
      //rs232_LSR = data;
		break;

    case UART_MSR_REG:
      //rs232_MSR = data;
		break;

    case UART_SPR_REG:
      rs232_SCR = data;
		break;
	}
}

u8 rs232_zx_read(u8 index)
{
	u8 data = 0;

  // ZiFi
  if (index <= ZF_CR_ER_REG)
  {
    data = 0xFF;

    if (zf_api)
    {
      // data read
      if (index <= ZF_DR_REG_LIM)
        data = zf_in.get_byte();

      else switch (index)
      {
        // FIFO in used read
        case ZF_IFR_REG:
          data = zf_in.used;
        break;

        // FIFO out free read
        case ZF_OFR_REG:
          data = zf_out.free;
        break;

        // error code read
        case ZF_CR_ER_REG:
          data = zf_err;
        break;
      }
    }
  }

  // Kondratyev
  else switch (index)
	{
	case UART_DAT_DLL_REG:
		if (rs232_LCR & 0x80)
		{
			data = rs232_DLL;
		}
		else
		{
			if (rs232_in.used)
        //get byte from fifo in
        data = rs232_in.get_byte();

      //set empty FIFO flag
      if (rs232_in.used)
        rs232_LSR |= 0x01;
      else
        rs232_LSR &= ~0x01;
		}
		break;

	case UART_IER_DLM_REG:
		if (rs232_LCR & 0x80)
		{
			data = rs232_DLM;
		}
		else
		{
			data = rs232_IER;
		}
		break;

	case UART_FCR_ISR_REG:
		data = rs232_ISR;
		break;

	case UART_LCR_REG:
		data = rs232_LCR;
		break;

	case UART_MCR_REG:
		data = rs232_MCR;
		break;

	case UART_LSR_REG:
		data = rs232_LSR;
		break;

	case UART_MSR_REG:
		//DSR=CD=1
		data = rs232_MSR;
		//clear flags
		rs232_MSR &= 0xF0;
		break;

	case UART_SPR_REG:
		data = rs232_SCR;
		break;
	}
#ifdef LOGENABLE
	static u8 last = 0;
	if (last!=index)
	{
		char log_read[] = "A..D..R\r\n";
		log_read[1] = ((index >> 4) <= 9)?'0'+(index >> 4):'A'+((index >> 4)-10);
		log_read[2] = ((index & 0x0F) <= 9)?'0'+(index & 0x0F):'A'+(index & 0x0F)-10;
		log_read[4] = ((data >> 4) <= 9)?'0'+(data >> 4):'A'+((data >> 4)-10);
		log_read[5] = ((data & 0x0F) <= 9)?'0'+(data & 0x0F):'A'+((data & 0x0F)-10);
		to_log(log_read);
		last = index;
	}
#endif
	return data;
}

void rs232_task(void)
{
  // ZiFi
	//send data
	if (zf_out.used)
	{
		if (UCSR0A & _BV(UDRE))
			UDR0 = zf_out.get_byte();
	}

	//receive data
	if (UCSR0A & _BV(RXC))
	{
    if (zf_in.free)
      zf_in.put_byte(UDR0);
	}
  
  // Kondratyev
	//send data
	if (rs232_out.used)
	{
		if (UCSR1A & _BV(UDRE))
		{
			UDR1 = rs232_out.get_byte();

      //set fifo is empty flag
			if (!rs232_out.used)
				rs232_LSR |= 0x60;
		}
	}

	//receive data
	if (UCSR1A & _BV(RXC))
	{
    if (rs232_in.free)
      rs232_in.put_byte(UDR1);
    else
      //set overrun flag
			rs232_LSR |= 0x02;

		//set data received flag
		rs232_LSR |= 0x01;
	}

	//statuses
	if (UCSR1A & _BV(FE))
	{
		//frame error
		rs232_LSR |= 0x08;
	}
	else
	{
		rs232_LSR &= ~(0x08);
	}

	if (UCSR1A & _BV(UPE))
	{
		//parity error
		rs232_LSR |= 0x04;
	}
	else
	{
		rs232_LSR &= ~(0x04);
	}

	if (RS232CTS_PIN & _BV(RS232CTS))
	{
		//CTS clear
		if ((rs232_MSR & 0x10)!=0)
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
		if ((rs232_MSR & 0x10)==0)
		{
#ifdef LOGENABLE
			to_log("CTS\r\n");
#endif
			//CTS changed - set flag
			rs232_MSR |= 0x01;
		}
		rs232_MSR |= 0x10;
	}

  // status for TS-Conf
  char status = 0;

  // Rx data available
  if (rs232_in.used)
    status |= SPI_STATUS_REG_RX;

  // Tx space available
  if (rs232_out.free)
    status |= SPI_STATUS_REG_TX;

  zx_spi_send(SPI_STATUS_REG, status, ZXW_MASK);
}
