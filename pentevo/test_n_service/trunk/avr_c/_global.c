#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"
#include <avr/interrupt.h>

//-----------------------------------------------------------------------------

u8 mode1, lang, int6vect=0;
u8 flags1=0; // .0 - put_char вызывает directuart_putchar
             // .1 - put_char вызывает uart_putchar
             // .2 - put_char вызывает scr_putchar
             // .3 - лог обмена SD в RS-232
             // .4 - RS-232 RTS/CTS flow control
volatile u8 newframe;
volatile u16 mscounter;
u8 _rnd[4]={0x53,0x65,0x45,0x64};
u8 megabuffer[2048];

//-----------------------------------------------------------------------------

u8 ee_dummy[2] EEMEM = {0x54,0x53};
u8 ee_mode1[1] EEMEM = {0xff};
u8 ee_lang[1] EEMEM = {0};

//-----------------------------------------------------------------------------
//выбор текущего регистра FPGA
void fpga_sel_reg(u8 reg)
{
 SetSPICS();
 SPDR=reg;
 while ( !(SPSR&(1<<SPIF)) );
}

//-----------------------------------------------------------------------------
//обмен с регистрами в FPGA
u8 fpga_reg(u8 reg, u8 data)
{
 SetSPICS();
 SPDR=reg;
 while ( !(SPSR&(1<<SPIF)) );
 ClrSPICS();
 SPDR=data;
 while ( !(SPSR&(1<<SPIF)) );
 SetSPICS();
 return SPDR;
}

//-----------------------------------------------------------------------------
//обмен без установки регистра
u8 fpga_same_reg(u8 data)
{
 ClrSPICS();
 SPDR=data;
 while ( !(SPSR&(1<<SPIF)) );
 SetSPICS();
 return SPDR;
}

//-----------------------------------------------------------------------------

void timers_init(void)
{
 // timer3
 TCCR3A=(0<<WGM31)|(0<<WGM30);
 TCCR3B=(0<<WGM33)|(1<<WGM32)|(0<<CS32)|(0<<CS31)|(1<<CS30);
 OCR3A=11058; // 0x2B32
 ETIMSK|=(1<<OCIE3A);
}

//-----------------------------------------------------------------------------
// in: timeout - таймайт, мс (1..16383)
void set_timeout_ms(u16 *storehere, u16 timeout)
{
 u16 tmp;
 cli();
 tmp=mscounter;
 sei();
 *storehere=(tmp+timeout)|0x8000;
}

//-----------------------------------------------------------------------------

u8 check_timeout_ms(u16 *storedhere)
{
 if (!((*storedhere)&0x8000)) return 1;
 u16 tmp;
 cli();
 tmp=mscounter;
 sei();
 tmp-=*storedhere;
 if (!(tmp&0x4000))
 {
  *storedhere=0;
  return 1;
 }
 return 0;
}

//-----------------------------------------------------------------------------

void toggle_vga(void)
{
 mode1^=0b10000000;
 fpga_reg(SCR_MODE,mode1&0b10000000);
 _EEPUT(ee_mode1,mode1);
}

//-----------------------------------------------------------------------------

void save_lang(void)
{
 _EEPUT(ee_lang,lang);
}

//-----------------------------------------------------------------------------
