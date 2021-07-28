/**
* This source code ("the Software") is provided by Bridgetek Pte Ltd
* ("Bridgetek") subject to the licence terms set out
*   http://brtchip.com/BRTSourceCodeLicenseAgreement/ ("the Licence Terms").
* You must read the Licence Terms before downloading or using the Software.
* By installing or using the Software you agree to the Licence Terms. If you
* do not agree to the Licence Terms then do not download or use the Software.
*
* Without prejudice to the Licence Terms, here is a summary of some of the key
* terms of the Licence Terms (and in the event of any conflict between this
* summary and the Licence Terms then the text of the Licence Terms will
* prevail).
*
* The Software is provided "as is".
* There are no warranties (or similar) in relation to the quality of the
* Software. You use it at your own risk.
* The Software should not be used in, or for, any medical device, system or
* appliance. There are exclusions of Bridgetek liability for certain types of loss
* such as: special loss or damage; incidental loss or damage; indirect or
* consequential loss or damage; loss of income; loss of business; loss of
* profits; loss of revenue; loss of contracts; business interruption; loss of
* the use of money or anticipated savings; loss of information; loss of
* opportunity; loss of goodwill or reputation; and/or loss of, damage to or
* corruption of data.
* There is a monetary cap on Bridgetek's liability.
* The Software may have subsequently been amended by another user and then
* distributed by that other user ("Adapted Software").  If so that user may
* have additional licence terms that apply to those amendments. However, Bridgetek
* has no liability in relation to those amendments.
*/

#ifndef EVE_GPU_TYPES__H
#define EVE_GPU_TYPES__H
#include "EVE_Config.h"

/***********
** MACROS **
***********/

#define EVE_GPU_NUMCHAR_PERFONT (128)
#define EVE_GPU_FONT_TABLE_SIZE (148)

/* SPI channel options */
typedef enum EVE_SPI_CHANNELS_T
{
	EVE_SPI_SINGLE_CHANNEL = 0x00,
	EVE_SPI_DUAL_CHANNEL = 0x01,
	EVE_SPI_QUAD_CHANNEL = 0x02,
} EVE_SPI_CHANNELS_T;

typedef enum EVE_SPI_DUMMY_BYTES_T
{
	EVE_SPI_ONE_DUMMY_BYTE = 0x00,
	EVE_SPI_TWO_DUMMY_BYTES = 0x04,
} EVE_SPI_DUMMY_BYTES_T;

/* APIs for Host Commands */
typedef enum EVE_PLL_SOURCE_T
{
	EVE_INTERNAL_OSC = 0x48, // default
	EVE_EXTERNAL_OSC = 0x44,
} EVE_PLL_SOURCE_T;

typedef enum EVE_PLL_FREQ_T
{
	EVE_PLL_48M = 0x62, // default
	EVE_PLL_36M = 0x61,
	EVE_PLL_24M = 0x64,
} EVE_PLL_FREQ_T;

typedef enum EVE_POWER_MODE_T
{
	EVE_ACTIVE_M = 0x00,
	EVE_STANDBY_M = 0x41, // default
	EVE_SLEEP_M = 0x42,
	EVE_POWERDOWN_M = 0x50,
} EVE_POWER_MODE_T;

typedef enum EVE_81X_PLL_FREQ_T
{
	EVE_SYSCLK_DEFAULT = 0x00, // default 60mhz on ft810, 48mhz on ft800
#if (EVE_SUPPORT_CHIPID >= EVE_BT815)
	EVE_SYSCLK_84M = 0x07,
#endif
#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
	EVE_SYSCLK_72M = 0x06,
	EVE_SYSCLK_60M = 0x05,
#endif
	EVE_SYSCLK_48M = 0x04,
	EVE_SYSCLK_36M = 0x03,
	EVE_SYSCLK_24M = 0x02,
} EVE_81X_PLL_FREQ_T;

#if (EVE_SUPPORT_CHIPID >= EVE_FT810)


typedef enum EVE_81X_GPIO_DRIVE_STRENGTH_T
{
	EVE_5MA = 0x00, //default current
	EVE_10MA = 0x01,
	EVE_15MA = 0x02,
	EVE_20MA = 0x03,
} EVE_81X_GPIO_DRIVE_STRENGTH_T;

typedef enum EVE_81X_GPIO_GROUP_T
{
	EVE_GPIO0 = 0x00,
	EVE_GPIO1 = 0x04,
	EVE_GPIO2 = 0x08,
	EVE_GPIO3 = 0x0C,
	EVE_GPIO4 = 0x10,
	EVE_DISP = 0x20,
	EVE_DE = 0x24,
	EVE_VSYNC_HSYNC = 0x28,
	EVE_PCLK = 0x2C,
	EVE_BACKLIGHT = 0x30,
	EVE_R_G_B = 0x34,
	EVE_AUDIO_L = 0x38,
	EVE_INT_N = 0x3C,
	EVE_TOUCHWAKE = 0x40,
	EVE_SCL = 0x44,
	EVE_SDA = 0x48,
	EVE_SPI_MISO_MOSI_IO2_IO3 = 0x4C,
} EVE_81X_GPIO_GROUP_T;

#define EVE_81X_RESET_ACTIVE 0x000268
#define EVE_81X_RESET_REMOVAL 0x002068
#endif

#define EVE_CORE_RESET (0x68)

/* FT81x and FT80x font table structure */
/* Font table address in ROM can be found by reading the address from 0xFFFFC location. */
/* 16 font tables are present at the address read from location 0xFFFFC */
typedef struct EVE_Gpu_Fonts
{
	/* All the values are in bytes */
	/* Width of each character font from 0 to 127 */
	uint8_t FontWidth[EVE_GPU_NUMCHAR_PERFONT];
	/* Bitmap format of font wrt bitmap formats supported by FT800 - L1, L4, L8 */
	uint32_t FontBitmapFormat;
	/* Font line stride in FT800 ROM */
	uint32_t FontLineStride;
	/* Font width in pixels */
	uint32_t FontWidthInPixels;
	/* Font height in pixels */
	uint32_t FontHeightInPixels;
	/* Pointer to font graphics raw data */
	uint32_t PointerToFontGraphicsData;
} EVE_Gpu_Fonts;

#if defined(EVE_SUPPORT_UNICODE)

/* BT815 inclusion - extended font table for unicode support */
typedef struct EVE_Gpu_FontsExt
{
	uint32_t Signature;
	uint32_t Size;
	uint32_t Format;
	uint32_t Swizzle;
	uint32_t LayoutWidth;
	uint32_t LayoutHeight;
	uint32_t PixelWidth;
	uint32_t PixelHeight;
	uint32_t StartOfGraphicData;
	uint32_t OffsetGlyphData;
	// uint32_t OffsetWidthData; // to be OffsetWidthData = Starting adress + StartOfGraphicData * 4.
} EVE_Gpu_FontsExt;

#endif

#endif /* #ifndef EVE_GPU_TYPES__H */

/* end of file */