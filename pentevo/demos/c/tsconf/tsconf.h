
// ------- macros

// these are for IAR only
#define outp(a,b)   output(a, b)
#define inp(a)      input(a)

// ------- definitions

// -- TS-config port regs

#define     TS_VCONFIG      0x00AF
#define     TS_STATUS       0x00AF
#define     TS_VPAGE        0x01AF
#define     TS_GXOFFSL      0x02AF
#define     TS_GXOFFSH      0x03AF
#define     TS_GYOFFSL      0x04AF
#define     TS_GYOFFSH      0x05AF
#define     TS_TSCONFIG     0x06AF
#define     TS_PALSEL       0x07AF
#define     TS_BORDER       0x0FAF
#define     TS_PAGE0        0x10AF
#define     TS_PAGE1        0x11AF
#define     TS_PAGE2        0x12AF
#define     TS_PAGE3        0x13AF
#define     TS_FMADDR       0x15AF
#define     TS_TMPAGE       0x16AF
#define     TS_T0GPAGE      0x17AF
#define     TS_T1GPAGE      0x18AF
#define     TS_SGPAGE       0x19AF
#define     TS_DMASADDRL    0x1AAF
#define     TS_DMASADDRH    0x1BAF
#define     TS_DMASADDRX    0x1CAF
#define     TS_DMADADDRL    0x1DAF
#define     TS_DMADADDRH    0x1EAF
#define     TS_DMADADDRX    0x1FAF
#define     TS_SYSCONFIG    0x20AF
#define     TS_MEMCONFIG    0x21AF
#define     TS_HSINT        0x22AF
#define     TS_VSINTL       0x23AF
#define     TS_VSINTH       0x24AF
#define     TS_DMALEN       0x26AF
#define     TS_DMACTR       0x27AF
#define     TS_DMASTATUS    0x27AF
#define     TS_DMANUM       0x28AF
#define     TS_FDDVIRT      0x29AF
#define     TS_INTMASK      0x2AAF
#define     TS_T0XOFFSL     0x40AF
#define     TS_T0XOFFSH     0x41AF
#define     TS_T0YOFFSL     0x42AF
#define     TS_T0YOFFSH     0x43AF
#define     TS_T1XOFFSL     0x44AF
#define     TS_T1XOFFSH     0x45AF
#define     TS_T1YOFFSL     0x46AF
#define     TS_T1YOFFSH     0x47AF

// TS parameters
#define     FM_EN           0x10

// FPGA arrays
#define     FM_CRAM         0x0000
#define     FM_SFILE        0x0200

// VIDEO
#define     VID_256X192     0x00
#define     VID_320X200     0x40
#define     VID_320X240     0x80
#define     VID_360X288     0xC0
#define     VID_RASTER_BS	6

#define     VID_ZX          0x00
#define     VID_16C         0x01
#define     VID_256C        0x02
#define     VID_TEXT        0x03
#define     VID_NOGFX       0x20
#define     VID_MODE_BS		0

// PALETTE
#define     PAL_R_BS        10
#define     PAL_G_BS        5
#define     PAL_B_BS        0
#define     PAL_S_BS        15
#define     PAL_VDAC_EN     0x8000

// PALSEL
#define     PAL_GPAL_MASK	0x0F
#define     PAL_GPAL_BS		0
#define     PAL_T0PAL_MASK	0x30
#define     PAL_T0PAL_BS	4
#define     PAL_T1PAL_MASK	0xC0
#define     PAL_T1PAL_BS	6

// TSU
#define     TSU_T0ZEN		0x04
#define     TSU_T1ZEN		0x08
#define     TSU_T0EN		0x20
#define     TSU_T1EN		0x40
#define     TSU_SEN			0x80

// SYSTEM
#define     SYS_ZCLK3_5		0x00
#define     SYS_ZCLK7		0x01
#define     SYS_ZCLK14		0x02
#define     SYS_ZCLK_BS		0

#define     SYS_CACHEEN		0x04

// MEMORY
#define     MEM_ROM128		0x01
#define     MEM_W0WE		0x02
#define     MEM_W0MAP_N		0x04
#define     MEM_W0RAM		0x08

#define     MEM_LCK512		0x00
#define     MEM_LCK128		0x40
#define     MEM_LCKAUTO		0x80
#define     MEM_LCK1024		0xC0
#define     MEM_LCK_BS		6

// INT
#define     INT_VEC_FRAME	0xFF
#define     INT_VEC_LINE	0xFD
#define     INT_VEC_DMA		0xFB

#define     INT_MSK_FRAME	0x01
#define     INT_MSK_LINE	0x02
#define     INT_MSK_DMA		0x04

// DMA
#define     DMA_WNR         0x80
#define     DMA_SALGN       0x20
#define     DMA_DALGN       0x10
#define     DMA_ASZ         0x08

#define     DMA_RAM         0x01
#define     DMA_BLT         0x81
#define     DMA_FILL	    0x04
#define     DMA_SPI_RAM     0x02
#define     DMA_RAM_SPI     0x82
#define     DMA_IDE_RAM     0x03
#define     DMA_RAM_IDE     0x83
#define     DMA_RAM_CRAM    0x84
#define     DMA_RAM_SFILE   0x85

// SPRITES
#define     SP_XF			0x80
#define     SP_YF			0x80
#define     SP_LEAP			0x40
#define     SP_ACT			0x20

#define     SP_SIZE8		0x00
#define     SP_SIZE16		0x02
#define     SP_SIZE24		0x04
#define     SP_SIZE32		0x06
#define     SP_SIZE40		0x08
#define     SP_SIZE48		0x0A
#define     SP_SIZE56		0x0C
#define     SP_SIZE64		0x0E
#define     SP_SIZE_BS		1

#define     SP_PAL_MASK		0xF0

#define     SP_XF_W			0x8000
#define     SP_YF_W			0x8000
#define     SP_LEAP_W		0x4000
#define     SP_ACT_W		0x2000

#define     SP_SIZE8_W		0x0000
#define     SP_SIZE16_W		0x0200
#define     SP_SIZE24_W		0x0400
#define     SP_SIZE32_W		0x0600
#define     SP_SIZE40_W		0x0800
#define     SP_SIZE48_W		0x0A00
#define     SP_SIZE56_W		0x0C00
#define     SP_SIZE64_W		0x0E00
#define     SP_SIZE_BS_W	9

#define     SP_X_MASK_W		0x01FF
#define     SP_Y_MASK_W		0x01FF
#define     SP_TNUM_MASK_W	0x0FFF
#define     SP_PAL_MASK_W	0xF000

// TILES
#define     TL_XF			0x40
#define     TL_YF			0x80
#define     TL_PAL_MASK		0x30
#define     TL_PAL_BS		4

#define     TL_XF_W			0x4000
#define     TL_YF_W			0x8000

#define     TL_TNUM_MASK_W	0x0FFF
#define     TL_PAL_MASK_W	0x3000
#define     TL_PAL_BS_W		12


// ------- funcs

void ts_load_pal(void*, U16);
