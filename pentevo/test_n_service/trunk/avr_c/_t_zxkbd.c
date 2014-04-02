#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"
#include <util/delay_basic.h>
//
//    ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//    ³      Š« ¢¨ âãà  ZX        „¦®©áâ¨ª ³
//    ³ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿            ³
//    ³ ³ 1 2 3 4 5 6 7 8 9 0 ³  ÚÄÄÄÄÄÄÄ¿ ³
//    ³ ³ Q W E R T Y U I O P ³  ³      ³ ³
//    ³ ³ A S D F G H J K L e ³  ³ < F > ³ ³
//    ³ ³ c Z X C V B N M s s ³  ³      ³ ³
//    ³ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  ÀÄÄÄÄÄÄÄÙ ³
//    ³    ÚÄÄÄÄÄÄÄÄÄÄÄ¿   ÚÄÄÄÄÄÄÄÄÄÄ¿    ³
//    ³    ³ SoftReset ³   ³ TurboKey ³    ³
//    ³    ÀÄÄÄÄÄÄÄÄÄÄÄÙ   ÀÄÄÄÄÄÄÄÄÄÄÙ    ³
//    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
//
//
//
//   03 11 19 27 35 36 28 20 12 04
//   02 10 18 26 34 37 29 21 13 05      43
//   01 09 17 25 33 38 30 22 14 06   41 44 40
//   00 08 16 24 32 39 31 23 15 07      42
//
//              46               45
//
//-----------------------------------------------------------------------------

const WIND_DESC wind_t_zxkbd_1 PROGMEM = {  7, 6,38,12,0xdf,0x01 };
const WIND_DESC wind_t_zxkbd_2 PROGMEM = {  9, 8,23, 6,0xdf,0x00 };
const WIND_DESC wind_t_zxkbd_3 PROGMEM = { 34, 9, 9, 5,0xdf,0x00 };
const WIND_DESC wind_t_zxkbd_4 PROGMEM = { 12,14,13, 3,0xdf,0x00 };
const WIND_DESC wind_t_zxkbd_5 PROGMEM = { 28,14,12, 3,0xdf,0x00 };
#define p_wind_t_zxkbd_1 ((const P_WIND_DESC)&wind_t_zxkbd_1)
#define p_wind_t_zxkbd_2 ((const P_WIND_DESC)&wind_t_zxkbd_2)
#define p_wind_t_zxkbd_3 ((const P_WIND_DESC)&wind_t_zxkbd_3)
#define p_wind_t_zxkbd_4 ((const P_WIND_DESC)&wind_t_zxkbd_4)
#define p_wind_t_zxkbd_5 ((const P_WIND_DESC)&wind_t_zxkbd_5)

//-----------------------------------------------------------------------------

void zxkbd4(u8 data, u8 len)
{
 u8 attr;
 if (data)
 {
  if (data==0x03)
   attr=0xae;
  else
   attr=0xd1;
 }
 else
  attr=0xdf;
 scr_fill_attr(attr,len);
}

//-----------------------------------------------------------------------------

void zxkbd3(u8 data)
{
 u8 attr;
 if (data)
 {
  if (data==0x03)
   attr=0xae;
  else
   attr=0xd1;
 }
 else
  attr=0xdf;
 scr_fill_attr(attr,1);
 scr_fill_attr(0xdf,1);
}

//-----------------------------------------------------------------------------

u8 * zxkbd2(u8 *ptr, u8 data, u8 count)
{
 do
 {
  if (data&0x01)
   *ptr&=0x02;
  else
   *ptr=0x03;
  ptr++;
  data>>=1;
 }while (--count);
 return ptr;
}

//-----------------------------------------------------------------------------

u8 * zxkbd1(u8 *ptr)
{
 _delay_loop_1(255);
 return zxkbd2(ptr, PINA, 8);
}

//-----------------------------------------------------------------------------

