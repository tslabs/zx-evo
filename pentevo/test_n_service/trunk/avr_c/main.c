#include "_global.h"
#include "_output.h"
#include "_ps2k.h"
#include "_screen.h"
#include "_uart.h"
#include "_pintest.h"
#include "_t_dram.h"
#include "_t_video.h"
#include "_t_beep.h"
#include "_t_zxkbd.h"
#include "_t_ps2k.h"
#include "_t_ps2m.h"
#include "_t_rs232.h"
#include "_t_sd.h"
#include <avr/interrupt.h>
#include <util/delay_basic.h>

extern uint_farptr_t fpgadat1;

//-----------------------------------------------------------------------------

void nothing(void)
{
 asm volatile ("nop");
}

//-----------------------------------------------------------------------------

const PITEMHNDL hndl_menu_main[8] PROGMEM = {
  Test_PS2Keyb,
  Test_ZXKeyb,
  Test_PS2Mouse,
  Test_Beep,
  Test_Video,
  Test_RS232,
  Test_SD_MMC,
  nothing
};

const MENU_DESC menu_main PROGMEM = {
  6,3,26,8,
  Test_DRAM,
  1000,
  hndl_menu_main,
  str_menu_main
};
#define p_menu_main ((const P_MENU_DESC)&menu_main)

//-----------------------------------------------------------------------------

void TestFpgaExchange(void)
{
 u8 i, curr, prev;
 curr=random8();
 fpga_reg(SCR_ATTR,curr);
 i=0;
 do
 {
  prev=~curr;
  curr=random8();
  if (fpga_same_reg(curr)!=prev)
  {
   print_mlmsg(mlmsg_someerrors);
   while (1)
   {
    u16 j, errors;
    errors=0;
    j=50000;
    do
    {
     prev=~curr;
     curr=random8();
     if (fpga_same_reg(curr)!=prev) errors++;
    }while (--j);
    print_mlmsg(mlmsg_spi_test);
    print_dec16(errors);
   }
  }
 }while (--i);
}

//-----------------------------------------------------------------------------

void PowerStatus(const u8 * const *mlmsg)
{
 print_mlmsg(mlmsg);
 print_msg(msg_power_pg);
 u8 ch;
 ch='0';
 if (PINC&(1<<PC5)) ch++;
 put_char(ch);
 print_msg(msg_power_vcc5);
 ch='0';
 if (PINF&(1<<PF0)) ch++;
 put_char(ch);
}

//-----------------------------------------------------------------------------

void PowerUp(void)
{
 PowerStatus(mlmsg_statusof_crlf);
 u8 i;
 i=PINF&(1<<PF0);
 if ( (i==0) || ((PINC&(1<<PC5))==0) )
 {
  if (i==0)
  {
   print_mlmsg(mlmsg_power_on);
   while ((PINF&(1<<PF0))==0)
    PowerStatus(mlmsg_statusof_cr);
  }
  i=170;
  do
   PowerStatus(mlmsg_statusof_cr);
  while (--i);
 }
}

//-----------------------------------------------------------------------------

int main(void)
{
 cli();
 MCUCSR=0;
 _EEGET(mode1,ee_mode1);
 _EEGET(lang,ee_lang);
 if (lang>=TOTAL_LANG) lang=0;

 PinTest();                                     // проверка ножек ATMEGA128
                                                // активирует вывод через UART без буфера FIFO
 // configure pins
 PORTG = 0b11111111;
 DDRG  = 0b00000000;

 PORTF = 0b00001000;
 DDRF  = 0b00001000;

 PORTE = 0b11110011;
 DDRE  = 0b00000000;

 PORTD = 0b11111111;
 DDRD  = 0b00000000;

 PORTC = 0b11011111;
 DDRC  = 0b00000000;

 PORTB = 0b11111001;
 DDRB  = 0b10000111;

 PORTA = 0b11111111;
 DDRA  = 0b00000000;

 PowerUp();

 print_mlmsg(mlmsg_cfgfpga);
 load_fpga(GET_FAR_ADDRESS(fpgadat1));          //инициализация SPI и загрузка FPGA
 SPCR=(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA); // SPI reinit
 print_mlmsg(mlmsg_done1);

 TestFpgaExchange();
 print_msg(msg_ok);

 _delay_loop_2(553); // 200 us
 uart_init();
 ps2k_init();
 timers_init();
 EICRB|=(1<<ISC61)|(0<<ISC60);
 EIMSK|=(1<<INT6);
 sei();

 fpga_reg(SCR_MODE,mode1&0b10000000);
 fpga_reg(INT_CONTROL,0b00000000);

 ps2k_detect_kbd();

 fpga_reg(MTST_CONTROL,0b00000001);

 print_msg(msg_ready);
 ps2k_setsysled();

 while (1) scr_menu(p_menu_main);
}

//-----------------------------------------------------------------------------
