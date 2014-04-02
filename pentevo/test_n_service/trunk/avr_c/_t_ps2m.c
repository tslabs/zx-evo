/* 4                              5678901234567
  ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
  ³Detecting mouse...                           ³03
  ³FF FA AA 00                                  ³
  ³Customization...                             ³
  ³F3 FA C8 FA F3 FA 64 FA F3 FA 50 FA          ³
  ³F2 FA 03                                     ³
  ³E8 FA 02 FA E6 FA F3 FA 64 FA F4 FA          ³
  ³Let's go!                                    ³
  ³08 00 00 00                    ÚÄÄÄÄÄÄÄÄÄÄÄ¿ ³10
  ³                               ³ÛÛÛ        ³ ³11
  ³                               ³ÛLÛ  M   R ³ ³12
  ³                               ³ÛÛÛ        ³ ³13
  ³                               ³           ³ ³14
  ³                               ³ Wheel = 1 ³ ³15
  ³                               ³           ³ ³16
  ³                               ³ X  =  123 ³ ³17
  ³                               ³ Y  =   58 ³ ³18
  ³                               ÀÄÄÄÄÄÄÄÄÄÄÄÙ ³
  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
   4                              5678901234567  */
//-----------------------------------------------------------------------------

#include "_global.h"
#include "_screen.h"
#include "_t_ps2m.h"
#include "_ps2k.h"
#include "_uart.h"
#include <avr/interrupt.h>
#include <util/delay_basic.h>

//-----------------------------------------------------------------------------

volatile u8 ps2m_bit_count, ps2m_data, ps2m_raw_ready, ps2m_raw_code;
volatile u8 ps2m_flags;
u8 tpsm_id;

//-----------------------------------------------------------------------------

#define ps2m_dataline_up()    { DDRD&=~(1<<PD7); PORTD|=(1<<PD7); }
#define ps2m_dataline_down()  { PORTD&=~(1<<PD7); DDRD|=(1<<PD7); }
#define ps2m_clockline_up()   { DDRE&=~(1<<PE5); PORTE|=(1<<PE5); }
#define ps2m_clockline_down() { PORTE&=~(1<<PE5); DDRE|=(1<<PE5); }

//-----------------------------------------------------------------------------

const WIND_DESC wind_t_ps2m_1 PROGMEM = {  3, 2,47,19,0xdf,0x01 };
const WIND_DESC wind_t_ps2m_2 PROGMEM = {  9,10,34, 4,0xaf,0x01 };
const WIND_DESC wind_t_ps2m_3 PROGMEM = { 35,10,13,10,0xdf,0x00 };
#define p_wind_t_ps2m_1 ((const P_WIND_DESC)&wind_t_ps2m_1)
#define p_wind_t_ps2m_2 ((const P_WIND_DESC)&wind_t_ps2m_2)
#define p_wind_t_ps2m_3 ((const P_WIND_DESC)&wind_t_ps2m_3)

const u8 ps2msetup1[] PROGMEM = { 0xf3,200,0xf3,100,0xf3,80,0xff };
const u8 ps2msetup2[] PROGMEM = { 0xe8,0x02,0xe6,0xf3,100,0xf4,0xff };

//-----------------------------------------------------------------------------

u8 ps2m_send_byte(u8 data)
{
 u8 temp;
 temp=0;
 do
 {
  if (!(PINE&(1<<PE5))) temp=0;
 }while (--temp);
 cli();
 ps2m_data=data;
 ps2m_flags=1<<PS2M_BIT_TX;
 ps2m_bit_count=0;
 ps2m_raw_ready=0;
 ps2m_clockline_down();
 sei();
 _delay_loop_2(359);                            // 130 us
 ps2m_dataline_down();
 _delay_loop_1(74);                             // 20 us
 u16 to;
 set_timeout_ms(&to,15);
 ps2m_clockline_up();
 do
 {
  if (check_timeout_ms(&to))  return 0;
 }while (!(ps2m_flags&(1<<PS2M_BIT_ACKBIT)));
 return 1;
}

//-----------------------------------------------------------------------------

u8 ps2m_receive_byte(u8 *data)
{
 ps2m_raw_ready=0;
 u16 to;
 set_timeout_ms(&to,7);
 do
 {
  if (ps2m_raw_ready)
  {
   *data=ps2m_raw_code;
   ps2m_raw_ready=0;
   return 1;
  }
 }while (!(check_timeout_ms(&to)));
 return 0;
}

//-----------------------------------------------------------------------------

u8 ps2m_detect_send(u8 data)
{
 print_hexbyte_for_dump(data);
 if (ps2m_send_byte(data))
 {
  u8 response;
  if (ps2m_receive_byte(&response))
  {
   print_hexbyte_for_dump(response);
   if (response==0xfa) return 1;
  }
 }
 return 0;
}

//-----------------------------------------------------------------------------

