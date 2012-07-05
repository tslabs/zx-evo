#pragma once

#define MAX_WIDTH_P (64*2)
#define MAX_WIDTH 512
#define MAX_HEIGHT 304
#define MAX_BUFFERS 8

#define MAX_FONT_TABLES 0x62000
static const int sc2lines_width = MAX_WIDTH*2;

CACHE_ALIGNED struct T
{
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

   unsigned char attrtab[0x200]; // pc attribute + bit (pixel on/off) -> palette index

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

   union {
      struct { // 8bpp
         CACHE_ALIGNED unsigned char scale2buf[8][sc2lines_width];    // temp buffer for scale2x,3x filter
         CACHE_ALIGNED unsigned char scale4buf[8][sc2lines_width];    // temp buffer for scale4x filter
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
      unsigned char *s, *a;
   } alco[304][8];

   #ifdef MOD_VID_VD
   __m64 vdtab[2][4][256];
   #endif

};

struct videopoint
{
   unsigned next_t;
   unsigned char *screen_ptr;
   union {
      unsigned nextvmode;  // for vmode=1
      unsigned atr_offs;   // for vmode=2
   };
   unsigned scr_offs;
};

struct AtmVideoController
{
    struct ScanLine
    {
        int Offset; // смещение внутри АТМ видеостраницы
        int VideoMode; // видеорежим для данной сканлинии
    };
    void PrepareFrameATM2(int VideoMode);
    void PrepareFrameATM1(int VideoMode);

    ScanLine Scanlines[256]; // параметры 56 надбордерных сканлиний и 200 растровых сканлиний

    // Если инкременты видеоадреса происходят до фактической отрисовки сканлинии - 
    // то они применяются к её началу и сохраняются в соответствующем поле .offset этой сканлинии.
    //
    // Если инкременты видеоадреса происходят в момент отрисовки растра или сразу за ним -
    // то они применяются к следующей сканлинии.
    // Чтобы вести учёт накопления инкрементов используются следующие два поля:
    int CurrentRayLine; // номер текущей сканлинии, на которой происходит накопление инкрементов
    int IncCounter_InRaster; // счётчик для накопления +64 инкрементов, сделанных на растре
    int IncCounter_InBorder; // счётчик для накопления +64 инкрементов, сделанных на бордюре
};

extern AtmVideoController AtmVideoCtrl;

static const int rb2_offs = MAX_HEIGHT*MAX_WIDTH_P;
static const int sizeof_rbuf = rb2_offs*(MAX_BUFFERS+2);
#ifdef CACHE_ALIGNED
extern CACHE_ALIGNED unsigned char rbuf[sizeof_rbuf];
#else // __declspec(align) not available, force QWORD align with old method
extern unsigned char * const rbuf;
#endif

extern unsigned char * const save_buf;
extern unsigned char colortab[0x100];// map zx attributes to pc attributes
// colortab shifted to 8 and 24
extern unsigned colortab_s8[0x100];
extern unsigned colortab_s24[0x100];

extern unsigned *atrtab;
extern T t;

extern PALETTEENTRY pal0[0x100]; // emulator palette
extern unsigned char * const rbuf_s; // frames to mix with noflic and resampler filters
extern unsigned vmode;  // what are drawing: 0-not visible, 1-border, 2-screen
extern videopoint *vcurr;

void set_video();
void apply_video();
void paint_scr(char alt); // alt=0/1 - main/alt screen, alt=2 - ray-painted
void update_screen();
void video_timing_tables();
void pixel_tables();
void video_color_tables();
void video_permanent_tables();
void init_frame();
void flush_frame();
void load_spec_colors();
