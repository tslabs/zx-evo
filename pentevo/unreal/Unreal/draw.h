#pragma once
#include "sysdefs.h"

constexpr auto MAX_WIDTH_P = (64*2);
constexpr auto MAX_WIDTH = 512;
constexpr auto MAX_HEIGHT = 320;
constexpr auto MAX_BUFFERS = 8;

constexpr auto VID_TACTS = 224;
constexpr auto VID_LINES = 320;
constexpr auto VID_WIDTH = 448;
constexpr auto VID_HEIGHT = 320;

constexpr auto MEM_CYCLES = (VID_TACTS * 2);

enum VMODE
{
	M_BRD=0	,	// Border only
	M_NUL	,	// Non-existing mode
	M_ZX	,	// Sinclair
	M_PMC	,	// Pentagon Multicolor
	M_P16	,	// Pentagon 16c
	M_P384	,	// Pentagon 384x304
	M_PHR	,	// Pentagon HiRes
	M_TS16	,	// TS 16c
	M_TS256	,	// TS 256c
	M_TSTX	,	// TS Text
	M_ATM16	,	// ATM 16c
	M_ATMHR	,	// ATM HiRes
	M_ATMTX	,	// ATM Text
	M_ATMTL ,	// ATM Text Linear
	M_PROFI ,	// Profi
	M_GMX   ,	// GMX
};

enum RASTER_N
{
	R_256_192=0,	// Sinclair
	R_320_200,		// ATM, TS
	R_320_240,		// TS
	R_360_288,		// TS
	R_384_304,		// AlCo
	R_512_240,		// Profi
	R_MAX
};

struct RASTER
{
	RASTER_N num;
	u32 u_brd;	// first pixel line
	u32 d_brd;	// first lower border line
	u32 l_brd;	// first pixel tact
	u32 r_brd;	// first right border tact
	u32 r_ts;	// tact on which call TS engine draw
};

struct VCTR
{
	u32 	clut[256];		// TS palette LUT in truecolor
	RASTER 	raster;			// raster parameters
	VMODE	mode;			// renderer mode
	VMODE	mode_next;		// renderer mode, delayed to the start of the line
	u32 	t_next;			// next tact to be rendered
	u32		vptr;			// address in videobuffer
	u32		xctr;			// videocontroller X counter
	u32		yctr;			// videocontroller absolute Y counter (used for TS)
	u32		ygctr;			// videocontroller graphics Y counter (used for graphics)
	u32 	buf;			// active video buffer
	u32 	flash;			// flash counter
	u16		line;			// current rendered line
	u16		line_pos;	// current rendered position in line
	u16		ts_pos;		// current rendered position in tsline
	u8		tsline[2][512];	// TS buffers (indexed colors)
	u16		memvidcyc[320];	// number of memory cycles used in every video line by video
	u16		memcpucyc[320];	// number of memory cycles used in every video line by CPU
	u16		memtsscyc[320];	// number of memory cycles used in every video line by TS sprites
	u16		memtstcyc[320];	// number of memory cycles used in every video line by TS tiles
	u16		memdmacyc[320]; // number of memory cycles used in every video line by DMA
	u16		memcyc_lcmd;	// number of memory cycles used in last command
};

constexpr auto MAX_FONT_TABLES = 0x62000;
constexpr auto sc2lines_width = MAX_WIDTH*2;

