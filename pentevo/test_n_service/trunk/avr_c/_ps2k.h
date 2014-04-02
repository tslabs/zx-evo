#ifndef _PS2K_H
#define _PS2K_H 1


#define PS2K_BIT_PARITY  0
/* расш.код */
#define PS2K_BIT_EXTKEY  1
/* отпускание */
#define PS2K_BIT_RELEASE 2
/* окончание передачи */
#define PS2K_BIT_ACKBIT  3
/* передача */
#define PS2K_BIT_TX      7

#define PS2K_BIT_READY   7


#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
.extern ps2k_bit_count
.extern ps2k_data
.extern ps2k_raw_ready
.extern ps2k_raw_code
.extern ps2k_skip
.extern ps2k_flags
.extern ps2k_key_flags_and_code
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__


#include "_types.h"

extern volatile u8 ps2k_raw_ready, ps2k_raw_code;
//extern volatile u8 ps2k_bit_count, ps2k_data;
//extern volatile u8 ps2k_skip, ps2k_flags;
extern volatile u16 ps2k_key_flags_and_code;
//extern u16 ps2k_timeout;

#define KEY_ESC         0x76
#define KEY_ENTER       0x5A
#define KEY_UP          0x75
#define KEY_DOWN        0x72
#define KEY_LEFT        0x6B
#define KEY_RIGHT       0x74
#define KEY_PAGEUP      0x7D
#define KEY_PAGEDOWN    0x7A
#define KEY_HOME        0x6C
#define KEY_END         0x69
#define KEY_SPACE       0x29
#define KEY_F1          0x05
#define KEY_NUMLOCK     0x77
#define KEY_CAPSLOCK    0x58
#define KEY_SCROLLLOCK  0x7E
#define KEY_Y           0x35

void ps2k_init(void);
void ps2k_setsysled(void);
u16 waitkey(void);
u8 inkey(u16 *key);
u8 ps2k_send_byte(u8 data);
u8 ps2k_receive_byte(u8 *data);
void ps2k_detect_kbd(void);

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _PS2K_H
