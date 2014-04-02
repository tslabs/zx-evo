/*3                                             9
 ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
 ³      ÚÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿       ³
 ³      ³ pc/win32 ³  TESTCOM            ³       ³
 ³      ÃÄÄÄÄÄÄÄÄÄÄÙ                     ³       ³
 ³      ³ Bit rate 115200   No parity    ³       ³
 ³      ³ Data bits 8       Flow control ³       ³
 ³      ³ Stop bits 2        û RTS/CTS   ³       ³
 ³      ³                   DSR - Ignored³       ³
 ³      ³           Start BERT           ³       ³
 ³      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ       ³
 ³                      ³COM port                ³
 ³                      ³                        ³
 ³                RS-232³                        ³
 ³ÚÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿³
 ³³ ZX-Evo ³   Last sec        65535 sec        ³³16
 ³ÃÄÄÄÄÄÄÄÄÙ     10472            10472         ³³17
 ³³ RxBuff °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° RTS ³³18
 ³³ TxBuff °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° CTS ³³19
 ³ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ³20
 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
  3                                             9 */
//-----------------------------------------------------------------------------

#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"
#include <avr/interrupt.h>
#include "_uart.h"

typedef struct {
 u16 timeout;
 u16 seconds;
 u16 count_last;
 u32 count_total;
} TRS;

//-----------------------------------------------------------------------------

const WIND_DESC wind_t_rs_1 PROGMEM = { 2, 2,49,20,0xdf,0x01 };
const WIND_DESC wind_t_rs_2 PROGMEM = { 9, 3,34, 9,0xdf,0x00 };
const WIND_DESC wind_t_rs_3 PROGMEM = { 9, 3,12, 3,0xdf,0x00 };
const WIND_DESC wind_t_rs_4 PROGMEM = { 3,15,47, 6,0xdf,0x00 };
const WIND_DESC wind_t_rs_5 PROGMEM = { 3,15,10, 3,0xdf,0x00 };
#define p_wind_t_rs_1 ((const P_WIND_DESC)&wind_t_rs_1)
#define p_wind_t_rs_2 ((const P_WIND_DESC)&wind_t_rs_2)
#define p_wind_t_rs_3 ((const P_WIND_DESC)&wind_t_rs_3)
#define p_wind_t_rs_4 ((const P_WIND_DESC)&wind_t_rs_4)
#define p_wind_t_rs_5 ((const P_WIND_DESC)&wind_t_rs_5)

//-----------------------------------------------------------------------------

void t_rs_clrbuffs(void)
{
 cli();
 uart_rx_hd=0;
 uart_rx_tl=0;
 uart_tx_hd=0;
 uart_tx_tl=0;
 RTS_CLR();
 sei();
}

//-----------------------------------------------------------------------------

void t_rsbar0(u8 sz)
{
 if (sz)  scr_fill_char(0xdb,sz);               // 'Û'
 sz=32-sz;
 if (sz)  scr_fill_char(0xb0,sz);               // '°'
 scr_putchar(0x20);
}

//-----------------------------------------------------------------------------

void t_rs_status(TRS *trs)
{
 set_timeout_ms(&trs->timeout,1000);

 scr_set_cursor(32,16);
 trs->seconds++;
 if (trs->seconds==0)
 {
  trs->count_last=0;
  trs->count_total=0;
 }
 print_dec16(trs->seconds);

 scr_set_cursor(18,17);
 print_dec16(trs->count_last);
 trs->count_last=0;

 scr_set_cursor(32,17);
 if (trs->seconds)
  print_dec16((u16)(trs->count_total/trs->seconds));
 else
  print_dec16(0);
}

//-----------------------------------------------------------------------------

void Test_RS232(void)
{
 flags1&=0b11111100;
 flags1|=0b00000100;
 scr_window(p_wind_t_rs_1);
 scr_window(p_wind_t_rs_2);
 scr_window(p_wind_t_rs_3);
 scr_window(p_wind_t_rs_4);
 scr_window(p_wind_t_rs_5);
 scr_print_msg(msg_trs_1);
 TRS trs;
 u8 go2;

 do
 {
  trs.seconds=0xffff;
  flags1|=0b00010000;
  t_rs_clrbuffs();
  t_rs_status(&trs);
  do
  {
   go2=GO_READKEY;
   u16 key;
   if (inkey(&key))
   {
    if ((u8)(key>>8)==KEY_SPACE)
     go2=GO_RESTART;
    else if ( (!((u8)key&(1<<PS2K_BIT_EXTKEY))) && ((u8)(key>>8)==KEY_ESC) )
     go2=GO_EXIT;
   }

   if (go2==GO_READKEY)
   {
    if (check_timeout_ms(&trs.timeout))  t_rs_status(&trs);

    scr_set_cursor(12,18);
    {
     u8 ln;
     ln=(uart_rx_hd-uart_rx_tl)&U_RX_MASK;
#if (UART_RXBUFF_SIZE>128)
     ln>>=1;
#endif
#if (UART_RXBUFF_SIZE>64)
     ln>>=1;
#endif
#if (UART_RXBUFF_SIZE>32)
     ln++;
     ln>>=1;
#endif
     t_rsbar0(ln);
    }
    {
     u8 attr;
     if (PIND&(1<<PD5)) attr=0xae; else attr=0xc0;
     scr_fill_attr(attr,3);
    }
    scr_set_attr(0xdf);

    scr_set_cursor(12,19);
    {
     u8 ln;
     ln=(uart_tx_hd-uart_tx_tl)&U_TX_MASK;
#if (UART_RXBUFF_SIZE>128)
     ln>>=1;
#endif
#if (UART_RXBUFF_SIZE>64)
     ln>>=1;
#endif
#if (UART_RXBUFF_SIZE>32)
     ln++;
     ln>>=1;
#endif
     t_rsbar0(ln);
    }
    {
     u8 attr;
     if (PINB&(1<<PB6)) attr=0xae; else attr=0xc0;
     scr_fill_attr(attr,3);
    }
    scr_set_attr(0xdf);

    while (1)
    {
     uart_chk_cts();
     if (((uart_tx_tl-uart_tx_hd-1)&U_TX_MASK)==0) break;
     u8 data;
     if (!(uart_getchar(&data))) break;
     uart_putchar(data);
     trs.count_total++;
     trs.count_last++;
    }
   }

  }while (go2==GO_READKEY);

 }while (go2!=GO_EXIT);

 t_rs_clrbuffs();
 flags1&=0b11101111;
}

//-----------------------------------------------------------------------------
