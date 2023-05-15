#pragma once
#include "sysdefs.h"

typedef void (*INITIAL_FUNCTION)();
typedef u32 (*TASK_FUNCTION)(u32);

// TS ext port #AF registers
enum TSREGS
{
    TSW_VCONF    = 0x00,
    TSW_VPAGE       = 0x01,
    TSW_GXOFFSL     = 0x02,
    TSW_GXOFFSH     = 0x03,
    TSW_GYOFFSL     = 0x04,
    TSW_GYOFFSH     = 0x05,
    TSW_TSCONF      = 0x06,
    TSW_PALSEL      = 0x07,
    TSW_BORDER      = 0x0F,

    TSW_PAGE0       = 0x10,
    TSW_PAGE1       = 0x11,
    TSW_PAGE2       = 0x12,
    TSW_PAGE3       = 0x13,
    TSW_FMADDR      = 0x15,
    TSW_TMPAGE      = 0x16,
    TSW_T0GPAGE     = 0x17,
    TSW_T1GPAGE     = 0x18,
    TSW_SGPAGE      = 0x19,
    TSW_DMASAL      = 0x1A,
    TSW_DMASAH      = 0x1B,
    TSW_DMASAX      = 0x1C,
    TSW_DMADAL      = 0x1D,
    TSW_DMADAH      = 0x1E,
    TSW_DMADAX      = 0x1F,

    TSW_SYSCONF     = 0x20,
    TSW_MEMCONF     = 0x21,
    TSW_HSINT       = 0x22,
    TSW_VSINTL      = 0x23,
    TSW_VSINTH      = 0x24,
    TSW_DMALEN      = 0x26,
    TSW_DMACTR      = 0x27,
    TSW_DMANUM      = 0x28,
    TSW_FDDVIRT     = 0x29,
    TSW_INTMASK     = 0x2A,
    TSW_CACHECONF   = 0x2B,

    TSW_T0XOFFSL    = 0x40,
    TSW_T0XOFFSH    = 0x41,
    TSW_T0YOFFSL    = 0x42,
    TSW_T0YOFFSH    = 0x43,
    TSW_T1XOFFSL    = 0x44,
    TSW_T1XOFFSH    = 0x45,
    TSW_T1YOFFSL    = 0x46,
    TSW_T1YOFFSH    = 0x47,

    TSR_STATUS      = 0x00,
    TSR_PAGE2       = 0x12,
    TSR_PAGE3       = 0x13,
    TSR_DMASTATUS   = 0x27
};

// FMAPS
enum FMAPSDEV
{
    TSF_CRAM    = 0x00, // 000x a[11:9]
    TSF_SFILE   = 0x01, // 001x a[11:9]
    TSF_REGS    = 0x04  // 0100 a[11:8]
};

// INT
enum INTSRC
{
    INT_FRAME    = 0x00,
    INT_LINE,
    INT_DMA
};

// DMA devices
enum DMADEV
{
    DMA_RES1     = 0x00,
    DMA_RAMRAM   = 0x01,
    DMA_SPIRAM   = 0x02,
    DMA_IDERAM   = 0x03,
    DMA_FILLRAM  = 0x04,
    DMA_RES2     = 0x05,
    DMA_BLT2RAM  = 0x06,
    DMA_RES3     = 0x07,
    DMA_RES4     = 0x08,
    DMA_BLT1RAM  = 0x09,
    DMA_RAMSPI   = 0x0A,
    DMA_RAMIDE   = 0x0B,
    DMA_RAMCRAM  = 0x0C,
    DMA_RAMSFILE = 0x0D,
    DMA_RES5     = 0x0E,
    DMA_RES6     = 0x0F
};

enum DMA_STATE
{
  DMA_ST_RAM,
  DMA_ST_BLT1,
  DMA_ST_BLT2,
  DMA_ST_SPI_R,
  DMA_ST_SPI_W,
  DMA_ST_IDE_R,
  DMA_ST_IDE_W,
  DMA_ST_FILL,
  DMA_ST_CRAM,
  DMA_ST_SFILE,
  DMA_ST_NOP,
  DMA_ST_INIT
};

typedef struct
{
  TASK_FUNCTION task;
} DMA_TASK;

enum DMA_DATA_STATE
{
  DMA_DS_READ,
  DMA_DS_BLIT,
  DMA_DS_WRITE
};

enum TS_STATE
{
  TSS_TMAP_READ,
  TSS_TILE_RENDER,
  TSS_SPR_RENDER,
  TSS_INIT,
  TSS_NOP
};

enum TS_PWRUP
{
  TS_PWRUP_ON  = 0x40,
  TS_PWRUP_OFF = 0x00
};

