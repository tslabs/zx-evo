#include "_global.h"
#include "_output.h"
#include "_uart.h"
#include <util/delay_basic.h>

//-----------------------------------------------------------------------------

void PinTest(void)
{
 u8 err, tmp;

 err=0;                                         // проверка ножки UART TX
 PORTD|=(1<<PD3);
 _delay_loop_1(3);
 if (!(PIND&(1<<PD3))) err|=0x01;
 PORTD&=~(1<<PD3);
 DDRD|=(1<<PD3);
 _delay_loop_1(3);
 if (PIND&(1<<PD3)) err|=0x02;
 DDRD&=~(1<<PD3);

 if (err)                                       // проблемы с ножкой UART TX
 {                                              // хаотически мигаем св.диодом
  DDRB|=(1<<PB7);
  while (1)
  {
   PORTB&=~(1<<PB7);                            // led off
   if (random8()&0x01) PORTB|=(1<<PB7);        // led on
   _delay_loop_2(0x6c00);
  }
 }
                                                // проблем с ножки UART TX нет -
 directuart_init();                             // - задействуем UART без буфера FIFO
 directuart_crlf();
 directuart_crlf();
 directuart_crlf();
 print_msg(msg_title1);
 print_short_vers();
 print_mlmsg(mlmsg_pintest);
                                                // проверка ножек. 1 этап.
 PORTA=0b01010101;
 DDRA =0b10101010;
 PORTB=0b10000010;
 DDRB =0b00000101;
 PORTC=0b00010101;
 DDRC =0b00001010;
 PORTD|= (1<<PD5);
 DDRD &=~(1<<PD5);
 PORTE|= (1<<PE0);
 DDRE &=~(1<<PE0);
 PORTE&=~(1<<PE1);
 DDRE |= (1<<PE1);
 PORTG=0b00010101;
 DDRG =0b00001010;
 _delay_loop_2(276); // 100 us

 err=0;
 tmp=PINA;
 if (tmp!=0b01010101) err|=0x01;
 tmp=PINB;
 if ((tmp&0b10000111)!=0b10000000) err|=0x02;
 tmp=PINC;
 if ((tmp&0b00011111)!=0b00010101) err|=0x04;
 if (!(PIND&(1<<PD5))) err|=0x08;
 tmp=PINE;
 if ((tmp&0b00000011)!=0b00000001) err|=0x10;
 tmp=PING;
 if ((tmp&0b00011111)!=0b00010101) err|=0x20;
                                                // проверка ножек. 2 этап.
 PORTA=0b10101010;
 DDRA =0b01010101;
 PORTB=0b00000101;
 DDRB =0b10000010;
 PORTC=0b00001010;
 DDRC =0b00010101;
 PORTD&=~(1<<PD5);
 DDRD |= (1<<PD5);
 PORTE&=~(1<<PE0);
 DDRE |= (1<<PE0);
 PORTE|= (1<<PE1);
 DDRE &=~(1<<PE1);
 PORTG=0b00001010;
 DDRG =0b00010101;
 _delay_loop_2(276); // 100 us

 tmp=PINA;
 if (tmp!=0b10101010) err|=0x01;
 tmp=PINB;
 if ((tmp&0b10000111)!=0b00000101) err|=0x02;
 tmp=PINC;
 if ((tmp&0b00011111)!=0b00001010) err|=0x04;
 if (PIND&(1<<PD5)) err|=0x08;
 tmp=PINE;
 if ((tmp&0b00000011)!=0b00000010) err|=0x10;
 tmp=PING;
 if ((tmp&0b00011111)!=0b00001010) err|=0x20;
                                                // итог, печать результата
 if (err)
 {
  u16 ptr;
  ptr=0x0020;
  while (ptr<0x003c) *(u8*)(ptr++)=0;
  ptr=0x0061;
  while (ptr<0x0066) *(u8*)(ptr++)=0;
  print_mlmsg(mlmsg_pintest_error);
  if (err&0x01) print_msg(msg_pintest_pa);
  if (err&0x02) print_msg(msg_pintest_pb);
  if (err&0x04) print_msg(msg_pintest_pc);
  if (err&0x08) print_msg(msg_pintest_pd);
  if (err&0x10) print_msg(msg_pintest_pe);
  if (err&0x20) print_msg(msg_pintest_pg);
  print_mlmsg(mlmsg_halt);
  while (1) {}
 }
 else
 {
  print_mlmsg(mlmsg_pintest_ok);
 }
}

//-----------------------------------------------------------------------------
