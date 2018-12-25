#define KEY_TRU     0x04    //;(CS+3)
#define KEY_PGUP    0x04
#define KEY_INV     0x05    //;(CS+4)
#define KEY_PGDN    0x05
#define KEY_CAPS    0x06    //;(CS+2)
#define KEY_EDIT    0x07    //;(CS+1)
#define KEY_LEFT    0x08    //;(CS+5)
#define KEY_RIGHT   0x09    //;(CS+8)
#define KEY_DOWN    0x0A    //;(CS+6)
#define KEY_UP      0x0B    //;(CS+7)
#define KEY_BACK    0x0C    //;(CS+0)
#define KEY_ENTER   0x0D
#define KEY_EXT     0x0E    //;(SS+CS)
#define KEY_GRAPH   0x0F    //;(CS+9)
#define KEY_DEL     0x0F
#define KEY_INS     0x10    //;(SS+W)
#define KEY_HOME    0x11    //;(SS+Q)
#define KEY_END     0x12    //;(SS+E)
#define KEY_SSENT   0x13    //;(SS+ENTER)
#define KEY_SSSP    0x14    //;(SS+SPACE)
#define KEY_CSENT   0x15    //;(CS+ENTER)
#define KEY_BREAK   0x16    //;(CS+SPACE)
#define KEY_SPACE   0x20

char gcGetKey(void) __naked;
void gcWaitKey(u8 key);
void gcWaitNoKey(void);
