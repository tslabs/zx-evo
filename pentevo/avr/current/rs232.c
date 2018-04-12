#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "mytypes.h"
#include "rs232.h"
#include "zx.h"
#include "pins.h"
//#include "fifo.h"

// if want Log than comment next string
#undef LOGENABLE

#define BAUD115200 115200
#define BAUD230400 230400
#define UBRR115200 (((F_CPU/16)/BAUD115200)-1)
#define UBRR230400 (((F_CPU/16)/BAUD230400)-1)

// Registers for 16550 emulation
u8 rs232_DLL; //Divisor Latch LSB
u8 rs232_DLM; //Divisor Latch MSB
u8 rs232_IER; //Interrupt Enable
u8 rs232_ISR; //Interrupt Identification
//u8 rs232_FCR; //FIFO Control
u8 rs232_LCR; //Line Control
u8 rs232_MCR; //Modem Control
volatile u8 rs232_LSR; //Line Status
u8 rs232_MSR; //Modem Status
u8 rs232_SCR; //Scratch Pad

u8 select_zf;
u8 zf_int_mask;
u8 zf_int_src;
u8 zf_api;
u8 zf_err;

volatile u8 rs_tmo_cnt;
volatile u8 zf_tmo_cnt;

volatile u16 rs_rx_hd;
u16 rs_rx_tl;
u8 rs_tx_hd;
volatile u8 rs_tx_tl;
u8 rs_ibtr;
u8 rs_itor;

volatile u16 zf_rx_hd;
u16 zf_rx_tl;
u8 zf_tx_hd;
volatile u8 zf_tx_tl;
u8 zf_ibtr;
u8 zf_itor;

void rs232_init(void)
{
  // Set default values
  rs232_DLM = 0;
  rs232_DLL = 0x01;
  rs232_IER = 0;
  //rs232_FCR = 0x01; //FIFO always enable
  rs232_ISR = 0x01;
  rs232_LCR = 0;
  rs232_MCR = 0;
  rs232_LSR = 0x60;
  rs232_MSR = 0xA0; //DSR=CD=1, RI=0
  rs232_SCR = 0xFF;

  rs_tmo_cnt = 0;
  zf_tmo_cnt = 0;

  rs_rx_hd = 0;
  rs_rx_tl = 0;
  rs_tx_hd = 0;
  rs_tx_tl = 0;
  rs_ibtr = 0x80;
  rs_itor = 1;

  select_zf  =  0;
  zf_int_mask = 0;
  zf_int_src = 0;
  zf_api = 0;
  zf_err = 0;

  zf_rx_hd = 0;
  zf_rx_tl = 0;
  zf_tx_hd = 0;
  zf_tx_tl = 0;
  zf_ibtr = 0x80;
  zf_itor = 1;

  // Set baud rate
  UBRR1H = (u8)(UBRR115200>>8);
  UBRR1L = (u8)UBRR115200;
  UCSR1A = 0;
  UCSR1C = _BV(USBS1)|_BV(UCSZ10)|_BV(UCSZ11); // Set frame format: 8data (2stop bit - for TX only)
  UCSR1B = _BV(RXCIE1)|_BV(RXEN1)|_BV(TXEN1);  // Enable transmitter, receiver and receiver's interrupt

  UBRR0H = (u8)(UBRR115200>>8);
  UBRR0L = (u8)UBRR115200;
  UCSR0A = 0;
  UCSR0C = _BV(USBS0)|_BV(UCSZ00)|_BV(UCSZ01); // Set frame format: 8data (2stop bit - for TX only)
  UCSR0B = _BV(RXCIE0)|_BV(RXEN0)|_BV(TXEN0);  // Enable transmitter, receiver and receiver's interrupt
}