void Test_ZXKeyb(void)
{
 scr_window(p_wind_t_zxkbd_1);
 scr_window(p_wind_t_zxkbd_2);
 scr_window(p_wind_t_zxkbd_3);
 scr_window(p_wind_t_zxkbd_4);
 scr_window(p_wind_t_zxkbd_5);
 scr_print_mlmsg(mlmsg_tzxk1);
 scr_print_msg(msg_tzxk2);

 u8 i, *ptr;
 ptr=megabuffer;
 for (i=48;i;i--) *ptr++=0;
 DDRA=0;
 PORTA=0xff;
 DDRC=0;
 PORTC=0;

 do
 {
  ptr=megabuffer;
  DDRC|=(1<<PC0);
  ptr=zxkbd1(ptr);
  DDRC&=~(1<<PC0);
  DDRC|=(1<<PC1);
  ptr=zxkbd1(ptr);
  DDRC&=~(1<<PC1);
  DDRC|=(1<<PC2);
  ptr=zxkbd1(ptr);
  DDRC&=~(1<<PC2);
  DDRC|=(1<<PC3);
  ptr=zxkbd1(ptr);
  DDRC&=~(1<<PC3);
  DDRC|=(1<<PC4);
  ptr=zxkbd1(ptr);
  DDRC&=~(1<<PC4);
  ptr=zxkbd2(ptr, PING, 5);
  ptr=zxkbd2(ptr, (PINC>>6), 2);

  ptr=megabuffer;
  scr_set_cursor(11,9);
  zxkbd3(ptr[3]);
  zxkbd3(ptr[11]);
  zxkbd3(ptr[19]);
  zxkbd3(ptr[27]);
  zxkbd3(ptr[35]);
  zxkbd3(ptr[36]);
  zxkbd3(ptr[28]);
  zxkbd3(ptr[20]);
  zxkbd3(ptr[12]);
  zxkbd3(ptr[4]);

  scr_set_cursor(11,10);
  zxkbd3(ptr[2]);
  zxkbd3(ptr[10]);
  zxkbd3(ptr[18]);
  zxkbd3(ptr[26]);
  zxkbd3(ptr[34]);
  zxkbd3(ptr[37]);
  zxkbd3(ptr[29]);
  zxkbd3(ptr[21]);
  zxkbd3(ptr[13]);
  zxkbd3(ptr[5]);

  scr_set_cursor(11,11);
  zxkbd3(ptr[1]);
  zxkbd3(ptr[9]);
  zxkbd3(ptr[17]);
  zxkbd3(ptr[25]);
  zxkbd3(ptr[33]);
  zxkbd3(ptr[38]);
  zxkbd3(ptr[30]);
  zxkbd3(ptr[22]);
  zxkbd3(ptr[14]);
  zxkbd3(ptr[6]);

  scr_set_cursor(11,12);
  zxkbd3(ptr[0]);
  zxkbd3(ptr[8]);
  zxkbd3(ptr[16]);
  zxkbd3(ptr[24]);
  zxkbd3(ptr[32]);
  zxkbd3(ptr[39]);
  zxkbd3(ptr[31]);
  zxkbd3(ptr[23]);
  zxkbd3(ptr[15]);
  zxkbd3(ptr[7]);

  scr_set_cursor(38,10);
  zxkbd3(ptr[43]);
  scr_set_cursor(36,11);
  zxkbd3(ptr[41]);
  zxkbd3(ptr[44]);
  zxkbd3(ptr[40]);
  scr_set_cursor(38,12);
  zxkbd3(ptr[42]);

  scr_set_cursor(14,15);
  zxkbd4(ptr[46],9);

  scr_set_cursor(30,15);
  zxkbd4(ptr[45],8);

  i=1;
  u16 key;
  if (inkey(&key))
   if ((u8)(key>>8)==KEY_ESC) i=0;
 }while (i);

 DDRC=0;
 PORTC=0;
}

//-----------------------------------------------------------------------------
