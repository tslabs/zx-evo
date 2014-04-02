#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"
//  5                                   1
//ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//³                                             ³
//³ e   1 2 3 4 5 6 7 8 9 0 1 2  p s p  . . .   ³07
//³                                             ³
//³ ` 1 2 3 4 5 6 7 8 9 0 - = <  i h u  n / * - ³
//³ t Q W E R T Y U I O P [ ] \  d e d  7 8 9   ³
//³ c A S D F G H J K L ; '   e         4 5 6 + ³
//³ s Z X C V B N M , . /     s        1 2 3   ³
//³ c w a       s       a w m c  <  >  0   . e ³
//³                                             ³
//³ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ³
//³ Raw data:                                   ³16
//³  00 00 00 00 00 00 00 00 00 00 00 00 00 00  ³17
//³                                             ³
//³ ’àñåªà â­®¥ ­ ¦ â¨¥ <ESC> - ¢ëå®¤ ¨§ â¥áâ   ³19
//ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
//
//-----------------------------------------------------------------------------

const WIND_DESC wind_t_ps2k PROGMEM = { 3,5,47,16,0xdf,0x01 };
#define p_wind_t_ps2k ((const P_WIND_DESC)&wind_t_ps2k)

const u8 tpsk_tab[] PROGMEM = {
 0        , 0        ,    // 0x00
 (11<<3)|0, 0        ,    // 0x01
 0        , 0        ,    // 0x02
 ( 7<<3)|0, 0        ,    // 0x03
 ( 5<<3)|0, 0        ,    // 0x04
 ( 3<<3)|0, 0        ,    // 0x05
 ( 4<<3)|0, 0        ,    // 0x06
 (14<<3)|0, 0        ,    // 0x07
 0        , 0        ,    // 0x08
 (12<<3)|0, 0        ,    // 0x09
 (10<<3)|0, 0        ,    // 0x0a
 ( 8<<3)|0, 0        ,    // 0x0b
 ( 6<<3)|0, 0        ,    // 0x0c
 ( 1<<3)|2, 0        ,    // 0x0d
 ( 1<<3)|1, 0        ,    // 0x0e
 0        , 0        ,    // 0x0f
 0        , 0        ,    // 0x10
 ( 3<<3)|5, (11<<3)|5,    // 0x11
 ( 1<<3)|4, 0        ,    // 0x12
 0        , 0        ,    // 0x13
 ( 1<<3)|5, (14<<3)|5,    // 0x14
 ( 2<<3)|2, 0        ,    // 0x15
 ( 2<<3)|1, 0        ,    // 0x16
 0        , 0        ,    // 0x17
 0        , 0        ,    // 0x18
 0        , 0        ,    // 0x19
 ( 2<<3)|4, 0        ,    // 0x1a
 ( 3<<3)|3, 0        ,    // 0x1b
 ( 2<<3)|3, 0        ,    // 0x1c
 ( 3<<3)|2, 0        ,    // 0x1d
 ( 3<<3)|1, 0        ,    // 0x1e
 0        , ( 2<<3)|5,    // 0x1f
 0        , 0        ,    // 0x20
 ( 4<<3)|4, 0        ,    // 0x21
 ( 3<<3)|4, 0        ,    // 0x22
 ( 4<<3)|3, 0        ,    // 0x23
 ( 4<<3)|2, 0        ,    // 0x24
 ( 5<<3)|1, 0        ,    // 0x25
 ( 4<<3)|1, 0        ,    // 0x26
 0        , (12<<3)|5,    // 0x27
 0        , 0        ,    // 0x28
 ( 7<<3)|5, 0        ,    // 0x29
 ( 5<<3)|4, 0        ,    // 0x2a
 ( 5<<3)|3, 0        ,    // 0x2b
 ( 6<<3)|2, 0        ,    // 0x2c
 ( 5<<3)|2, 0        ,    // 0x2d
 ( 6<<3)|1, 0        ,    // 0x2e
 0        , (13<<3)|5,    // 0x2f
 0        , 0        ,    // 0x30
 ( 7<<3)|4, 0        ,    // 0x31
 ( 6<<3)|4, 0        ,    // 0x32
 ( 7<<3)|3, 0        ,    // 0x33
 ( 6<<3)|3, 0        ,    // 0x34
 ( 7<<3)|2, 0        ,    // 0x35
 ( 7<<3)|1, 0        ,    // 0x36
 0        , 0        ,    // 0x37
 0        , 0        ,    // 0x38
 0        , 0        ,    // 0x39
 ( 8<<3)|4, 0        ,    // 0x3a
 ( 8<<3)|3, 0        ,    // 0x3b
 ( 8<<3)|2, 0        ,    // 0x3c
 ( 8<<3)|1, 0        ,    // 0x3d
 ( 9<<3)|1, 0        ,    // 0x3e
 0        , 0        ,    // 0x3f
 0        , 0        ,    // 0x40
 ( 9<<3)|4, 0        ,    // 0x41
 ( 9<<3)|3, 0        ,    // 0x42
 ( 9<<3)|2, 0        ,    // 0x43
 (10<<3)|2, 0        ,    // 0x44
 (11<<3)|1, 0        ,    // 0x45
 (10<<3)|1, 0        ,    // 0x46
 0        , 0        ,    // 0x47
 0        , 0        ,    // 0x48
 (10<<3)|4, 0        ,    // 0x49
 (11<<3)|4, (19<<3)|1,    // 0x4a
 (10<<3)|3, 0        ,    // 0x4b
 (11<<3)|3, 0        ,    // 0x4c
 (11<<3)|2, 0        ,    // 0x4d
 (12<<3)|1, 0        ,    // 0x4e
 0        , 0        ,    // 0x4f
 0        , 0        ,    // 0x50
 0        , 0        ,    // 0x51
 (12<<3)|3, 0        ,    // 0x52
 0        , 0        ,    // 0x53
 (12<<3)|2, 0        ,    // 0x54
 (13<<3)|1, 0        ,    // 0x55
 0        , 0        ,    // 0x56
 0        , 0        ,    // 0x57
 ( 1<<3)|3, 0        ,    // 0x58
 (14<<3)|4, 0        ,    // 0x59
 (14<<3)|3, (21<<3)|5,    // 0x5a
 (13<<3)|2, 0        ,    // 0x5b
 0        , 0        ,    // 0x5c
 (14<<3)|2, 0        ,    // 0x5d
 0        , 0        ,    // 0x5e
 0        , 0        ,    // 0x5f
 0        , 0        ,    // 0x60
 0        , 0        ,    // 0x61
 0        , 0        ,    // 0x62
 0        , 0        ,    // 0x63
 0        , 0        ,    // 0x64
 0        , 0        ,    // 0x65
 (14<<3)|1, 0        ,    // 0x66
 0        , 0        ,    // 0x67
 0        , 0        ,    // 0x68
 (18<<3)|4, (16<<3)|2,    // 0x69
 0        , 0        ,    // 0x6a
 (18<<3)|3, (15<<3)|5,    // 0x6b
 (18<<3)|2, (16<<3)|1,    // 0x6c
 0        , 0        ,    // 0x6d
 0        , 0        ,    // 0x6e
 0        , 0        ,    // 0x6f
 (18<<3)|5, (15<<3)|1,    // 0x70
 (20<<3)|5, (15<<3)|2,    // 0x71
 (19<<3)|4, (16<<3)|5,    // 0x72
 (19<<3)|3, 0        ,    // 0x73
 (20<<3)|3, (17<<3)|5,    // 0x74
 (19<<3)|2, (16<<3)|4,    // 0x75
 ( 1<<3)|0, 0        ,    // 0x76
 (18<<3)|1, 0        ,    // 0x77
 (13<<3)|0, 0        ,    // 0x78
 (21<<3)|3, 0        ,    // 0x79
 (20<<3)|4, (17<<3)|2,    // 0x7a
 (21<<3)|1, 0        ,    // 0x7b
 (20<<3)|1, (15<<3)|0,    // 0x7c
 (20<<3)|2, (17<<3)|1,    // 0x7d
 (16<<3)|0, (17<<3)|0,    // 0x7e
 0        , 0        };   // 0x7f

