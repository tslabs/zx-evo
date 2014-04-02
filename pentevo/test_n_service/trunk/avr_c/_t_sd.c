#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"
#include "_uart.h"
#include "_sd.h"

//-----------------------------------------------------------------------------
const WIND_DESC wind_tsd1 PROGMEM = { 10,10,32, 4,0x9f,0x01 };
const WIND_DESC wind_tsd2 PROGMEM = {  0, 2,53,22,0xdf,0x00 };
#define p_wind_tsd1 ((const P_WIND_DESC)&wind_tsd1)
#define p_wind_tsd2 ((const P_WIND_DESC)&wind_tsd2)
//-----------------------------------------------------------------------------

void t_sd_log_crlf(void)
{
 uart_crlf();
 if (flags1&ENABLE_SD_LOG)  uart_putchar(0x3b);          // ';'
}

//-----------------------------------------------------------------------------

void t_sd_scrY_log_crlf(u8 y)
{
 scr_set_cursor(1,y);
 uart_crlf();
 if (flags1&ENABLE_SD_LOG)  uart_putchar(0x3b);          // ';'
}

//-----------------------------------------------------------------------------

void t_sd_log_acmd41(void)
{
 if (flags1&ENABLE_SD_LOG)
 {
  u8 stored_flags1;
  stored_flags1=flags1;
  flags1&=~(ENABLE_SCR|ENABLE_DIRECTUART);
  flags1|=ENABLE_UART;
  print_msg(msg_tsd_acmd41);
  flags1=stored_flags1;
 }
}

//-----------------------------------------------------------------------------

void t_sd_log_csup(void)
{
 if (flags1&ENABLE_SD_LOG)
 {
  u8 stored_flags1;
  stored_flags1=flags1;
  flags1&=~(ENABLE_SCR|ENABLE_DIRECTUART);
  flags1|=ENABLE_UART;
  print_msg(msg_tsd_csup);
  flags1=stored_flags1;
 }
}

//-----------------------------------------------------------------------------

void t_sd_log_csdown(void)
{
 if (flags1&ENABLE_SD_LOG)
 {
  u8 stored_flags1;
  stored_flags1=flags1;
  flags1&=~(ENABLE_SCR|ENABLE_DIRECTUART);
  flags1|=ENABLE_UART;
  print_msg(msg_tsd_csdown);
  flags1=stored_flags1;
 }
}

//-----------------------------------------------------------------------------

void t_sd_log_cmdxx(u8 data)
{
 if (flags1&ENABLE_SD_LOG)
 {
  u8 stored_flags1;
  stored_flags1=flags1;
  flags1&=~(ENABLE_SCR|ENABLE_DIRECTUART);
  flags1|=ENABLE_UART;
  print_msg(msg_tsd_cmd);
  print_dec99(data);
  flags1=stored_flags1;
 }
}

//-----------------------------------------------------------------------------

void t_sd_setblklen(void)
{
 // CMD16
 do
 {
  t_sd_log_cmdxx(16);
 }while (sd_cmd_with_arg(0x40|16,512));
}

//-----------------------------------------------------------------------------

void t_sd_crc_off(void)
{
 // CMD59
 do
 {
  t_sd_log_cmdxx(59);
 }while (sd_cmd_without_arg(0x40|59));
}

//-----------------------------------------------------------------------------

void t_sd_sensors(void)
{
 scr_set_cursor(0,1);
 scr_set_attr(0x7f);
 print_mlmsg(mlmsg_sensors);
 if (PINB&(1<<PB5))
 {
  scr_set_attr(0x7a);
  print_mlmsg(mlmsg_s_nocard);
 }
 else
 {
  scr_set_attr(0x7c);
  print_mlmsg(mlmsg_s_inserted);
 }
 if (PINB&(1<<PB4))
 {
  scr_set_attr(0x7a);
  print_mlmsg(mlmsg_s_readonly);
 }
 else
 {
  scr_set_attr(0x7c);
  print_mlmsg(mlmsg_s_writeen);
 }
}

//-----------------------------------------------------------------------------

void t_sd_putcursor(u8 y)
{
 u8 i;
 for (i=0;i<2;i++)
 {
  scr_set_cursor(11,11+i);
  u8 attr;
  if (i==y) attr=0xf0; else attr=0x9f;
  scr_fill_attr(attr,30);
 }
 scr_set_cursor(13,12);
 u8 ch;
 if (flags1&ENABLE_SD_LOG) ch=0xfb; else ch=0x20;        // 'û'
 scr_putchar(ch);
}

