#include "_global.h"
#include "_screen.h"

//3         4         5
//01234567890123456789012
//ÚÄÄÄÄ ’¥áâ DRAM ÄÄÄÄ¿  18
//³ à®¢¥¤¥­® æ¨ª«®¢  ³  19
//³ ¡¥§ ®è¨¡®ª   1234 ³  20
//³ á ®è¨¡ª ¬¨      0 ³  21
//ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  22

const WIND_DESC wind_t_dram_1 PROGMEM = { 30,18,21,5,0x77,0x00 };
const WIND_DESC wind_t_dram_2 PROGMEM = { 30,18,21,5,0xae,0x00 };
#define p_wind_t_dram_1 ((const P_WIND_DESC)&wind_t_dram_1)
#define p_wind_t_dram_2 ((const P_WIND_DESC)&wind_t_dram_2)

//-----------------------------------------------------------------------------

void mtst_decword(u16 data)
{
 if ((data+1)==0)  scr_putchar('>');  else  scr_putchar(' ');
 print_dec16(data);
}

//-----------------------------------------------------------------------------

void Test_DRAM(u8 callflag)
{
 //u8 stored_flags1=flags1;
 u16 mtst_pass, mtst_fail;
 mtst_pass=fpga_reg(MTST_PASS_CNT0,0xff);
 mtst_pass|=(fpga_reg(MTST_PASS_CNT1,0xff)<<8);
 mtst_fail=fpga_reg(MTST_FAIL_CNT0,0xff);
 mtst_fail|=(fpga_reg(MTST_FAIL_CNT1,0xff)<<8);

 if (mtst_fail)
 {
  scr_window(p_wind_t_dram_2);
  scr_print_mlmsg(mlmsg_mtst);
 }
 else
 {
  if (callflag)
   scr_set_attr(0x77);
  else
  {
   scr_window(p_wind_t_dram_1);
   scr_print_mlmsg(mlmsg_mtst);
  }
 }

 scr_set_cursor(43,20);
 mtst_decword(mtst_pass);
 scr_set_cursor(43,21);
 mtst_decword(mtst_fail);
}

//-----------------------------------------------------------------------------
