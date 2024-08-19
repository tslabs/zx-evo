
#pragma once

__sfr __at 0x57 SPI_DATA;
__sfr __at 0x77 SPI_CTRL;

__sfr __at 0xFE BORDER;

__sfr __banked __at 0x00AF TS_VCONFIG;
__sfr __banked __at 0x00AF TS_STATUS;
__sfr __banked __at 0x01AF TS_VPAGE;
__sfr __banked __at 0x02AF TS_GXOFFSL;
__sfr __banked __at 0x03AF TS_GXOFFSH;
__sfr __banked __at 0x04AF TS_GYOFFSL;
__sfr __banked __at 0x05AF TS_GYOFFSH;
__sfr __banked __at 0x06AF TS_TSCONFIG;
__sfr __banked __at 0x07AF TS_PALSEL;
__sfr __banked __at 0x0FAF TS_BORDER;
__sfr __banked __at 0x10AF TS_PAGE0;
__sfr __banked __at 0x11AF TS_PAGE1;
__sfr __banked __at 0x12AF TS_PAGE2;
__sfr __banked __at 0x13AF TS_PAGE3;
__sfr __banked __at 0x15AF TS_FMADDR;
__sfr __banked __at 0x16AF TS_TMPAGE;
__sfr __banked __at 0x17AF TS_T0GPAGE;
__sfr __banked __at 0x18AF TS_T1GPAGE;
__sfr __banked __at 0x19AF TS_SGPAGE;
__sfr __banked __at 0x1AAF TS_DMASADDRL;
__sfr __banked __at 0x1BAF TS_DMASADDRH;
__sfr __banked __at 0x1CAF TS_DMASADDRX;
__sfr __banked __at 0x1DAF TS_DMADADDRL;
__sfr __banked __at 0x1EAF TS_DMADADDRH;
__sfr __banked __at 0x1FAF TS_DMADADDRX;
__sfr __banked __at 0x20AF TS_SYSCONFIG;
__sfr __banked __at 0x21AF TS_MEMCONFIG;
__sfr __banked __at 0x22AF TS_HSINT;
__sfr __banked __at 0x23AF TS_VSINTL;
__sfr __banked __at 0x24AF TS_VSINTH;
__sfr __banked __at 0x25AF TS_DMAWPD;
__sfr __banked __at 0x26AF TS_DMALEN;
__sfr __banked __at 0x27AF TS_DMACTR;
__sfr __banked __at 0x27AF TS_DMASTATUS;
__sfr __banked __at 0x28AF TS_DMANUM;
__sfr __banked __at 0x29AF TS_FDDVIRT;
__sfr __banked __at 0x2AAF TS_INTMASK;
__sfr __banked __at 0x2BAF TS_CACHECONF;
__sfr __banked __at 0x2CAF TS_DMANUMH;
__sfr __banked __at 0x2DAF TS_DMAWPA;
__sfr __banked __at 0x40AF TS_T0XOFFSL;
__sfr __banked __at 0x41AF TS_T0XOFFSH;
__sfr __banked __at 0x42AF TS_T0YOFFSL;
__sfr __banked __at 0x43AF TS_T0YOFFSH;
__sfr __banked __at 0x44AF TS_T1XOFFSL;
__sfr __banked __at 0x45AF TS_T1XOFFSH;
__sfr __banked __at 0x46AF TS_T1YOFFSL;
__sfr __banked __at 0x47AF TS_T1YOFFSH;

// TS parameters
#define TS_FM_EN            0x10

// FPGA arrays
#define TS_FM_CRAM          0x0000
#define TS_FM_SFILE         0x0200

// VIDEO
#define TS_VID_256X192      0x00
#define TS_VID_320X200      0x40
#define TS_VID_320X240      0x80
#define TS_VID_360X288      0xC0
#define TS_VID_RASTER_BS    6

#define TS_VID_ZX           0x00
#define TS_VID_16C          0x01
#define TS_VID_256C         0x02
#define TS_VID_TEXT         0x03
#define TS_VID_FT812        0x04
#define TS_VID_NOGFX        0x20
#define TS_VID_MODE_BS      0

// PALSEL
#define TS_PAL_GPAL_MASK    0x0F
#define TS_PAL_GPAL_BS      0
#define TS_PAL_T0PAL_MASK   0x30
#define TS_PAL_T0PAL_BS     4
#define TS_PAL_T1PAL_MASK   0xC0
#define TS_PAL_T1PAL_BS     6

