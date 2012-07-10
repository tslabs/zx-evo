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
#define		TSW_INTV 	    0x25
#define		TSW_DMALEN      0x26
#define		TSW_DMACTR      0x27
#define		TSW_DMANUM      0x28
#define		TSW_FDDVIRT     0x29

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


typedef struct {

// -- system --
	union {
		struct {
			u8 zclk:2;
			u8 _p0:1;
			u8 ayclk:2;
			u8 _p1:3;
		} i;
		u8 b;
	} sysconf;

	u8 hsint;
	union {
		struct {
			u16 l:8;
			u16 h:1;
		} b;
		u16 h;
	} vsint;

	u8 im2vect;
	u8 fddvirt;
	u8 pwr_up;


// -- video --

	// video configuration
	union {
		struct {
			u8 vmode:2;
			u8 _p:3;
			u8 nogfx:1;
			u8 rres:2;
		} i;
		u8 b;
	} vconf;

	u8 vpage;
	
	// TS configuration
	u8 tmpage;
	u8 t0gpage;
	u8 t1gpage;
	u8 sgpage;
	
	union {
		struct {
			u8 t0ys_en:1;
			u8 t1ys_en:1;
			u8 t0z_en:1;
			u8 t1z_en:1;
			u8 z80_lp:1;
			u8 t0_en:1;
			u8 t1_en:1;
			u8 s_en:1;
		} i;
		u8 b;
	} tsconf;

	// palette
	union {
		struct {
			u8 gpal:4;
			u8 t0pal:2;
			u8 t1pal:2;
		} i;
		u8 b;
	} palsel;

	// offsets
typedef struct {
	union {
		struct {
			u16 l:8;
			u16 h:1;
		} b;
		u16 h;
	} x;

	union {
		struct {
			u16 l:8;
			u16 h:1;
		} b;
		u16 h;
	} y;
} OFFS_t;

	OFFS_t g_offs;
	OFFS_t t0_offs;
	OFFS_t t1_offs;

	// other
	u8 border;


// -- video FPGA memories --
	u16 cram[256];
	u16 sfile[256];


// -- memory --
	union {
		struct {
			u8 rom128:1;	// unused
			u8 w0_we:1;
			u8 w0_map_n:1;
			u8 w0_ram:1;
			u8 _p0:2;
			u8 lck128:2;
		} i;
		u8 b;
	} memconf;

	u8 page0;
	u8 page1;
	u8 page2;
	u8 page3;

	union {
		struct {
			u8 addr:4;
			u8 fm_en:1;
		} i;
		u8 b;
	} fmaddr;


// -- dma --
typedef union {
	struct {
		u32 l:8;
		u32 h:6;
		u32 x:8;
	} b;
	u32 w;
} ADDR_t;

	ADDR_t dmasaddr;
	ADDR_t dmadaddr;

	u8 dmalen;
	u8 dmanum;

} TS_t;


typedef	union {
	struct {
		u8 dev:3;
		u8 asz:1;
		u8 d_algn:1;
		u8 s_algn:1;
		u8 z80_lp:1;
		u8 rw:1;
	} i;
	u8 b;
} DMACTRL_t;


void update_tspal(u8 addr);