CACHE_ALIGNED struct T
{
#if 0
   struct { // switch palette/color values
      // 8bit
      unsigned sctab8[2][16*0x100];  //4 bits data+pc-attribute -> 4 palette pixels
      unsigned sctab8d[2][4*0x100];  //2 bits data+pc-attribute -> 2 palette pixels (doubled)
      unsigned sctab8q[2*0x100];     //1 bit  data+pc-attribute -> 1 palette pixel (quad)
      // 16bit & 32bit
      unsigned sctab16[2][4*0x100];  //2 bits data+pc-attribute -> 2 pixels
      unsigned sctab16d[2][2*0x100]; //1 bit  data+pc-attribute -> 1 pixel (doubled)
      unsigned sctab32[2][2*0x100];  //1 bit  data+pc-attribute -> 1 pixel
   };

   union { // switch chunks/noflic
      unsigned c32tab[0x100][32]; // for chunks 16/32: n_pixels+attr -> chunk color
      struct {
         unsigned sctab16_nf[2][4*0x100];  //2 bits data+pc-attribute -> 2 pixels
         unsigned sctab16d_nf[2][2*0x100]; //1 bit  data+pc-attribute -> 2 pixels
         unsigned sctab32_nf[2][2*0x100];  //1 bit  data+pc-attribute -> 1 pixel
      };
   };

   u8 attrtab[0x200]; // pc attribute + bit (pixel on/off) -> palette index

   CACHE_ALIGNED union {
      unsigned p4bpp8[2][0x100];   // ATM EGA screen. EGA byte -> raw video data: 2 pixels (doubled) (p2p2p1p1)
      unsigned p4bpp16[2][2*0x100];// ATM EGA screen. EGA byte -> raw video data: 2 pixels (doubled)
      unsigned p4bpp32[2][2*0x100];// ATM EGA screen. EGA byte -> raw video data: 2 pixels
   };
   CACHE_ALIGNED union {
      unsigned p4bpp16_nf[2][2*0x100];// ATM EGA screen. EGA byte -> raw video data: 2 pixels (doubled)
      unsigned p4bpp32_nf[2][2*0x100];// ATM EGA screen. EGA byte -> raw video data: 2 pixels
   };
   CACHE_ALIGNED union {
      struct {
         unsigned zctab8[2][16*0x100];  // 4 bits data+zx-attribute -> 4 palette pixels
         unsigned zctab8ad[2][4*0x100]; // 2 bits data+zx-attribute -> 2 palette pixels (doubled)
      };
      struct {
         unsigned zctab16[2][4*0x100];  // 2 bits data+pc-attribute -> 2 pixels
         unsigned zctab16ad[2][2*0x100];// 1 bits data+pc-attribute -> 1 pixel (doubled)
      };
      struct {
         unsigned zctab32[2][2*0x100];  // 1 bit  data+pc-attribute -> 1 pixel
         unsigned zctab32ad[2][2*0x100];// 1 bit  data+pc-attribute -> 1 pixel
      };
   };
#endif
   union {
      struct { // 8bpp
         CACHE_ALIGNED u8 scale2buf[8][sc2lines_width];    // temp buffer for scale2x,3x filter
         CACHE_ALIGNED u8 scale4buf[8][sc2lines_width];    // temp buffer for scale4x filter
      };
      struct // 32 bpp
      {
         CACHE_ALIGNED u32 scale2buf32[8][sc2lines_width];    // temp buffer for scale2x,3x filter
         CACHE_ALIGNED u32 scale4buf32[8][sc2lines_width];    // temp buffer for scale4x filter
      };
      unsigned bs2h[96][129];      // temp buffer for chunks 2x2 flt
      unsigned bs4h[48][65];       // temp buffer for chunks 4x4 flt
      void* font_tables[MAX_FONT_TABLES / sizeof(void*)]; // for anti-text64
   };
   // pre-calculated
   unsigned settab[0x100];         // for chunks 4x4
   unsigned settab2[0x100];        // for chunks 2x2
   unsigned dbl[0x100]; // reverse and double pixels (for scaling renderer)
   unsigned scrtab[256]; // offset to start of line
   unsigned atrtab[256]; // offset to start of attribute line
   unsigned atrtab_hwmc[256]; // attribute for HWMC
   unsigned atm_pal_map[0x100]; // atm palette port value -> palette index
   struct {   // for AlCo-384
      u8 *s, *a;
   } alco[304][8];

   #ifdef MOD_VID_VD
   __m64 vdtab[2][4][256];
   #endif

};

struct videopoint
{
   unsigned next_t;
   u8 *screen_ptr;
   union {
      unsigned nextvmode;  // for vmode=1
      unsigned atr_offs;   // for vmode=2
   };
   unsigned scr_offs;
};

static const int rb2_offs = MAX_HEIGHT*MAX_WIDTH_P;
static const int sizeof_rbuf = rb2_offs*(MAX_BUFFERS+2);
static const int sizeof_vbuf = VID_HEIGHT*VID_WIDTH*2;
/* not really needed anymore */
#ifdef CACHE_ALIGNED
extern CACHE_ALIGNED u8 rbuf[sizeof_rbuf];
#else // __declspec(align) not available, force u64 align with old method
extern u8 * const rbuf;
#endif

extern u8 * const save_buf;
extern u8 colortab[0x100];// map zx attributes to pc attributes
// colortab shifted to 8 and 24
extern unsigned colortab_s8[0x100];
extern unsigned colortab_s24[0x100];

extern unsigned *atrtab;
extern T t;

extern PALETTEENTRY pal0[0x100]; // emulator palette
extern u8 * const rbuf_s; // frames to mix with noflic and resampler filters
extern unsigned vmode;  // what are drawing: 0-not visible, 1-border, 2-screen
extern videopoint *vcurr;


// functions
void set_video();
void apply_video();
void paint_scr(char alt); // alt=0/1 - main/alt screen, alt=2 - ray-painted
void update_screen();
void pixel_tables();
void video_color_tables();
void video_permanent_tables();
void init_frame();
void flush_frame();
void load_spec_colors();
void init_raster();
void update_raypos(bool showleds = true);