enum TS_VDAC
{
  TS_VDAC_OFF = 0x00,
  TS_VDAC_3   = 0x01,
  TS_VDAC_4   = 0x02,
  TS_VDAC_5   = 0x03,
  TS_VDAC2    = 0x07,
  TS_VDAC_MAX
};

typedef struct {
  const char *name;    // displayed on config GUI
  int value;           // value from TS_VDAC:: enum
  const char *nick;    // used in *.ini
} TS_VDAC_NAME;

typedef struct
{
  INITIAL_FUNCTION init_task;
  TASK_FUNCTION task;
} TSU_TASK;

// Sprite Descriptor
typedef struct
{
  u16 y:9;
  u16 ys:3;
  u16 _0:1;
  u16 act:1;
  u16 leap:1;
  u16 yflp:1;
  u16 x:9;
  u16 xs:3;
  u16 _1:3;
  u16 xflp:1;
  u16 tnum:12;
  u16 pal:4;
} SPRITE_t;

// Tile Descriptor
typedef struct
{
  u16 tnum:12;
  u16 pal:2;
  u16 xflp:1;
  u16 yflp:1;
} TILE_t;

// TileMap description
typedef struct
{
  u8 line;
  u8 offset;
  i8 pos_dir;
  u8 pal;
  TILE_t data;
} TMAP_t;

typedef union
{
  u16 w;
  struct
  {
    u8 b0;
    u8 b1;
  };
  struct
  {
    u8 n0:4;
    u8 n1:4;
    u8 n2:4;
    u8 n3:4;
  };
} BLT16;

