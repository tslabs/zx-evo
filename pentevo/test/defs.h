
// common types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;


// common constants
#define OFF = 0
#define ON = 1

// vars
static	u8 *vm = (u8 *)0x4000;

extern const u8 font[2048];

	u8 cx, cy;		// text coordinates
	u8 cc;			// text attributes
	u8 menu = 0;	// current menu
	u8 mode = 255;		// test mode
	u8 z80_lp = 1;


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

// tsconf
	// read ports
#define XSTATUS         0x00AF
#define DSTATUS         0x27AF

	// write ports
#define VCONF           0x00AF
#define VPAGE           0x01AF
#define XOFFS           0x02AF
#define XOFFSL          0x02AF
#define XOFFSH          0x03AF
#define YOFFS           0x04AF
#define YOFFSL          0x04AF
#define YOFFSH          0x05AF
#define TSCONF          0x06AF
#define PALSEL          0x07AF
#define TMPAGE          0x08AF
#define T0GPAGE         0x09AF
#define T1GPAGE         0x0AAF
#define SGPAGE          0x0BAF
#define BORDER          0x0FAF
#define PAGE0           0x10AF
#define PAGE1           0x11AF
#define PAGE2           0x12AF
#define PAGE3           0x13AF
#define FMADDR          0x15AF
#define DMASAL          0x1AAF
#define DMASAH          0x1BAF
#define DMASAX          0x1CAF
#define DMADAL          0x1DAF
#define DMADAH          0x1EAF
#define DMADAX          0x1FAF
#define SYSCONF         0x20AF
#define MEMCONF         0x21AF
#define HSINT           0x22AF
#define VSINT           0x23AF    // just alias
#define VSINTL          0x23AF
#define VSINTH          0x24AF
#define INTV            0x25AF
#define DMALEN          0x26AF
#define DMACTR          0x27AF
#define DMANUM          0x28AF
#define FDDVIRT         0x29AF

	// params
#define FM_EN           0x10
#define TS_S_EN			0x80
#define TS_T1_EN		0x40
#define TS_T0_EN		0x20

	// video modes
#define RRES_256X192    0x00
#define RRES_320X200    0x40
#define RRES_320X240    0x80
#define RRES_360X288    0xC0
#define MODE_ZX         0
#define MODE_16C        1
#define MODE_256C       2
#define MODE_TEXT       3
#define MODE_NOGFX      0x20

	// DMA
#define DMA_BSY         0x80
#define DMA_WNR         0x80
#define DMA_ZWT         0x40
#define DMA_SALGN       0x20
#define DMA_DALGN       0x10
#define DMA_ASZ         0x08
#define DMA_DEV_MEM     0x01
#define DMA_DEV_SPI     0x02
#define DMA_DEV_IDE     0x03
#define DMA_DEV_CRM     0x04
#define DMA_DEV_SFL     0x05

	// fmaps addresses
#define CRAM			0x000
#define SFILE			0x200