#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"

//┌──────────────────────────────┐
//│                              │
//│           12345 Гц           │
//│                              │
//│ <>, <> - изменение частоты │
//└──────────────────────────────┘

u16 t_beep_ptr, t_beep_delta;

const WIND_DESC wind_t_beep PROGMEM = { 8,8,32,6,0xdf,0x01 };
#define p_wind_t_beep ((const P_WIND_DESC)&wind_t_beep)

const u16 t_beep_freqtab[] PROGMEM =
{
   75,   45,
  107,   64,
  152,   91,
  214,  128,
  302,  181,
  427,  256,
  604,  362,
  854,  512,
 1208,  724,
 1709, 1024,
 2417, 1448,
 3418, 2048,
 4833, 2896,
 6836, 4096,
 9668, 5793
};

//-----------------------------------------------------------------------------

void Test_Beep(void)
{
 t_beep_ptr=0;
 t_beep_delta=0;
 scr_window(p_wind_t_beep);
 scr_print_mlmsg(mlmsg_tbeep);
 fpga_reg(INT_CONTROL,0b00000001);
 u8 tbeep_n, go2;
 tbeep_n=7;
 do
 {
  go2=GO_READKEY;
  scr_set_cursor(20,10);
  const u16 *ptr=&t_beep_freqtab[tbeep_n*2];
  print_dec16(pgm_read_word(ptr));
  t_beep_delta=pgm_read_word(ptr+1);
  fpga_sel_reg(COVOX);
  int6vect=0b00000001;
  // внутри этого цикла ничего не выводить в FPGA (не менять текущий регистр) !
  do
  {
   switch ((u8)(waitkey()>>8))
   {
    case KEY_ESC:
      go2=GO_EXIT;
      break;
    case KEY_UP:
      if (tbeep_n<14)
      {
       tbeep_n++;
       go2=GO_RESTART;
      }
      break;
    case KEY_DOWN:
      if (tbeep_n)
      {
       tbeep_n--;
       go2=GO_RESTART;
      }
   }
  }while (go2==GO_READKEY);
  int6vect=0;
 }while (go2!=GO_EXIT);
 fpga_reg(INT_CONTROL,0);
}

//-----------------------------------------------------------------------------