u8 ps2m_detect_sendmulti(const u8 * array)
{
 u8 data;
 do
 {
  data=pgm_read_byte(array);
  array++;
  if (data!=0xff)
  {
   if (!(ps2m_detect_send(data))) return 0;
  }
 }while (data!=0xff);
 return 1;
}

//-----------------------------------------------------------------------------

u8 ps2m_detect_receive(u8 expected)
{
 u8 data;
 if (ps2m_receive_byte(&data))
 {
  print_hexbyte_for_dump(data);
  if (data==expected) return 1;
 }
 return 0;
}

//-----------------------------------------------------------------------------

u8 t_psm_detect(void)
{
 u8 go2;
 do
 {
  cli();
  ps2m_dataline_up();
  ps2m_clockline_up();
  EICRB=(EICRB&~((1<<ISC51)|(1<<ISC50)))|((1<<ISC51)|(0<<ISC50));
  EIMSK|=(1<<INT5);
  ps2m_flags=0;
  ps2m_bit_count=0;
  ps2m_raw_ready=0;
  sei();

  scr_window(p_wind_t_ps2m_1);
  scr_set_cursor(4,3);
  uart_crlf();
  print_mlmsg(mlmsg_mouse_detect);
  scr_set_cursor(4,4);
  uart_crlf();

  do
  {
   go2=GO_REPEAT;
   ps2m_raw_ready=0;
   u16 to;
   set_timeout_ms(&to,2);
   while (ps2m_raw_ready==0)
   {
    if (check_timeout_ms(&to))
    {
     go2=GO_CONTINUE;
     break;
    }
   }
  }while (go2==GO_REPEAT);

  const u8 * const *errmlmsg=mlmsg_mouse_fail0;
  go2=GO_ERROR;
  print_hexbyte_for_dump(0xff);
  if (ps2m_send_byte(0xff))
  {
   u8 temp;
   if (ps2m_receive_byte(&temp))
   {
    print_hexbyte_for_dump(temp);
    if (temp==0xfa)
    {
     //ps2m_raw_ready=0;
     errmlmsg=mlmsg_mouse_fail1;
     u16 to;
     set_timeout_ms(&to,1000);
     while (!(check_timeout_ms(&to)))
     {
      if (ps2m_raw_ready)
      {
       go2=GO_CONTINUE;
       break;
      }
     }
     if (go2==GO_CONTINUE)
     {
      go2=GO_ERROR;
      if (ps2m_raw_code==0xaa)
      {
       if (ps2m_detect_receive(0x00))
       {
        scr_set_cursor(4,5);
        uart_crlf();
        print_mlmsg(mlmsg_mouse_setup);
        scr_set_cursor(4,6);
        uart_crlf();
        if (ps2m_detect_sendmulti(ps2msetup1))
        {
         scr_set_cursor(4,7);
         uart_crlf();
         if (ps2m_detect_send(0xf2))
         {
          ps2m_raw_ready=0;
          set_timeout_ms(&to,20);
          while (!(check_timeout_ms(&to)))
          {
           if (ps2m_raw_ready)
           {
            go2=GO_CONTINUE;
            break;
           }
          }
          if (go2==GO_CONTINUE)
          {
           go2=GO_ERROR;
           temp=ps2m_raw_code;
           tpsm_id=temp;
           print_hexbyte_for_dump(temp);
           if ( (temp==0x00) || (temp==0x03) )
           {
            scr_set_cursor(4,8);
            uart_crlf();
            if (ps2m_detect_sendmulti(ps2msetup2))
            {
             scr_set_cursor(4,9);
             uart_crlf();
             go2=GO_CONTINUE;
            }
           }
          }
         }
        }
       }
      }
     }
    }
   }
  }

  if (go2!=GO_CONTINUE)
  {
   scr_window(p_wind_t_ps2m_2);
   scr_set_cursor(10,11);
   uart_crlf();
   print_mlmsg(errmlmsg);
   scr_set_cursor(10,12);
   print_mlmsg(mlmsg_mouse_restart);
   do
   {
    u16 key;
    key=waitkey();
    if ((u8)(key>>8)==KEY_ENTER)
     go2=GO_RESTART;
    else if ( (!((u8)key&(1<<PS2K_BIT_EXTKEY))) && ((u8)(key>>8)==KEY_ESC) )
     go2=GO_EXIT;
   }while (go2==GO_CONTINUE);
  }

 }while ( (go2!=GO_CONTINUE) && (go2!=GO_EXIT) );

 return go2;
}

//-----------------------------------------------------------------------------

