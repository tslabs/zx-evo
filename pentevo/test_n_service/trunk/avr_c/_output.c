#include "_global.h"
#include "_uart.h"
#include "_screen.h"

//-----------------------------------------------------------------------------

void put_char(u8 ch)
{
 if (flags1&ENABLE_DIRECTUART) directuart_putchar(ch);
 if (flags1&ENABLE_UART) uart_putchar(ch);
 if (flags1&ENABLE_SCR) scr_putchar(ch);
}

//-----------------------------------------------------------------------------

void print_hexhalf(u8 b)
{
 b&=0x0f;
 if (b>9) b+=7;
 b+=0x30;
 put_char(b);
}

//-----------------------------------------------------------------------------

void print_hexbyte(u8 b)
{
 print_hexhalf(b>>4);
 print_hexhalf(b);
}

//-----------------------------------------------------------------------------

void print_hexbyte_for_dump(u8 b)
{
 print_hexbyte(b);
 put_char(0x20);
}

//-----------------------------------------------------------------------------

void print_hexlong(u32 l)
{
 print_hexbyte((u8)(l>>24));
 print_hexbyte((u8)(l>>16));
 print_hexbyte((u8)(l>>8));
 print_hexbyte((u8)l);
}

//-----------------------------------------------------------------------------

void put_char_for_dump(u8 ch)
{
 if (ch<0x20) put_char('.'); else put_char(ch);
}

//-----------------------------------------------------------------------------

void print_dec99(u8 b)
{
 asm volatile("\tsubi %0,208\n"\
              "\tsbrs %0,7\n"\
              "\tsubi %0,48\n"\
              "\tsubi %0,232\n"\
              "\tsbrs %0,6\n"\
              "\tsubi %0,24\n"\
              "\tsubi %0,244\n"\
              "\tsbrs %0,5\n"\
              "\tsubi %0,12\n"\
              "\tsubi %0,250\n"\
              "\tsbrs %0,4\n"\
              "\tsubi %0,6\n"\
              :"=d"(b):"d"(b) );
 print_hexbyte(b);
}

//-----------------------------------------------------------------------------

const u16 decwtab[4] PROGMEM = {10000,1000,100,10};

void print_dec16(u16 w)
{
 u8 i, f;
 i=0; f=0;
 do
 {
  u16 k;
  k=pgm_read_word(&decwtab[i]);
  u8 x;
  x=0;
  while (w>=k)
  {
   w-=k;
   x++;
  }
  if ( (x==0) && (f==0) )
   put_char(' ');
  else
  {
   f=1;
   x|=0x30;
   put_char(x);
  }
  i++;
 }while (i<4);
 i=(u8)w;
 i|=0x30;
 put_char(i);
}

//-----------------------------------------------------------------------------

void print_msg(const u8 *msg)
{
 u8 ch;
 do
 {
  ch=pgm_read_byte(msg);
  msg++;
  if (ch) put_char(ch);
 }while (ch);
}

//-----------------------------------------------------------------------------

void print_mlmsg(const u8 * const *mlmsg)
{
 print_msg((const u8 *)pgm_read_word(mlmsg+lang));
}

//-----------------------------------------------------------------------------

void print_short_vers(void)
{
 u16 vers;
 vers=pgm_read_word_far(0x1dffc);
 u8 day;
 day=vers&0x1f;
 if (day)
 {
  u8 month, year;
  month=(vers>>5)&0x0f;
  year=(vers>>9)&0x3f;
  if ( (month) && (month<13) && (year>10) )
  {
   put_char('(');
   print_dec99(year);
   print_dec99(month);
   print_dec99(day);
   put_char(')');
  }
 }
}

//-----------------------------------------------------------------------------