//-----------------------------------------------------------------------------

void t_sd_complete_and_waitkey(u8 y)
{
 t_sd_scrY_log_crlf(y);
 print_mlmsg(mlmsg_tsd_complete);
 flags1&=~(ENABLE_UART|ENABLE_DIRECTUART);
 flags1|=ENABLE_SCR;
 u16 key;
 while (!(inkey(&key))) t_sd_sensors();
}

//-----------------------------------------------------------------------------

void testsd(void)
{
 u8 tsd_y, t_sd_attempt, tsd_arg_acmd41, tmp, tbuf[18];
 flags1&=~(ENABLE_SCR|ENABLE_DIRECTUART);
 flags1|=ENABLE_UART;
 t_sd_log_crlf();
 t_sd_log_crlf();
 print_msg(msg_title1);
 print_short_vers();
 t_sd_log_crlf();
 flags1|=ENABLE_SCR;
 t_sd_sensors();
 scr_window(p_wind_tsd2);
 scr_set_cursor(1,3); tsd_y=3;
 t_sd_log_crlf();
 print_mlmsg(mlmsg_tsd_init);
 sd_cardtype=0;

 t_sd_log_csup();
 fpga_sel_reg(SD_CS1);
 sd_rd_dummy(32);
 t_sd_log_csdown();
 fpga_sel_reg(SD_CS0);
 // CMD0
 t_sd_attempt=255;
 do
 {
  t_sd_attempt--;
  t_sd_log_cmdxx(0);
 }while ( (sd_cmd_without_arg(0x40)!=1) && (t_sd_attempt) );

 if (t_sd_attempt==0)
 {
  t_sd_scrY_log_crlf(++tsd_y);
  print_mlmsg(mlmsg_tsd_nocard);
  t_sd_complete_and_waitkey(++tsd_y);
  return;
 }
 // CMD8
 t_sd_log_cmdxx(8);
 sd_rd_dummy(2);
 sd_exchange(0x48);
 sd_exchange(0x00);
 sd_exchange(0x00);
 sd_exchange(0x01);
 sd_exchange(0xaa);
 sd_exchange(0x87);
 if (sd_wait_notff()&0x04) tsd_arg_acmd41=0x00; else tsd_arg_acmd41=0x40;
 sd_rd_dummy(4);
 // ACMD41
 do
 {
  t_sd_log_acmd41();
  sd_cmd_without_arg(0x40|55);
  tmp=sd_cmd_with_1arg(0x40|41,tsd_arg_acmd41<<8);
 }while ( (tmp) && ((tmp&0x04)==0) );

 if (tmp)
 {
  // CMD1
  do
  {
   t_sd_log_cmdxx(1);
  }while (sd_cmd_without_arg(0x40|1));
  t_sd_crc_off();
  t_sd_setblklen();
  sd_cardtype=MMCFLAG;
  t_sd_scrY_log_crlf(++tsd_y);
  print_mlmsg(mlmsg_tsd_foundcard);
  print_msg(msg_tsd_mmc);
 }
 else
 {
  t_sd_crc_off();
  t_sd_setblklen();
  if (tsd_arg_acmd41==0)
  {
   sd_cardtype=SDV1FLAG;
   t_sd_scrY_log_crlf(++tsd_y);
   print_mlmsg(mlmsg_tsd_foundcard);
   print_msg(msg_tsd_sdv1);
  }
  else
  {
   // CMD58
   t_sd_log_cmdxx(58);
   sd_cmd_without_arg(0x40|58);
   u8 i;
   for (i=0;i<6;i++) tbuf[i]=sd_receive();
   if (tbuf[0]&0x40)
   {
    sd_cardtype=SDV2FLAG|SDHCFLAG;
    t_sd_scrY_log_crlf(++tsd_y);
    print_mlmsg(mlmsg_tsd_foundcard);
    print_msg(msg_tsd_sdhc);
   }
   else
   {
    sd_cardtype=SDV2FLAG;
    t_sd_scrY_log_crlf(++tsd_y);
    print_mlmsg(mlmsg_tsd_foundcard);
    print_msg(msg_tsd_sdhc);
   }
   t_sd_scrY_log_crlf(++tsd_y);
   print_msg(msg_tsd_ocr);
   for (i=0;i<4;i++) print_hexbyte_for_dump(tbuf[i]);
  }
 }

 fpga_sel_reg(SD_CS0);
 // CMD9
 t_sd_log_cmdxx(9);
 if (sd_cmd_without_arg(0x40|9)==0)
 {
  if (sd_wait_notff()!=0xff)
  {
   u8 i;
   for (i=0;i<18;i++) tbuf[i]=sd_receive();
   t_sd_scrY_log_crlf(++tsd_y);
   print_msg(msg_tsd_csd);
   for (i=0;i<15;i++) print_hexbyte_for_dump(tbuf[i]);
  }
 }

 fpga_sel_reg(SD_CS0);
 // CMD10
 t_sd_log_cmdxx(10);
 if (sd_cmd_without_arg(0x40|10)==0)
 {
  if (sd_wait_notff()!=0xff)
  {
   u8 i;
   for (i=0;i<18;i++) tbuf[i]=sd_receive();
   t_sd_scrY_log_crlf(++tsd_y);
   print_msg(msg_tsd_cid0);
   for (i=0;i<15;i++) print_hexbyte_for_dump(tbuf[i]);

   t_sd_scrY_log_crlf(++tsd_y);
   print_msg(msg_tsd_cid1);
   print_hexbyte(tbuf[0]);

   t_sd_scrY_log_crlf(++tsd_y);
   print_msg(msg_tsd_cid2);

   if (sd_cardtype&MMCFLAG)
   {
    print_hexbyte(tbuf[1]);
    print_hexbyte(tbuf[2]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid3);
    for (i=3;i<9;i++) put_char_for_dump(tbuf[i]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid4);
    print_hexhalf(__builtin_avr_swap(tbuf[9]));
    put_char('.');
    print_hexhalf(tbuf[9]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid5);
    for (i=10;i<14;i++) print_hexbyte(tbuf[i]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid6);
    print_dec99((__builtin_avr_swap(tbuf[14]))&0x0f);
    tmp=tbuf[14]&0x0f;
    if (tmp<3)
    {
     print_msg(msg_tsd_cid6c);
     tmp+=97;
    }
    else
    {
     print_msg(msg_tsd_cid6b);
     tmp-=3;
    }
    print_dec99(tmp);
   }
   else
   {
    put_char_for_dump(tbuf[1]);
    put_char_for_dump(tbuf[2]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid3);
    for (i=3;i<8;i++) put_char_for_dump(tbuf[i]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid4);
    print_hexhalf(__builtin_avr_swap(tbuf[8]));
    put_char('.');
    print_hexhalf(tbuf[8]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid5);
    for (i=9;i<13;i++) print_hexbyte(tbuf[i]);

    t_sd_scrY_log_crlf(++tsd_y);
    print_msg(msg_tsd_cid6);
    print_dec99(tbuf[14]&0x0f);
    print_msg(msg_tsd_cid6b);
    tmp=(tbuf[14]&0xf0) | (tbuf[13]&0x0f);
    tmp=__builtin_avr_swap(tmp);
    print_dec99(tmp);
   }
  }
 }



 t_sd_complete_and_waitkey(++tsd_y);
}

