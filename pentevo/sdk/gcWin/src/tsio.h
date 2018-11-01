#pragma once
__sfr __at 0xFE BORDER;

__sfr __banked __at 0x00AF TS_VCONFIG;
__sfr __banked __at 0x01AF TS_VPAGE;
__sfr __banked __at 0x0FAF TS_BORDER;
__sfr __banked __at 0x10AF TS_PAGE0;
__sfr __banked __at 0x11AF TS_PAGE1;
__sfr __banked __at 0x12AF TS_PAGE2;
__sfr __banked __at 0x13AF TS_PAGE3;

#define TS_VID_256X192      0x00#define TS_VID_320X200      0x40#define TS_VID_320X240      0x80#define TS_VID_360X288      0xC0

#define TS_VID_ZX           0x00#define TS_VID_16C          0x01#define TS_VID_256C         0x02#define TS_VID_TEXT         0x03#define TS_VID_FT812        0x04#define TS_VID_NOGFX        0x20

void strprn_xy(u16 *str, u8 x, u8 y, u8 color);
