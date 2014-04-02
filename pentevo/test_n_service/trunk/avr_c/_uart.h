#ifndef _UART_H
#define _UART_H 1

/* размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт (32,64,128 или 256) */
#define UART_RXBUFF_SIZE 128
#define U_RX_MASK (UART_RXBUFF_SIZE-1)
/* размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт (32,64,128 или 256) */
#define UART_TXBUFF_SIZE 128
#define U_TX_MASK (UART_TXBUFF_SIZE-1)


#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
/* u8 uart_rxbuff[UART_RXBUFF_SIZE]; */
.extern uart_rxbuff
/* u8 uart_rx_hd, uart_rx_tl; */
.extern uart_rx_hd
.extern uart_rx_tl
/* u8 uart_txbuff[UART_TXBUFF_SIZE]; */
.extern uart_txbuff
/* u8 uart_tx_hd, uart_tx_tl; */
.extern uart_tx_hd
.extern uart_tx_tl
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

#include "_types.h"

extern volatile u8 uart_rx_hd;
extern u8 uart_rx_tl;
extern u8 uart_tx_hd;
extern volatile u8 uart_tx_tl;

#define RTS_CLR() PORTD&=~(1<<PD5)

void directuart_putchar(u8 ch);
void directuart_crlf(void);
void directuart_init(void);

void uart_chk_cts(void);
void uart_putchar(u8 ch);
u8 uart_getchar(u8 *ch);
void uart_crlf(void);
void uart_dump512(u8 *buff);
void uart_init(void);

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef _UART_H
