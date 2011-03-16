#ifndef ZX_H
#define ZX_H

#include "mytypes.h"

#define KEY_SP  0
#define KEY_EN  1
#define KEY_P   2
#define KEY_0   3
#define KEY_1   4
#define KEY_Q   5
#define KEY_A   6
#define KEY_CS  7

#define KEY_SS  8
#define KEY_L   9
#define KEY_O  10
#define KEY_9  11
#define KEY_2  12
#define KEY_W  13
#define KEY_S  14
#define KEY_Z  15

#define KEY_M  16
#define KEY_K  17
#define KEY_I  18
#define KEY_8  19
#define KEY_3  20
#define KEY_E  21
#define KEY_D  22
#define KEY_X  23

#define KEY_N  24
#define KEY_J  25
#define KEY_U  26
#define KEY_7  27
#define KEY_4  28
#define KEY_R  29
#define KEY_F  30
#define KEY_C  31

#define KEY_B  32
#define KEY_H  33
#define KEY_Y  34
#define KEY_6  35
#define KEY_5  36
#define KEY_T  37
#define KEY_G  38
#define KEY_V  39

#define NO_KEY 0xFF
#define RST_48 0xFE
#define RST128 0xFD
#define RSTRDS 0xFC
#define RSTSYS 0xFB
#define CLRKYS 0xFA

struct zx {
	UBYTE counters[40];
	UBYTE map[5]; // send order: LSbit first, from [4] to [0]
	UBYTE reset_type;
};




void zx_init(void);
void to_zx(UBYTE scancode, UBYTE was_E0, UBYTE was_release);
void update_keys(UBYTE zxcode, UBYTE was_release);

void zx_task(void);








#endif