// TSU
#define TS_TSU_T0ZEN      0x04
#define TS_TSU_T1ZEN      0x08
#define TS_TSU_T0EN       0x20
#define TS_TSU_T1EN       0x40
#define TS_TSU_SEN        0x80

// SYSTEM
#define TS_SYS_ZCLK3_5    0x00
#define TS_SYS_ZCLK7      0x01
#define TS_SYS_ZCLK14     0x02
#define TS_SYS_ZCLK_BS    0

#define TS_SYS_CACHEEN    0x04

// MEMORY
#define TS_MEM_ROM128     0x01
#define TS_MEM_W0WE       0x02
#define TS_MEM_W0MAP_N    0x04
#define TS_MEM_W0RAM      0x08

#define TS_MEM_LCK512     0x00
#define TS_MEM_LCK128     0x40
#define TS_MEM_LCKAUTO    0x80
#define TS_MEM_LCK1024    0xC0
#define TS_MEM_LCK_BS     6

// INT
#define TS_INT_VEC_FRAME    0xFF
#define TS_INT_VEC_LINE     0xFD
#define TS_INT_VEC_DMA      0xFB
#define TS_INT_VEC_WTP      0xF9

#define TS_INT_MSK_FRAME    0x01
#define TS_INT_MSK_LINE     0x02
#define TS_INT_MSK_DMA      0x04
#define TS_INT_MSK_WTP      0x08

// DMA
#define TS_DMA_WNR          0x80
#define TS_DMA_SALGN        0x20
#define TS_DMA_DALGN        0x10
#define TS_DMA_ASZ          0x08

#define TS_DMA_RAM          0x01
#define TS_DMA_BLT          0x81
#define TS_DMA_FILL         0x04
#define TS_DMA_SPI_RAM      0x02
#define TS_DMA_WTP_RAM      0x07
#define TS_DMA_RAM_SPI      0x82
#define TS_DMA_IDE_RAM      0x03
#define TS_DMA_RAM_IDE      0x83
#define TS_DMA_RAM_CRAM     0x84
#define TS_DMA_RAM_SFILE    0x85

#define TS_WPD_GLU      0
#define TS_WPD_COM      1

// SPRITES
#define TS_SP_XF        0x80
#define TS_SP_YF        0x80
#define TS_SP_LEAP      0x40
#define TS_SP_ACT       0x20

#define TS_SP_SIZE8     0x00
#define TS_SP_SIZE16    0x02
#define TS_SP_SIZE24    0x04
#define TS_SP_SIZE32    0x06
#define TS_SP_SIZE40    0x08
#define TS_SP_SIZE48    0x0A
#define TS_SP_SIZE56    0x0C
#define TS_SP_SIZE64    0x0E
#define TS_SP_SIZE_BS   1

#define TS_SP_PAL_MASK      0xF0

#define TS_SP_XF_W          0x8000
#define TS_SP_YF_W          0x8000
#define TS_SP_LEAP_W        0x4000
#define TS_SP_ACT_W         0x2000

#define TS_SP_SIZE8_W       0x0000
#define TS_SP_SIZE16_W      0x0200
#define TS_SP_SIZE24_W      0x0400
#define TS_SP_SIZE32_W      0x0600
#define TS_SP_SIZE40_W      0x0800
#define TS_SP_SIZE48_W      0x0A00
#define TS_SP_SIZE56_W      0x0C00
#define TS_SP_SIZE64_W      0x0E00
#define TS_SP_SIZE_BS_W     9

#define TS_SP_X_MASK_W      0x01FF
#define TS_SP_Y_MASK_W      0x01FF
#define TS_SP_TNUM_MASK_W   0x0FFF
#define TS_SP_PAL_MASK_W    0xF000

// TILES
#define TS_TL_XF          0x40
#define TS_TL_YF          0x80
#define TS_TL_PAL_MASK    0x30
#define TS_TL_PAL_BS      4

#define TS_TL_XF_W        0x4000
#define TS_TL_YF_W        0x8000

#define TS_TL_TNUM_MASK_W   0x0FFF
#define TS_TL_PAL_MASK_W    0x3000
#define TS_TL_PAL_BS_W      12
