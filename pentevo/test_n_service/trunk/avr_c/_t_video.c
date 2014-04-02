#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"

//-----------------------------------------------------------------------------

void Test_Video(void)
{
 scr_set_cursor(0,0);
 scr_fill_char_attr(0x00,0x00,53*25);
 scr_set_cursor(15,4);
 scr_print_msg(msg_title2);

 u8 tvid_tst_cnt;
 tvid_tst_cnt=0;
 do
 {
  tvid_tst_cnt++;
  fpga_reg(SCR_MODE,tvid_tst_cnt|(mode1&0b10000000));
  if (tvid_tst_cnt>=6) tvid_tst_cnt=0;
 }while ((waitkey()>>8)!=KEY_ESC);

 fpga_reg(SCR_MODE,mode1&0b10000000);
}

//-----------------------------------------------------------------------------
