#include "_global.h"
#include <avr/interrupt.h>
#include "_uart.h"

u8 uart_rxbuff[UART_RXBUFF_SIZE];
volatile u8 uart_rx_hd;
u8 uart_rx_tl;
u8 uart_txbuff[UART_TXBUFF_SIZE];
u8 uart_tx_hd;
volatile u8 uart_tx_tl;

//-----------------------------------------------------------------------------

void directuart_putchar(u8 ch)
{
 while ((UCSR1A&(1<<UDRE))==0) {}
 UDR1=ch;
}

//-----------------------------------------------------------------------------

void directuart_crlf(void)
{
 directuart_putchar(0x0d);
 directuart_putchar(0x0a);
}

//-----------------------------------------------------------------------------

void directuart_init(void)
{
 UBRR1H=0; UBRR1L=5;                            // 115200 baud (@11.0592 MHz)
 UCSR1A=0;                                      // Double Speed
 UCSR1C=(1<<UCSZ11)|(1<<UCSZ10)|(1<<USBS);      // 8databits, 2stopbits
 UCSR1B=(1<<TXEN1);                             // Разрешаем передачу
 flags1=(flags1&~ENABLE_UART)|ENABLE_DIRECTUART;
}

//-----------------------------------------------------------------------------

void uart_chk_cts(void)
{
 if ( (!(PINB&(1<<PB6))) && (uart_tx_tl!=uart_tx_hd) )
  UCSR1B|=(1<<UDRIE1);
}

//-----------------------------------------------------------------------------

void uart_putchar(u8 ch)
{
 do
 {
  if (flags1&RTSCTS_FLOWCTRL) uart_chk_cts();
 }while ( ((uart_tx_tl-uart_tx_hd-1)&U_TX_MASK)==0 );
 uart_txbuff[uart_tx_hd]=ch;
 uart_tx_hd=(uart_tx_hd+1)&U_TX_MASK;
 if ( !(flags1&RTSCTS_FLOWCTRL) || !(PINB&(1<<PB6)) )
  UCSR1B|=(1<<UDRIE1);
}

//-----------------------------------------------------------------------------

u8 uart_getchar(u8 *ch)
{
 if (uart_rx_tl!=uart_rx_hd)
 {
  *ch=uart_rxbuff[uart_rx_tl];
  uart_rx_tl=(uart_rx_tl+1)&U_RX_MASK;
  if (((uart_rx_tl-uart_rx_hd-1)&U_RX_MASK)>21)
   RTS_CLR();
  return 1;
 }
 else
  return 0;
}

//-----------------------------------------------------------------------------

void uart_crlf(void)
{
 uart_putchar(0x0d);
 uart_putchar(0x0a);
}

//-----------------------------------------------------------------------------

void uart_dump512(u8 *buff)
{
 u8 stored_flags1;
 stored_flags1=flags1;
 flags1&=~(ENABLE_SD_LOG|ENABLE_SCR|ENABLE_DIRECTUART);
 flags1|=ENABLE_UART;
 print_msg(PGMSTR("\r\n;     .0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .A .B .C .D .E .F"));
 u16 i;
 i=0;
 do
 {
  uart_crlf();
  print_hexbyte(i&0xff);
  print_hexbyte(i>>8);
  uart_putchar(0x20);
  uart_putchar(0x20);
  u8 j;
  for (j=0;j<16;j++) print_hexbyte_for_dump(buff[i+j]);
  uart_putchar(0x20);
  for (j=0;j<16;j++) put_char_for_dump(buff[i+j]);
  i+=16;
 }while (i<512);
 flags1=stored_flags1;
}

//-----------------------------------------------------------------------------

void uart_init(void)
{
 UBRR1H=0; UBRR1L=5;                            // 115200 baud (@11.0592 MHz)
 UCSR1A=0;                                      // Double Speed
 UCSR1C=(1<<UCSZ11)|(1<<UCSZ10)|(1<<USBS);      // 8databits, 2stopbits
 UCSR1B=(1<<RXCIE)|(1<<RXEN)|(1<<TXEN);         // Разрешаем приём, передачу и прерывания приёма
 flags1=(flags1&~ENABLE_DIRECTUART)|ENABLE_UART;

 uart_rx_hd=0;
 uart_rx_tl=0;
 uart_tx_hd=0;
 uart_tx_tl=0;

 RTS_CLR();
 DDRD|=(1<<PD5);
 DDRB&=~(1<<PB6);
}

//-----------------------------------------------------------------------------