//-----------------------------------------------------------------------------

u8 tps2k_dump(u8 ptr, u8 data)
{
 ps2k_raw_ready=0;
 megabuffer[ptr]=data;
 ptr=(ptr+1)&0x0f;
 flags1&=0b11111100;
 scr_set_cursor(5,17);
 u8 p, cnt;
 p=(ptr+1)&0x0f;
 cnt=14;
 do
 {
  p=(p+1)&0x0f;
  u8 code, attr;
  code=megabuffer[p];
  if ( (code==0xe0) || (code==0xe1) )
   attr=0x0e;
  else if (code==0xf0)
   attr=0x0d;
  else if (code==0xed)
   attr=0x0b;
  else if (code>=0x85)
   attr=0x0a;
  else
   attr=0x0f;
  scr_set_attr(attr);
  scr_putchar(0x20);

  if (--cnt)
   print_hexbyte(code);
  else
  {
   flags1|=0b00000010;
   print_hexbyte(code);
   put_char(0x20);
  }
 }while (cnt);
 return ptr;
}

//-----------------------------------------------------------------------------

void Test_PS2Keyb(void)
{
 scr_window(p_wind_t_ps2k);
 scr_print_mlmsg(mlmsg_tps2k0);
 scr_print_msg(msg_tps2k1);
 scr_fill_char(0xc4,45);                        // 'Ä'
 u8 ptr;
 for (ptr=0;ptr<16;ptr++) megabuffer[ptr]=0;
 ps2k_raw_ready=0;
 u8 leds, esccnt, go2;
 leds=0x80;
 ptr=0;
 esccnt=0;

 do
 {
  go2=GO_REPEAT;

  if (ps2k_raw_ready) ptr=tps2k_dump(ptr,ps2k_raw_code);

  if (leds&0x80)
  {
   leds&=0x07;
   ptr=tps2k_dump(ptr,0xed);
   if (ps2k_send_byte(0xed))
   {
    u8 answ;
    if (ps2k_receive_byte(&answ))
    {
     ptr=tps2k_dump(ptr,answ);
     if (answ==0xfa)
     {
      ptr=tps2k_dump(ptr,leds);
      if (ps2k_send_byte(leds))
      {
       if (ps2k_receive_byte(&answ))
       {
        ptr=tps2k_dump(ptr,answ);
        if (answ==0xfa)
        {
         scr_set_cursor(41,7);
         u8 attr;
         if (leds&0x02) attr=0xdc; else attr=0xd0;
         scr_fill_attr(attr,2);
         if (leds&0x04) attr=0xdc; else attr=0xd0;
         scr_fill_attr(attr,2);
         if (leds&0x01) attr=0xdc; else attr=0xd0;
         scr_fill_attr(attr,2);
        }
       }
      }
     }
    }
   }
  }

  u16 kcf;
  kcf=ps2k_key_flags_and_code;
  u8 keycode, keyflags;
  keycode=(u8)(kcf>>8);
  keyflags=(u8)kcf;

  if (keyflags&(1<<PS2K_BIT_READY))
  {
   *(u8*)&ps2k_key_flags_and_code=0;
   if (keyflags&(1<<PS2K_BIT_RELEASE))
   {
    if (keyflags&(1<<PS2K_BIT_EXTKEY))
     esccnt=0;
    else
    {
     if (keycode!=KEY_ESC)
      esccnt=0;
     else
     {
      if (++esccnt>2) go2=GO_EXIT;
     }
    }
   }

   if (go2==GO_REPEAT)
   {
    if (!(keyflags & ((1<<PS2K_BIT_EXTKEY)|(1<<PS2K_BIT_RELEASE)) ) )
    {
     if (keycode==KEY_SCROLLLOCK) leds=(leds^0x01)|0x80;
     if (keycode==KEY_NUMLOCK)    leds=(leds^0x02)|0x80;
     if (keycode==KEY_CAPSLOCK)   leds=(leds^0x04)|0x80;
    }

    u8 temp;
    if ( (keyflags & (1<<PS2K_BIT_RELEASE)) && (keycode==0x83) )
     temp=(9<<3)|0;                             // F7
    else if (keycode==0x84)
     temp=(15<<3)|0;                            // SysReg
    else if (keycode>=0x80)
     temp=0;
    else
    {
     keycode<<=1;
     if (keyflags & (1<<PS2K_BIT_EXTKEY) )  keycode++;
     temp=pgm_read_byte(tpsk_tab+keycode);
    }

    if (temp)
    {
     u8 x,y;
     y=temp&0x07;
     if (y) y++;
     x=(temp>>2)&0x3e;
     if (x>=36) x++;
     if (x>=30) x++;
     scr_set_cursor(x+3,y+7);
     u8 attr;
     if (keyflags & (1<<PS2K_BIT_RELEASE) ) attr=0xd1; else attr=0xae;
     scr_fill_attr(attr,1);
    }
   }
  }

 }while (go2!=GO_EXIT);
 ps2k_setsysled();
}

//-----------------------------------------------------------------------------
