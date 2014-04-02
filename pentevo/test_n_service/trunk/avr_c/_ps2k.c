#include "_global.h"
#include "_ps2k.h"
#include <avr/interrupt.h>
#include <util/delay_basic.h>

//-----------------------------------------------------------------------------

volatile u8 ps2k_bit_count=0, ps2k_data, ps2k_raw_ready, ps2k_raw_code;
volatile u8 ps2k_skip=0, ps2k_flags=0;
volatile u16 ps2k_key_flags_and_code=0;

#define ps2k_dataline_up()    { DDRD&=~(1<<PD6); PORTD|=(1<<PD6); }
#define ps2k_dataline_down()  { PORTD&=~(1<<PD6); DDRD|=(1<<PD6); }
#define ps2k_clockline_up()   { DDRE&=~(1<<PE4); PORTE|=(1<<PE4); }
#define ps2k_clockline_down() { PORTE&=~(1<<PE4); DDRE|=(1<<PE4); }

//-----------------------------------------------------------------------------

void ps2k_init(void)
{
 EICRB|=(1<<ISC41)|(0<<ISC40);
 EIMSK|=(1<<INT4);
 TCCR0=(0<<CS02)|(1<<CS01)|(0<<CS00); // clk/8
}

//-----------------------------------------------------------------------------

void ps2k_setsysled(void)
{
 if (ps2k_send_byte(0xed))
 {
  u8 answ;
  if (ps2k_receive_byte(&answ))
  {
   if (answ==0xfa)
   {
    u8 leds;
    if (mode1&0x80) leds=0x00; else leds=0x01;
    ps2k_send_byte(leds);
   }
  }
 }
}

//-----------------------------------------------------------------------------
// out: hi8 - key code, lo8 - flags
u16 waitkey(void)
{
 u16 tmp;
 do
 {
  do
  {
   random8();
   tmp=ps2k_key_flags_and_code;
  }while (!((u8)tmp&(1<<PS2K_BIT_READY)));
  *(u8*)&ps2k_key_flags_and_code=0;
 }while ((u8)tmp&(1<<PS2K_BIT_RELEASE));
 return tmp;
}

//-----------------------------------------------------------------------------

u8 inkey(u16 *key)
{
 random8();
 u16 tmp;
 tmp=ps2k_key_flags_and_code;
 if (!((u8)tmp&(1<<PS2K_BIT_READY))) return 0;
 *(u8*)&ps2k_key_flags_and_code=0;
 if ((u8)tmp&(1<<PS2K_BIT_RELEASE)) return 0;
 *key=tmp;
 return 1;
}

//-----------------------------------------------------------------------------

u8 ps2k_send_byte(u8 data)
{
 u8 tmp, pin;
 u16 ps2k_timeout;
 do
 {
  ps2k_dataline_up();
  TIMSK&=~(1<<TOIE0);
  ps2k_clockline_up();
  tmp=0;
  do
   pin=PINE&(1<<PE4);
  while ( (pin) && (--tmp) );
 }while (pin==0);

 cli();
 ps2k_clockline_down();
 ps2k_dataline_down();
 ps2k_data=data;
 ps2k_flags=1<<PS2K_BIT_TX;
 ps2k_bit_count=0;
 ps2k_skip=0;
 *(u8*)&ps2k_key_flags_and_code=0;
 ps2k_raw_ready=0;
 sei();
// _delay_loop_2(276);  // 100 us
// ps2k_dataline_down();
 _delay_loop_2(1382); // 500 us
 set_timeout_ms(&ps2k_timeout,15);
 ps2k_clockline_up();
 do
 {
  if (ps2k_flags&(1<<PS2K_BIT_ACKBIT)) return 1;
 }while (!(check_timeout_ms(&ps2k_timeout)));
 ps2k_dataline_up();
 return 0;
}

//-----------------------------------------------------------------------------

u8 ps2k_receive_byte(u8 *data)
{
 u16 ps2k_timeout;
 ps2k_raw_ready=0;
 set_timeout_ms(&ps2k_timeout,20);
 do
 {
  if (ps2k_raw_ready)
  {
   *data=ps2k_raw_code;
   return 1;
  }
 }while (!(check_timeout_ms(&ps2k_timeout)));
 // *data=0;
 return 0;
}

//-----------------------------------------------------------------------------

u8 ps2k_detect_send(u8 data)
{
 print_hexbyte_for_dump(data);
 if (!(ps2k_send_byte(data)))
 {
  print_mlmsg(mlmsg_txfail);
  return 0;
 }
 u8 rxdata;
 if (!(ps2k_receive_byte(&rxdata)))
 {
  print_mlmsg(mlmsg_noresponse);
  return 0;
 }
 print_hexbyte_for_dump(rxdata);
 if (rxdata!=0xfa)
 {
  print_mlmsg(mlmsg_unwanted);
  return 0;
 }
 return 1;
}

//-----------------------------------------------------------------------------

u8 ps2k_detect_receive(u8 data)
{
 u8 rxdata;
 if (!(ps2k_receive_byte(&rxdata)))
 {
  print_mlmsg(mlmsg_noresponse);
  return 0;
 }
 print_hexbyte_for_dump(rxdata);
 if (rxdata!=data)
 {
  print_mlmsg(mlmsg_unwanted);
  return 0;
 }
 return 1;
}

//-----------------------------------------------------------------------------

void ps2k_detect_kbd(void)
{
 ps2k_clockline_up();
 print_mlmsg(mlmsg_kbd_detect);

 u16 ps2k_timeout;
 ps2k_raw_ready=0;
 set_timeout_ms(&ps2k_timeout,500);
 do
 {
  if (ps2k_raw_ready)
  {
   print_hexbyte_for_dump(ps2k_raw_code);
   break;
  }
 }while (!(check_timeout_ms(&ps2k_timeout)));

 ps2k_detect_send(0xff);
 ps2k_raw_ready=0;
 set_timeout_ms(&ps2k_timeout,1000);
 do
 {
  if (ps2k_raw_ready)
  {
   u8 tmp;
   tmp=ps2k_raw_code;
   print_hexbyte_for_dump(tmp);
   if (tmp!=0xaa) print_mlmsg(mlmsg_unwanted);
   break;
  }
 }while (!(check_timeout_ms(&ps2k_timeout)));
 if (!(ps2k_raw_ready)) print_mlmsg(mlmsg_noresponse);

 if (ps2k_detect_send(0xf2))
 {
  if (ps2k_detect_receive(0xab))
   ps2k_detect_receive(0x83);
 }
 if (ps2k_detect_send(0xf0))
  ps2k_detect_send(0x02);
 if (ps2k_detect_send(0xf3))
 {
  ps2k_detect_send(0x00);
  put_char('\r');
  put_char('\n');
 }
}

//-----------------------------------------------------------------------------
