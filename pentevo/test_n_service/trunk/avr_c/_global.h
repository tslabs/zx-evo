#ifndef _GLOBAL_H
#define _GLOBAL_H 1

#include <avr/io.h>

#define ENABLE_DIRECTUART       0b00000001
#define ENABLE_UART             0b00000010
#define ENABLE_SCR              0b00000100
#define ENABLE_SD_LOG           0b00001000
#define BIT_ENABLE_SD_LOG       3
#define RTSCTS_FLOWCTRL         0b00010000
//-----------------------------------------------------------------------------
//регистры fpga
#define TEMP_REG        0xA0
#define SD_CS0          0xA1
#define SD_CS1          0xA2
#define FLASH_LOADDR    0xA3
#define FLASH_MIDADDR   0xA4
#define FLASH_HIADDR    0xA5
#define FLASH_DATA      0xA6
#define FLASH_CTRL      0xA7
#define SCR_LOADDR      0xA8    // текущая позиция печати
#define SCR_HIADDR      0xA9    //
#define SCR_ATTR        0xAA    // запись атрибута в ATTR
#define SCR_FILL        0xAB    // прединкремент адреса и запись атрибута в ATTR и в память
                                // (если только дергать spics_n, будет писаться предыдущий ATTR)
#define SCR_CHAR        0xAC    // прединкремент адреса и запись символа в память и ATTR в память
                                // (если только дергать spics_n, будет писаться предыдущий символ)
#define SCR_MOUSE_TEMP  TEMP_REG
#define SCR_MOUSE_X     0xAD
#define SCR_MOUSE_Y     0xAE
#define SCR_MODE        0xAF    // .7 - 0=VGAmode, 1=TVmode; .2.1.0 - режимы;

#define MTST_CONTROL    0x50
#define MTST_PASS_CNT0  0x51
#define MTST_PASS_CNT1  TEMP_REG
#define MTST_FAIL_CNT0  0x52
#define MTST_FAIL_CNT1  TEMP_REG

#define COVOX           0x53

#define INT_CONTROL     0x54
//-----------------------------------------------------------------------------


#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
#define xl r26
#define xh r27
#define yl r28
#define yh r29
#define zl r30
#define zh r31
/* seed (32bit) */
.extern _rnd
/* буфер в RAM (размером 2048 байт) */
.extern megabuffer
/* (16bit) */
.extern mscounter

.extern flags1
.extern int6vect
.extern newframe
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

#include "_types.h"
#include "_messages.h"
#include "_output.h"

//-----------------------------------------------------------------------------

#define GO_READKEY  0
#define GO_REPEAT   1
#define GO_REDRAW   2
#define GO_RESTART  3
#define GO_CONTINUE 4
#define GO_ERROR    5
#define GO_EXIT     6

//-----------------------------------------------------------------------------

#define SetSPICS()  PORTB|=(1<<PB0)
#define ClrSPICS()  PORTB&=~(1<<PB0)

//-----------------------------------------------------------------------------

#ifdef pgm_get_far_address
#define GET_FAR_ADDRESS pgm_get_far_address
#else
#define GET_FAR_ADDRESS(var)                                            \
 ({ uint_farptr_t tmp; __asm__ __volatile__(                            \
 "ldi %A0,lo8(%1)\n\tldi %B0,hi8(%1)\n\tldi %C0,hh8(%1)\n\tclr %D0\n\t" \
 :"=d"(tmp):"p"(&(var)) ); tmp;                                         \
 })
#endif

//-----------------------------------------------------------------------------
extern u8 mode1, lang, int6vect;
extern u8 flags1;
extern volatile u8 newframe;
extern u8 megabuffer[2048];
//-----------------------------------------------------------------------------
extern u8 ee_dummy[2] EEMEM;
extern u8 ee_mode1[1] EEMEM;
extern u8 ee_lang[1] EEMEM;
//-----------------------------------------------------------------------------
//выбор текущего регистра FPGA
void fpga_sel_reg(u8 reg);
//-----------------------------------------------------------------------------
//обмен с регистрами в FPGA
u8 fpga_reg(u8 reg, u8 data);
//-----------------------------------------------------------------------------
//обмен без установки регистра
u8 fpga_same_reg(u8 data);
//-----------------------------------------------------------------------------
// п.сл.число
u8 random8(void);
//-----------------------------------------------------------------------------
// 1) запрет прерываний
// 2) инициализация SPI
// 3) распаковка данных и загрузка FPGA
void load_fpga(uint_farptr_t confdata);
//-----------------------------------------------------------------------------
void timers_init(void);
//-----------------------------------------------------------------------------
// in: timeout - таймайт, мс (1..16383)
void set_timeout_ms(u16 *storehere, u16 timeout);
//-----------------------------------------------------------------------------
u8 check_timeout_ms(u16 *storedhere);
//-----------------------------------------------------------------------------
void menu_help(void);
//-----------------------------------------------------------------------------
void toggle_vga(void);
//-----------------------------------------------------------------------------
void save_lang(void);
//-----------------------------------------------------------------------------

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _GLOBAL_H