typedef struct
{
// -- system --
  union
  {
    u8 sysconf;
    struct
    {
      u8 zclk:2;
      u8 cache:1;
      u8 ayclk:2;
      u8 _01:3;
    };
  };

  union
  {
    u8 cacheconf;
    struct
    {
      u8 cache_win0:1;
      u8 cache_win1:1;
      u8 cache_win2:1;
      u8 cache_win3:1;
    };
  };
  bool cache_miss;

  u8 hsint;
  union
  {
    u16 vsint:9;
    struct
    {
      u8 vsintl:8;
      u8 vsinth:1;
    };
  } ;

  union
  {
    u8 intmask;
    struct
    {
      u8 intframe:1;
      u8 intline:1;
      u8 intdma:1;
    };
  };

  u8 im2vect[8];

  struct
  {
    bool new_dma;     // generate new DMA INT
    u32 last_cput;    // last cpu tacts (used by frame INT)
    u32 frame_cnt;    // frame INT counter
    u32 frame_t;      // frame INT position int tacts
    u32 frame_len;    // frame INT len
    u32 line_t;       // line INT position int tacts
    
    union
    {
      u8 pend;
      struct
      {
        u8 frame_pend:1;
        u8 line_pend:1;
        u8 dma_pend:1;
      };
    };
  } intctrl;

  u8 fddvirt;
  u8 vdos;
  u8 vdos_m1;
  u8 pwr_up;
  u8 vdac;
  bool vdac2;

// -- video --
  u8 vpage;
  u8 vpage_d;    // *_d - line-delayed
  u8 tmpage;
  u8 t0gpage[3];
  u8 t1gpage[3];
  u8 sgpage;
  u8 border;
  u8 g_yoffs_updated;

  union
    {
    u8 vconf;
    struct
    {
      u8 vmode:2;
      u8 ft_en:1;
      u8 gfxovr:1;
      u8 notsu:1;
      u8 nogfx:1;
      u8 rres:2;
    };
  };
  u8 vconf_d;

  union
  {
    u8 tsconf;
    struct
    {
      u8 t0ys_en:1;
      u8 t1ys_en:1;
      u8 t0z_en:1;
      u8 t1z_en:1;
      u8 z80_lp:1;
      u8 t0_en:1;
      u8 t1_en:1;
      u8 s_en:1;
    };
  };
  u8 tsconf_d;

  union
  {
    u8 palsel;
    struct
    {
      u8 gpal:4;
      u8 t0pal:2;
      u8 t1pal:2;
    };
  } ;
  u8 palsel_d;

  union
  {
    u16 g_xoffs:9;
    struct
    {
      u8 g_xoffsl:8;
      u8 g_xoffsh:1;
    };
  };

  union
  {
    u16 g_xoffs_d:9;
    struct
    {
      u8 g_xoffsl_d:8;
      u8 g_xoffsh_d:1;
    };
  };

  union
  {
    u16 g_yoffs:9;
    struct
    {
      u8 g_yoffsl:8;
      u8 g_yoffsh:1;
    };
  };

  union
  {
    u16 t0_xoffs:9;
    struct
    {
      u8 t0_xoffsl:8;
      u8 t0_xoffsh:1;
    };
  };
  u16 t0_xoffs_d;

  union
  {
    u16 t0_yoffs:9;
    struct
    {
      u8 t0_yoffsl:8;
      u8 t0_yoffsh:1;
    };
  };

  union
  {
    u16 t1_xoffs:9;
    struct
    {
      u8 t1_xoffsl:8;
      u8 t1_xoffsh:1;
    };
  };
  u16 t1_xoffs_d;

  union
  {
    u16 t1_yoffs:9;
    struct
    {
      u8 t1_yoffsl:8;
      u8 t1_yoffsh:1;
    };
  };

// -- memory --
  u8 page[4];

  union
  {
    u8 memconf;
    struct
    {
      u8 rom128:1;  // unused - taken from the #7FFD
      u8 w0_we:1;
      u8 w0_map_n:1;
      u8 w0_ram:1;
      u8 _03:2;
      u8 lck128:2;  // 00 - lock512, 01 - lock128, 10 - auto (!a13), 11 - lock1024
    };
  };

  union
  {
    u8 fmaddr;
    struct
    {
      u8 fm_addr:4;
      u8 fm_en:1;
    };
  } ;

// -- dma --
  u8 dmalen;
  u8 dmanum;

  union
  { // source address of dma transaction
    u32 saddr;
    struct
    {
      u32 saddrl:8;
      u32 saddrh:6;
      u32 saddrx:8;
    };
  };

  union
  { // destination address of dma transaction
    u32 daddr;
    struct
    {
      u32 daddrl:8;
      u32 daddrh:6;
      u32 daddrx:8;
    };
  };

  // dma controller state
  struct
  {
    union
    {
      u8 ctrl; // dma ctrl value
      struct
      {
        u8 dev:3;
        u8 asz:1;
        u8 d_algn:1;
        u8 s_algn:1;
        u8 opt:1;
        u8 rw:1;
      };
    };
    u16 len;
    u16 num;
    u32 saddr;      // source address of dma transaction
    u32 daddr;      // destination address of dma transaction
    u32 m1;         // mask 1 (used for address arithmetic)
    u32 m2;         // mask 2 (used for address arithmetic)
    u32 asize;      // align size
    u16 data;       // data (used by transactions based on state)
    u8 dstate;  // state of data (0 - empty, 1 - have data, 2 - have blitted data)
    u8 state;       // state of dma transaction
  } dma;

  // saved state
  struct
  {
	  u32 saddr;      // source address of dma transaction
	  u32 daddr;      // destination address of dma transaction
  } dma_saved;

  struct
  {
    u32 y;                // Y coordinate of graphic layers
    u8 tnum;              // Tile number for render
    u8 tmax;              // Max Tile number for render
    u8 tpal;              // Prepared Tile palette selector
    u8 pal;               // Prepared Tile palette
    u16 pos;              // Position in line buffer
    u16 next_pos;         // Next position in line buffer
    u16 pos_dir;          // Direction of rendering to buffer
    u8 line;              // Number of rendering line in tiles
    u8 gpage;             // Graphic Page
    u8 gsize;             // Size of graphic iterations
    u8 tz_en;             // Enable rendering Tile with number 0
    u8 *gptr;             // Pointer to the Tile graphic
    TMAP_t *tmbptr;       // Pointer to the TileMap buffer
    bool leap;            // Flag of last sprite in current layer
    u8 snum;              // Number of Sprite
    TMAP_t tm;            // TileMap data for render
    SPRITE_t spr;         // Sprite data for render

    TILE_t *tmap[2];      // TileMap pointers for layers
    u8 tmsize;            // Number of elements which need read from TileMap for current layer
    u16 tmbpos[2];        // Position in TileMap buffer for each layer
    TMAP_t tmbuf[512];    // TileMap buffer

    u8 state;             // TSU state
    u8 prev_state;        // Previous TSU state
    u8 layer;             // Active layer (0, 1, 2, etc)
    bool tmap_read;       // Flag for read TileMap in current line
    bool render;          // Flag for render graphic in current line
    struct {
        u8 t0;
        u8 t1;
        u8 s;
        u8 gfx;
    } toggle;         // TSU layers toggle
  } tsu;
} TSPORTS_t;


extern HWND tsu_toggle_wnd;
// tsu toggle layers critical section - since our toggle dialog is not modal
// we can get into a trouble because comp.ts.tsu.toggle is a shared resource
extern CRITICAL_SECTION tsu_toggle_cr;

// functions
void update_clut(u8);
void dma(u32);
u32 render_ts(u32);
void tsinit(void);
void ts_frame_int(bool);
void ts_line_int(bool);
void ts_dma_int(bool);
void invalidate_ts_cache();

void main_tsutoggle();