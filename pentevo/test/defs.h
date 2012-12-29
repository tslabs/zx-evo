
// common types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

// vars
static	u8 *vm = (u8 *)0x4000;

extern const u8 font[2048];

	u8 cx, cy;		// text coordinates
	u8 cc;			// text attributes
	u8 menu = 0;	// current menu

// ports
sfr pFE = 0xFE;

// keys
#define K_CS	0
#define K_Z		1
#define K_X		2
#define K_C		3
#define K_V		4
#define K_A		5
#define K_S		6
#define K_D		7
#define K_F		8
#define K_G		9
#define K_Q		10
#define K_W		11
#define K_E		12
#define K_R		13
#define K_T		14
#define K_1		15
#define K_2		16
#define K_3		17
#define K_4		18
#define K_5		19
#define K_0		20
#define K_9		21
#define K_8		22
#define K_7		23
#define K_6		24
#define K_P		25
#define K_O		26
#define K_I		27
#define K_U		28
#define K_Y		29
#define K_EN	30
#define K_L		31
#define K_K		32
#define K_J		33
#define K_H		34
#define K_SP	35
#define K_SS	36
#define K_M		37
#define K_N		38
#define K_B		39

// colors
#define C_NORM 0x47
#define C_HEAD 0x45
#define C_INFO 0x44
#define C_WARN 0x42
#define C_QUST 0x46
#define C_ACHT 0x43
#define C_FRAM 0x07

// menus
#define M_MAIN	0x00
#define M_INFO	0x10
#define M_AY	0x20
#define M_SSG	0x30
#define M_SAVE	0x40
#define M_SAVE1	0x41
#define M_FUPD	0x50
#define M_FUPD1	0x51