//#ifdef LOGENABLE
//void to_log(char* ptr)
//{
//  while((*ptr)!=0)
//  {
//    rs232_transmit(*ptr);
//    ptr++;
//  }
//}
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
    // set rate to 230400 baud
    UBRR1H = (u8)(UBRR230400>>8);
    UBRR1L = (u8)UBRR230400;
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
      format |= _BV(UPM10)|_BV(UPM11);
      break;
    case 0x18:
      //even parity
      format |= _BV(UPM11);
      break;
    //default - parity not used
  }

  UCSR1C = format;
}

//------------------------------------------------------------------------------

void rs232_zx_write(u8 index, u8 data)
{

  // ZiFi
  // ZiFi data write
  if (index <= ZF_DR_REG_LIM)
  {
    if (select_zf)
    {
      if (zf_api == 1)
      {
        if ((zf_tx_tl-1-zf_tx_hd)&0xff)
        {
          zf_txbuff[zf_tx_hd]=data;
          zf_tx_hd++;
          UCSR0B|=_BV(UDRIE0);
        }
      }
    }
    else
    { // enhanced RS-232
      if (zf_api)
      {
        if ((rs_tx_tl-1-rs_tx_hd)&0xff)
        {
          rs_txbuff[rs_tx_hd]=data;
          rs_tx_hd++;
          UCSR1B|=_BV(UDRIE1);
        }
        if (((rs_tx_tl-1-rs_tx_hd)&0xff)==0)
          rs232_LSR &= ~(0x60);
      }
    }
  }

  // command write
  else switch (index)
  {
    case ZF_CR_REG:
      // set API mode
      if ((data & ZF_SETAPI_MASK) == ZF_SETAPI)
      {
        zf_api = data & ~ZF_SETAPI_MASK;
        if (zf_api > ZF_VER)
          zf_api = 0;
        zf_err = ZF_OK_RES;
      }

      // get API version
      else if (zf_api)
      {
        if ((data & ZF_GETVER_MASK) == ZF_GETVER)
          zf_err = ZF_VER;
        // clear FIFOs
        else if ((data & ZF_CLRFIFO_MASK) == ZF_CLRFIFO)
        {
          ATOMIC_BLOCK(ATOMIC_FORCEON)
          {
            if (data & ZF_CLRFIFO_IN)
            {
              zf_rx_hd=0;
              zf_rx_tl=0;
            }

            if (data & ZF_CLRFIFO_OUT)
            {
              zf_tx_hd=0;
              zf_tx_tl=0;
            }
          }
        }
        else if ((data & RS_CLRFIFO_MASK) == RS_CLRFIFO)
        {
          if (data & RS_CLRFIFO_IN)
          {
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
              rs_rx_hd=0;
              rs_rx_tl=0;
            }
          }

          if (data & RS_CLRFIFO_OUT)
          {
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
              rs_tx_hd=0;
              rs_tx_tl=0;
            }
          }
        }
      }
    break;

    // set interrupt mask
    case ZF_IMR_REG:
      zf_int_mask |= data;
    break;

    // set ZiFi input buffer threshold for interrupt
    case ZF_IBTR_REG:
      zf_ibtr = data;
    break;

    // set ZiFi input buffer timeout for interrupt
    case ZF_ITOR_REG:
      zf_itor = data;
    break;

    // set RS-232 input buffer threshold for interrupt
    case RS_IBTR_REG:
      rs_ibtr = data;
    break;

    // set RS-232 input buffer timeout for interrupt
    case RS_ITOR_REG:
      rs_itor = data;
    break;

    // Kondratyev
    default:
      switch (index)
      {
        case UART_DAT_DLL_REG:
          if (rs232_LCR & 0x80)
          {
            rs232_DLL = data;
            rs232_set_baud();
          }

          else
          {
            if ((rs_tx_tl-1-rs_tx_hd)&0xff)
            {
              rs_txbuff[rs_tx_hd]=data;
              rs_tx_hd++;
              UCSR1B|=_BV(UDRIE1);
            }
            if (((rs_tx_tl-1-rs_tx_hd)&0xff)==0)
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
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
              //receive FIFO reset
              if (data & (1<<1))
              {
                rs_rx_hd=0;
                rs_rx_tl=0;
                rs232_LSR &= ~(0x03);   //set empty FIFO flag and clear overrun flag
              }

              //tramsmit FIFO reset
              if (data & (1<<2))
              {
                rs_tx_hd=0;
                rs_tx_tl=0;
                rs232_LSR |= 0x60;      //set fifo is empty flag
              }
            }
            //rs232_FCR = data & 0xC9;
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
    break;
  }
}