//-----------------------------------------------------------------------------

void Test_SD_MMC(void)
{
 u8 tsd_y, go2;
 do
 {
  flags1&=~(ENABLE_UART|ENABLE_DIRECTUART);
  flags1|=ENABLE_SCR;
  scr_fade();
  scr_set_cursor(0,1);
  scr_fill_char_attr(0x20,0x7f,53);
  scr_window(p_wind_tsd1);
  scr_print_mlmsg(mlmsg_tsd_menu);
  tsd_y=0;
  t_sd_putcursor(tsd_y);
  go2=GO_READKEY;
  do
  {
   u16 key;
   while (!(inkey(&key))) t_sd_sensors();
   switch ((u8)(key>>8))
   {
    case KEY_UP:
      tsd_y=0;
      t_sd_putcursor(tsd_y);
      break;
    case KEY_DOWN:
      tsd_y=1;
      t_sd_putcursor(tsd_y);
      break;
    case KEY_ENTER:
      if (tsd_y)
      {
       flags1^=ENABLE_SD_LOG;
       t_sd_putcursor(tsd_y);
      }
      else
      {
       testsd();
       go2=GO_RESTART;
      }
      break;
    case KEY_ESC:
      if (!((u8)key&(1<<PS2K_BIT_EXTKEY)))
      go2=GO_EXIT;
   }
  }while (go2==GO_READKEY);

 }while (go2!=GO_EXIT);

 flags1&=~ENABLE_SD_LOG;
}

//-----------------------------------------------------------------------------