void Test_PS2Mouse(void)
{
 flags1&=0b11111011;
 flags1|=0b00000010;
 print_mlmsg(mlmsg_mouse_test);
 flags1|=0b00000100;

 if (t_psm_detect()!=GO_EXIT)
 {
  print_mlmsg(mlmsg_mouse_letsgo);
  uart_crlf();
  flags1&=0b11111100;
  int6vect=0b00000010;
  fpga_reg(INT_CONTROL,int6vect);
  scr_window(p_wind_t_ps2m_3);
  if (tpsm_id)
   scr_print_msg(msg_tpsm_1);
  else
   scr_print_msg(msg_tpsm_1+10);
  u16 tpsm_x;
  u8 tpsm_y, tpsm_z, tpsm_btn;
  u8 tpsm_byte1, tpsm_byte2, tpsm_byte3, tpsm_byte4;
  tpsm_x=150;
  tpsm_y=120;
  tpsm_z=0;
  tpsm_btn=0;
  ps2m_raw_ready=0;

  u8 go2;
  do
  {
   if (tpsm_id)
   {
    scr_set_cursor(45,15);
    print_hexhalf(tpsm_z);
   }
   scr_set_cursor(41,17);
   print_dec16(tpsm_x);
   scr_set_cursor(41,18);
   print_dec16(tpsm_y);

   u8 temp;
   temp=11;
   do
   {
    scr_set_cursor(36,temp);
    u8 attr;
    if (tpsm_btn&0x01) attr=0xae; else attr=0xdf;
    scr_fill_attr(attr,3);
    scr_fill_attr(0xdf,1);
    if (tpsm_btn&0x04) attr=0xae; else attr=0xdf;
    scr_fill_attr(attr,3);
    scr_fill_attr(0xdf,1);
    if (tpsm_btn&0x02) attr=0xae; else attr=0xdf;
    scr_fill_attr(attr,3);
   }while (++temp<14);

   do
   {
    go2=GO_REPEAT;
    if (newframe)
    {
     newframe=0;
     u16 w;
     w=tpsm_x+98;
     fpga_reg(SCR_MOUSE_TEMP,(u8)(w>>8));
     fpga_reg(SCR_MOUSE_X,(u8)w);
     w=tpsm_y+43;
     fpga_reg(SCR_MOUSE_TEMP,(u8)(w>>8));
     fpga_reg(SCR_MOUSE_Y,(u8)w);
    }

    if (ps2m_raw_ready)
    {
     ps2m_raw_ready=0;
     temp=ps2m_raw_code;
     if (temp&0b00001000)
     {
      tpsm_byte1=temp;
      if (ps2m_receive_byte(&tpsm_byte2))
      {
       if (ps2m_receive_byte(&tpsm_byte3))
       {
        if (tpsm_id) temp=ps2m_receive_byte(&tpsm_byte4);
        if (temp)
        {
         tpsm_btn=tpsm_byte1&0x07;

         if (tpsm_byte1&0b00010000)
         {
          u16 w;
          w=0xff00|tpsm_byte2;
          tpsm_x+=w;
          if (tpsm_x>317) tpsm_x=0;
         }
         else
         {
          tpsm_x+=tpsm_byte2;
          if (tpsm_x>317) tpsm_x=317;
         }

         if (tpsm_byte1&0b00100000)
         {
          u16 w;
          w=(u16)tpsm_y-(0xff00|tpsm_byte3);
          if (w>249) tpsm_y=249; else tpsm_y=(u8)w;
         }
         else
         {
          u16 w;
          w=(u16)tpsm_y-tpsm_byte3;
          if (w>249) tpsm_y=0; else tpsm_y=(u8)w;
         }

         if (tpsm_id)  tpsm_z=(tpsm_z+tpsm_byte4)&0x0f;

         scr_set_cursor(4,10);
         scr_set_attr(0xdf);
         flags1|=0b00000010;
         print_hexbyte(tpsm_byte1);
         put_char(0x20);
         print_hexbyte(tpsm_byte2);
         put_char(0x20);
         print_hexbyte(tpsm_byte3);
         put_char(0x20);
         if (tpsm_id)
         {
          print_hexbyte(tpsm_byte4);
          put_char(0x20);
         }
         put_char(0x20);
         flags1&=0b11111100;
         go2=GO_REDRAW;
        }
       }
      }
     }
    }

    u16 key;
    if (inkey(&key))
    {
     if ( (!((u8)key&(1<<PS2K_BIT_EXTKEY))) && ((u8)(key>>8)==KEY_ESC) )
      go2=GO_EXIT;
    }

   }while (go2==GO_REPEAT);

  }while (go2!=GO_EXIT);

 }

 int6vect=0b00000000;
 fpga_reg(INT_CONTROL,int6vect);
 fpga_reg(SCR_MOUSE_TEMP,0);
 fpga_reg(SCR_MOUSE_X,0);
 fpga_reg(SCR_MOUSE_Y,0);
 cli();
 ps2m_dataline_up();
 ps2m_clockline_up();
 EIMSK&=~(1<<INT5);
 sei();
 flags1|=0b00000010;
}

//-----------------------------------------------------------------------------