//------------------------------------------------------------------------------

u16 read_zf_ifr()
{
  u16 tmp;

  ATOMIC_BLOCK(ATOMIC_FORCEON)
  {
    tmp = zf_rx_hd;
  }

  tmp = (tmp - zf_rx_tl) & 0x01ff;
  return tmp;
}

u16 read_rs_ifr()
{
  u16 tmp;

  ATOMIC_BLOCK(ATOMIC_FORCEON)
  {
    tmp = rs_rx_hd;
  }

  tmp = (tmp - rs_rx_tl) & 0x01ff;
  return tmp;
}

u8 rs232_zx_read(u8 index)
{
  u8 data;

  // ZiFi & enhanced RS-232
  if (index <= ZF_REG_LIM)
  {
    data = 0xFF;

    // Data Register
    if (index <= ZF_DR_REG_LIM)
    {
      if (select_zf)
      { // ZiFi data read
        if (zf_api == 1)
        {
          u16 tmp;
          ATOMIC_BLOCK(ATOMIC_FORCEON) { tmp=zf_rx_hd; }
          if (zf_rx_tl!=tmp)
          {
            data=zf_rxbuff[zf_rx_tl];
            zf_rx_tl=(zf_rx_tl+1)&0x01ff;
          }
        }
      }
      else
      { // enhanced RS-232 data read
        if (zf_api)
        {
          u16 tmp;
          ATOMIC_BLOCK(ATOMIC_FORCEON)
          {
            tmp=rs_rx_hd;
          }

          if (rs_rx_tl!=tmp)
          {
            data=rs_rxbuff[rs_rx_tl];
            rs_rx_tl=(rs_rx_tl+1)&0x01ff;
          }
        }
      }
    }

    // other registers
    else if (zf_api)
      switch (index)
      {
        // ZF FIFO in used read
        case ZF_IFR_REG:
        {
          select_zf = 1;
          u16 t = read_zf_ifr();
          data = min(t, ZF_DR_REG_LIM);
        }
        break;

        // ZF FIFO out free read
        case ZF_OFR_REG:
        {
          u8 tmp = zf_tx_tl - 1 - zf_tx_hd;

          if (tmp > ZF_DR_REG_LIM)
            data = ZF_DR_REG_LIM;
          else
            data = tmp;

          select_zf = 1;
        }
        break;

        // ZF ISR read
        case ZF_ISR_REG:
          data = zf_int_src;
          zf_int_src = 0;   // clear flags
        break;

        // ZF IBTR read
        case ZF_IBTR_REG:
          data = zf_ibtr;
        break;

        // ZF ITOR read
        case ZF_ITOR_REG:
          data = zf_itor;
        break;

        // error code read
        case ZF_ER_REG:
          data = zf_err;
        break;

        // RS FIFO in used read
        case RS_IFR_REG:
        {
          select_zf = 0;
          u16 t = read_rs_ifr();
          data = min(t, ZF_DR_REG_LIM);
        }
        break;

        // RS FIFO out free read
        case RS_OFR_REG:
        {
          u8 tmp = rs_tx_tl - 1 - rs_tx_hd;

          if (tmp > ZF_DR_REG_LIM)
            data = ZF_DR_REG_LIM;
          else
            data = tmp;

          select_zf = 0;
        }
        break;

        // RS IBTR read
        case RS_IBTR_REG:
          data = rs_ibtr;
        break;

        // RS ITOR read
        case RS_ITOR_REG:
          data = rs_itor;
        break;
    }
  }

  // Kondratyev
  else
  {
    data = 0;

    switch (index)
    {
      case UART_DAT_DLL_REG:
        if (rs232_LCR & 0x80)
          data = rs232_DLL;
        else
        {
          u16 tmp;
          ATOMIC_BLOCK(ATOMIC_FORCEON) { tmp=rs_rx_hd; }
          if (rs_rx_tl!=tmp)
          {
            data=rs_rxbuff[rs_rx_tl];
            rs_rx_tl = (rs_rx_tl + 1) & 0x01ff;
          }

          //set empty FIFO flag
          if (rs_rx_tl == tmp)
            rs232_LSR &= ~0x01;
        }
      break;

      case UART_IER_DLM_REG:
        if (rs232_LCR & 0x80)
          data = rs232_DLM;
        else
          data = rs232_IER;
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
  }

  return data;
}

//------------------------------------------------------------------------------

void rs232_task(void)
{
  // ZiFi & enhanced RS-232

  // ZiFi Input Buffer over threshold interrupt
  if (zf_int_mask & ZF_IMR_IBT)
  {
    if (read_zf_ifr() >= zf_ibtr)
    {
      zf_int_mask &= ~ZF_IMR_IBT;
      zf_int_src |= ZF_IMR_IBT;
    }
  }

  // RS-232 Input Buffer over threshold interrupt
  if (zf_int_mask & RS_IMR_IBT)
  {
    if (read_rs_ifr() >= rs_ibtr)
    {
      zf_int_mask &= ~RS_IMR_IBT;
      zf_int_src |= RS_IMR_IBT;
    }
  }

  // ZiFi Input Buffer timeout interrupt
  if (zf_int_mask & ZF_IMR_ITO)
  {
    if ((zf_tmo_cnt >= zf_itor) && read_zf_ifr())
    {
      zf_int_mask &= ~ZF_IMR_ITO;
      zf_int_src |= ZF_IMR_ITO;
    }
  }

  // RS-232 Input Buffer timeout interrupt
  if (zf_int_mask & RS_IMR_ITO)
  {
    if ((rs_tmo_cnt >= rs_itor) && read_rs_ifr())
    {
      zf_int_mask &= ~RS_IMR_ITO;
      zf_int_src |= RS_IMR_ITO;
    }
  }

  // generate wait port interrupt
  if (zf_int_src)
  {
    modes_register |= MODE_WTP_INT;
    zx_set_config();
    modes_register &= ~MODE_WTP_INT;
  }

  // Kondratyev
  ATOMIC_BLOCK(ATOMIC_FORCEON)
  {
    //statuses
    u8 u1stat = UCSR1A;
    u8 tmplsr = rs232_LSR;

    //frame error
    if (u1stat & _BV(FE1))
      tmplsr |= 0x08;
    else
      tmplsr &= ~(0x08);

    //parity error
    if (u1stat & _BV(UPE1))
      tmplsr |= 0x04;
    else
      tmplsr &= ~(0x04);

    if (u1stat & _BV(TXC1))
      tmplsr |= 0x40;
    else
      tmplsr &= ~(0x40);
    rs232_LSR = tmplsr;
  }

  //CTS clear
  if (RS232CTS_PIN & _BV(RS232CTS))
  {
    //CTS changed - set flag
    if ((rs232_MSR & 0x10)!=0)
      rs232_MSR |= 0x01;

    rs232_MSR &= ~(0x10);
  }

  //CTS set
  else
  {
    //CTS changed - set flag
    if ((rs232_MSR & 0x10)==0)
      rs232_MSR |= 0x01;

    rs232_MSR |= 0x10;
  }
}
