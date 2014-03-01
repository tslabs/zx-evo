#pragma once

// TS ext port #AF registers
#define		TSW_VCONF		0x00
#define		TSW_VPAGE       0x01
#define		TSW_GXOFFSL     0x02
#define		TSW_GXOFFSH     0x03
#define		TSW_GYOFFSL     0x04
#define		TSW_GYOFFSH     0x05
#define		TSW_TSCONF      0x06
#define		TSW_PALSEL      0x07
#define		TSW_BORDER      0x0F

#define		TSW_PAGE0       0x10
#define		TSW_PAGE1       0x11
#define		TSW_PAGE2       0x12
#define		TSW_PAGE3       0x13
#define		TSW_FMADDR      0x15
#define		TSW_TMPAGE 	    0x16
#define		TSW_T0GPAGE 	0x17
#define		TSW_T1GPAGE 	0x18
#define		TSW_SGPAGE 	    0x19
#define		TSW_DMASAL      0x1A
#define		TSW_DMASAH      0x1B
#define		TSW_DMASAX      0x1C
#define		TSW_DMADAL      0x1D
#define		TSW_DMADAH      0x1E
#define		TSW_DMADAX      0x1F

#define		TSW_SYSCONF     0x20
#define		TSW_MEMCONF     0x21
#define		TSW_HSINT       0x22
#define		TSW_VSINTL      0x23
#define		TSW_VSINTH      0x24
#define		TSW_INTVECT	    0x25
#define		TSW_DMALEN      0x26
#define		TSW_DMACTR      0x27
#define		TSW_DMANUM      0x28
#define		TSW_FDDVIRT     0x29
#define		TSW_INTMASK     0x2A

#define		TSW_T0XOFFSL 	0x40
#define		TSW_T0XOFFSH 	0x41
#define		TSW_T0YOFFSL 	0x42
#define		TSW_T0YOFFSH 	0x43
#define		TSW_T1XOFFSL 	0x44
#define		TSW_T1XOFFSH 	0x45
#define		TSW_T1YOFFSL 	0x46
#define		TSW_T1YOFFSH 	0x47

#define		TSR_STATUS	 	0x00
#define		TSR_PAGE2       0x12
#define		TSR_PAGE3       0x13
#define		TSR_DMASTATUS	0x27

// FMAPS
#define		TSF_CRAM		0x00
#define		TSF_SFILE		0x01

// DMA devices
#define		DMA_RAM			0x01
#define		DMA_SPI			0x02
#define		DMA_IDE			0x03
#define		DMA_CRAM		0x04
#define		DMA_SFILE		0x05

// Sprite Descriptor
typedef struct {
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
typedef struct {
	u16 tnum:12;
	u16 pal:2;
	u16 xflp:1;
	u16 yflp:1;
} TILE_t;

typedef struct {

// -- system --
	union {
		u8 sysconf;
		struct {
			u8 zclk:2;
			u8 cache:1;
			u8 ayclk:2;
			u8 _01:3;
		};
	} ;

	u8 hsint;
	union {
		u16 vsint:9;
		struct {
			u8 vsintl:8;
			u8 vsinth:1;
		};
	} ;

	u8 im2vect;
	u8 fddvirt;
	u8 vdos;
  u8 vdos_m1;
	u8 pwr_up;


// -- video --
	u8 vpage;
	u8 vpage_d;		// *_d - line-delayed
	u8 tmpage;
	u8 t0gpage[3];
	u8 t1gpage[3];
	u8 sgpage;
	u8 border;
	u8 g_yoffs_updated;

	union {
		u8 vconf;
		struct {
			u8 vmode:2;
			u8 _02:3;
			u8 nogfx:1;
			u8 rres:2;
		};
	};
	u8 vconf_d;

	union {
		u8 tsconf;
		struct {
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

	union {
		u8 palsel;
		struct {
			u8 gpal:4;
			u8 t0pal:2;
			u8 t1pal:2;
		};
	} ;
	u8 palsel_d;

	union {
		u16 g_xoffs:9;
		struct {
			u8 g_xoffsl:8;
			u8 g_xoffsh:1;
		};
	};

	union {
		u16 g_xoffs_d:9;
		struct {
			u8 g_xoffsl_d:8;
			u8 g_xoffsh_d:1;
		};
	};

	union {
		u16 g_yoffs:9;
		struct {
			u8 g_yoffsl:8;
			u8 g_yoffsh:1;
		};
	};

	union {
		u16 t0_xoffs:9;
		struct {
			u8 t0_xoffsl:8;
			u8 t0_xoffsh:1;
		};
	};
	u16 t0_xoffs_d[2];

	union {
		u16 t0_yoffs:9;
		struct {
			u8 t0_yoffsl:8;
			u8 t0_yoffsh:1;
		};
	};

	union {
		u16 t1_xoffs:9;
		struct {
			u8 t1_xoffsl:8;
			u8 t1_xoffsh:1;
		};
	};
	u16 t1_xoffs_d[2];

	union {
		u16 t1_yoffs:9;
		struct {
			u8 t1_yoffsl:8;
			u8 t1_yoffsh:1;
		};
	};

// -- memory --
	u8 page[4];

	union {
		u8 memconf;
		struct {
			u8 rom128:1;	// unused in Unreal - taken from the #7FFD
			u8 w0_we:1;
			u8 w0_map_n:1;
			u8 w0_ram:1;
			u8 _03:2;
			u8 lck128:2;	// 00 - no lock, 01 - lock128, 1x - auto (!a13)
		};
	};

	union {
		u8 fmaddr;
		struct {
			u8 fm_addr:4;
			u8 fm_en:1;
		};
	} ;

// -- dma --
	u8 dmalen;
	u8 dmanum;

  // dma controller state
  struct {
    union {
      u8 ctrl; // dma ctrl value
      struct {
        u8 dev:3;
        u8 asz:1;
        u8 d_algn:1;
        u8 s_algn:1;
        u8 z80_lp:1;
        u8 rw:1;
      };
    };
    u16 len;
    u16 num;
    union { // source address of dma transaction
      u32 saddr;
      struct {
        u32 saddrl:8;
        u32 saddrh:6;
        u32 saddrx:8;
      };
    };
    union { // destination address of dma transaction
      u32 daddr;
      struct {
        u32 daddrl:8;
        u32 daddrh:6;
        u32 daddrx:8;
      };
    };
    u32 next_t; // next tact to transfer data
    u16 line; // number of pixel video line
    u32 m1; // mask 1 (used for address arithmetic)
    u32 m2; // mask 2 (used for address arithmetic)
    u32 asize; // align size
    u16 data; // data (used by transactions based on state)
    u8 state; // state of dma transaction
    u8 act; // 0 - dma inactive, 1 - dma active
  } dma;
} TSPORTS_t;

typedef	union {
	u8 ctrl;
	struct {
		u8 dev:3;
		u8 asz:1;
		u8 d_algn:1;
		u8 s_algn:1;
		u8 z80_lp:1;
		u8 rw:1;
	};
} DMACTRL_t;

// functions
void update_clut(u8);
//void dma (u8);
void dma();
u16 dma_ram(u16 memcyc);
u16 dma_blt(u16 memcyc);
u16 dma_spi(u16 memcyc);
u16 dma_ide(u16 memcyc);
u16 dma_cram(u16 memcyc);
u16 dma_fill(u16 memcyc);
u16 dma_sfile(u16 memcyc);
void render_ts();
void tsinit(void);
