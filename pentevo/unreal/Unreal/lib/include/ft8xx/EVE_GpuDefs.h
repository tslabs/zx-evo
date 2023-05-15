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

/*

Defines EVE hardware values.

This header is separated and included last 
in case of conflicts with other libraries.

Expects BT_81XA_ENABLE, BT_81X_ENABLE, FT_81X_ENABLE, or FT_80X_ENABLE
to be defined. If not, multi target compilation is assumed.

*/

#ifndef EVE_GPU_DEFS__H
#define EVE_GPU_DEFS__H

#ifndef ESD_ENUM
#define ESD_ENUM(name, ...)
#define ESD_END()
#endif

#if !defined(EVE_MULTI_TARGET) \
    && !defined(FT_80X_ENABLE) \
    && !defined(FT_81X_ENABLE) \
    && !defined(BT_81X_ENABLE) \
    && !defined(BT_81XA_ENABLE)
#define EVE_MULTI_TARGET
#endif

/* Definitions used for FT800 coprocessor command buffer */
#define EVE_DL_SIZE (8 * 1024UL) /* 8kB Display List buffer size */
#define EVE_DL_COUNT (2 * 1024UL)
#define EVE_CMD_FIFO_SIZE ((4) * 1024UL) /* 4kB coprocessor FIFO size */
#define EVE_CMD_FIFO_COUNT (1024UL)
#define EVE_CMD_FIFO_MASK (EVE_CMD_FIFO_SIZE - 1)
#define EVE_CMD_FIFO_ALIGNMENT_MASK (EVE_CMD_FIFO_SIZE - ((4) - 1))

#define EVE_CMD_FAULT(rp) (rp & 0x3)

/**************
** Addresses **
**************/

#define RAM_G 0UL
#define ROM_CHIPID 786432UL
#define RAM_ERR_REPORT 0x309800UL

#if defined(EVE_MULTI_TARGET)

#define EVE_HAL_REG_ID (phost->GpuDefs->RegId)
#define EVE_HAL_REG_CPURESET (phost->GpuDefs->RegCpuReset)
#define EVE_HAL_REG_J1_INT (phost->GpuDefs->RegJ1Int)
#define EVE_HAL_REG_CMD_READ (phost->GpuDefs->RegCmdRead)
#define EVE_HAL_REG_TOUCH_TRANSFORM_A (phost->GpuDefs->RegTouchTransformA)
#define EVE_HAL_REG_CRC (phost->GpuDefs->RegCrc)
#define EVE_HAL_REG_TRIM (phost->GpuDefs->RegTrim)
#define EVE_HAL_REG_TOUCH_DIRECT_XY (phost->GpuDefs->RegTouchDirectXY)
#define EVE_HAL_REG_DATESTAMP (phost->GpuDefs->RegDatestamp)
#define EVE_HAL_REG_CMDB_SPACE (phost->GpuDefs->RegCmdBSpace)
#define EVE_HAL_REG_TRACKER (phost->GpuDefs->RegTracker)

#define RAM_DL (phost->GpuDefs->RamDl)
#define ROMFONT_TABLEADDRESS (phost->GpuDefs->RomFontTableAddress)

#define RAM_G_SIZE (phost->GpuDefs->RamGSize)
#define LOW_FREQ_BOUND (phost->GpuDefs->LowFreqBound)

#define BITMAP_ADDR_MASK (phost->GpuDefs->BitmapAddrMask)
#define SCISSOR_XY_SHIFT (phost->GpuDefs->ScissorSizeShift - 1)
#define SCISSOR_XY_MASK (((1UL << phost->GpuDefs->ScissorSizeShift) - 1UL) >> 1)
#define SCISSOR_SIZE_SHIFT (phost->GpuDefs->ScissorSizeShift)
#define SCISSOR_SIZE_MASK ((1UL << phost->GpuDefs->ScissorSizeShift) - 1UL)

#else

#if defined(FT_81X_ENABLE) || defined(BT_81X_ENABLE) || defined(BT_81XA_ENABLE)

#define EVE_HAL_REG_ID 3153920UL
#define EVE_HAL_REG_CPURESET 3153952UL
#define EVE_HAL_REG_J1_INT 3154084UL
#define EVE_HAL_REG_CMD_READ 3154168UL
#define EVE_HAL_REG_TOUCH_TRANSFORM_A 3154256UL
#define EVE_HAL_REG_CRC 3154296UL
#define EVE_HAL_REG_TRIM 3154304UL
#define EVE_HAL_REG_TOUCH_DIRECT_XY 3154316UL
#define EVE_HAL_REG_DATESTAMP 3155300UL
#define EVE_HAL_REG_CMDB_SPACE 3155316UL
#define EVE_HAL_REG_TRACKER 3182592UL

#define RAM_DL 3145728UL
#define ROMFONT_TABLEADDRESS 3145724UL

#define RAM_G_SIZE (1024 * 1024L)
#define LOW_FREQ_BOUND 58800000L //98% of 60Mhz

#define SCISSOR_XY_SHIFT 11
#define SCISSOR_XY_MASK 2047UL
#define SCISSOR_SIZE_SHIFT 12
#define SCISSOR_SIZE_MASK 4095UL

#endif

#if defined(BT_81X_ENABLE) || defined(BT_81XA_ENABLE)

#define BITMAP_ADDR_MASK 16777215UL

#elif defined(FT_81X_ENABLE)

#define BITMAP_ADDR_MASK 4194303UL

#elif defined(FT_80X_ENABLE)

#define EVE_HAL_REG_ID 1057792UL
#define EVE_HAL_REG_CPURESET 1057820UL
#define EVE_HAL_REG_J1_INT 1057940UL
#define EVE_HAL_REG_CMD_READ 1058020UL
#define EVE_HAL_REG_TOUCH_TRANSFORM_A 1058076UL
#define EVE_HAL_REG_CRC 1058152UL
#define EVE_HAL_REG_TRIM 1058156UL
#define EVE_HAL_REG_TOUCH_DIRECT_XY 1058164UL
#define EVE_HAL_REG_DATESTAMP 1058108UL
#define EVE_HAL_REG_CMDB_SPACE 0
#define EVE_HAL_REG_TRACKER 1085440UL

#define RAM_DL 1048576UL
#define ROMFONT_TABLEADDRESS 1048572UL

#define RAM_G_SIZE (256 * 1024L)
#define LOW_FREQ_BOUND 47040000L // 98% of 48Mhz

#define BITMAP_ADDR_MASK 1048575UL
#define SCISSOR_XY_SHIFT 9
#define SCISSOR_XY_MASK 511UL
#define SCISSOR_SIZE_SHIFT 10
#define SCISSOR_SIZE_MASK 1023UL

#endif

#endif

#define RAM_PAL (RAM_DL + 8192UL)
#define RAM_TOP (RAM_DL + 16384UL)
#define RAM_CMD (RAM_DL + 32768UL)
#define RAM_J1RAM (RAM_DL + 36864UL)
#define RAM_ROMSUB (RAM_DL + 40960UL)
#define RAM_JTBOOT (RAM_DL + 45056UL)
#define RAM_BIST (RAM_DL + 524288UL)
#define RAM_COMPOSITE (RAM_DL + 794624UL)

#define RAM_REG (EVE_HAL_REG_ID + 0UL)
#define REG_ID (EVE_HAL_REG_ID + 0UL)
#define REG_FRAMES (REG_ID + 4UL)
#define REG_CLOCK (REG_ID + 8UL)
#define REG_FREQUENCY (REG_ID + 12UL)
#define REG_RENDERMODE (REG_ID + 16UL)
#define REG_SNAPY (REG_ID + 20UL)
#define REG_SNAPSHOT (REG_ID + 24UL)
#define REG_SNAPFORMAT (REG_ID + 28UL)

#define REG_CPURESET (EVE_HAL_REG_CPURESET + 0UL)
#define REG_TAP_CRC (REG_CPURESET + 4UL)
#define REG_TAP_MASK (REG_CPURESET + 8UL)
#define REG_HCYCLE (REG_CPURESET + 12UL)
#define REG_HOFFSET (REG_CPURESET + 16UL)
#define REG_HSIZE (REG_CPURESET + 20UL)
#define REG_HSYNC0 (REG_CPURESET + 24UL)
#define REG_HSYNC1 (REG_CPURESET + 28UL)
#define REG_VCYCLE (REG_CPURESET + 32UL)
#define REG_VOFFSET (REG_CPURESET + 36UL)
#define REG_VSIZE (REG_CPURESET + 40UL)
#define REG_VSYNC0 (REG_CPURESET + 44UL)
#define REG_VSYNC1 (REG_CPURESET + 48UL)
#define REG_DLSWAP (REG_CPURESET + 52UL)
#define REG_ROTATE (REG_CPURESET + 56UL)
#define REG_OUTBITS (REG_CPURESET + 60UL)
#define REG_DITHER (REG_CPURESET + 64UL)
#define REG_SWIZZLE (REG_CPURESET + 68UL)
#define REG_CSPREAD (REG_CPURESET + 72UL)
#define REG_PCLK_POL (REG_CPURESET + 76UL)
#define REG_PCLK (REG_CPURESET + 80UL)
#define REG_TAG_X (REG_CPURESET + 84UL)
#define REG_TAG_Y (REG_CPURESET + 88UL)
#define REG_TAG (REG_CPURESET + 92UL)
#define REG_VOL_PB (REG_CPURESET + 96UL)
#define REG_VOL_SOUND (REG_CPURESET + 100UL)
#define REG_SOUND (REG_CPURESET + 104UL)
#define REG_PLAY (REG_CPURESET + 108UL)
#define REG_GPIO_DIR (REG_CPURESET + 112UL)
#define REG_GPIO (REG_CPURESET + 116UL)
#define REG_GPIOX_DIR (REG_CPURESET + 120UL)
#define REG_GPIOX (REG_CPURESET + 124UL)
#define REG_J1_COLD (REG_CPURESET + 128UL)

#define REG_J1_INT (EVE_HAL_REG_J1_INT + 0UL)
#define REG_INT_FLAGS (REG_J1_INT + 4UL)
#define REG_INT_EN (REG_J1_INT + 8UL)
#define REG_INT_MASK (REG_J1_INT + 12UL)
#define REG_PLAYBACK_START (REG_J1_INT + 16UL)
#define REG_PLAYBACK_LENGTH (REG_J1_INT + 20UL)
#define REG_PLAYBACK_READPTR (REG_J1_INT + 24UL)
#define REG_PLAYBACK_FREQ (REG_J1_INT + 28UL)
#define REG_PLAYBACK_FORMAT (REG_J1_INT + 32UL)
#define REG_PLAYBACK_LOOP (REG_J1_INT + 36UL)
#define REG_PLAYBACK_PLAY (REG_J1_INT + 40UL)
#define REG_PWM_HZ (REG_J1_INT + 44UL)
#define REG_PWM_DUTY (REG_J1_INT + 48UL)
#define REG_MACRO_0 (REG_J1_INT + 52UL)
#define REG_MACRO_1 (REG_J1_INT + 56UL)
#define REG_CYA0 (REG_J1_INT + 60UL)
#define REG_CYA1 (REG_J1_INT + 64UL)
#define REG_BUSYBITS (REG_J1_INT + 68UL)
#define REG_ROMSUB_SEL (REG_J1_INT + 76UL)
#define REG_RAM_FOLD (REG_J1_INT + 80UL)

#define REG_CMD_READ (EVE_HAL_REG_CMD_READ + 0UL)
#define REG_CMD_WRITE (REG_CMD_READ + 4UL)
#define REG_CMD_DL (REG_CMD_READ + 8UL)
#define REG_TOUCH_MODE (REG_CMD_READ + 12UL)
#define REG_CTOUCH_EXTENDED (REG_CMD_READ + 16UL)
#define REG_TOUCH_ADC_MODE (REG_CMD_READ + 16UL)
#define REG_EHOST_TOUCH_X (REG_CMD_READ + 20UL)
#define REG_TOUCH_CHARGE (REG_CMD_READ + 20UL)
#define REG_TOUCH_SETTLE (REG_CMD_READ + 24UL)
#define REG_EHOST_TOUCH_ID (REG_CMD_READ + 28UL)
#define REG_TOUCH_OVERSAMPLE (REG_CMD_READ + 28UL)
#define REG_EHOST_TOUCH_Y (REG_CMD_READ + 32UL)
#define REG_TOUCH_RZTHRESH (REG_CMD_READ + 32UL)
#define REG_CTOUCH_TOUCH1_XY (REG_CMD_READ + 36UL)
#define REG_TOUCH_RAW_XY (REG_CMD_READ + 36UL)
#define REG_CTOUCH_IDS (REG_CMD_READ + 40UL)
#define REG_CTOUCH_TOUCH4_Y (REG_CMD_READ + 40UL)
#define REG_TOUCH_RZ (REG_CMD_READ + 40UL)
#define REG_CTOUCH_TOUCH0_XY (REG_CMD_READ + 44UL)
#define REG_TOUCH_SCREEN_XY (REG_CMD_READ + 44UL)
#define REG_TOUCH_TAG_XY (REG_CMD_READ + 48UL)
#define REG_TOUCH_TAG (REG_CMD_READ + 52UL)
#define REG_TOUCH_TAG1_XY (REG_CMD_READ + 56UL)
#define REG_TOUCH_TAG1 (REG_CMD_READ + 60UL)
#define REG_TOUCH_TAG2_XY (REG_CMD_READ + 64UL)
#define REG_TOUCH_TAG2 (REG_CMD_READ + 68UL)
#define REG_TOUCH_TAG3_XY (REG_CMD_READ + 72UL)
#define REG_TOUCH_TAG3 (REG_CMD_READ + 76UL)
#define REG_TOUCH_TAG4_XY (REG_CMD_READ + 80UL)
#define REG_TOUCH_TAG4 (REG_CMD_READ + 84UL)

#define REG_TOUCH_TRANSFORM_A (EVE_HAL_REG_TOUCH_TRANSFORM_A + 0UL)
#define REG_TOUCH_TRANSFORM_B (REG_TOUCH_TRANSFORM_A + 4UL)
#define REG_TOUCH_TRANSFORM_C (REG_TOUCH_TRANSFORM_A + 8UL)
#define REG_TOUCH_TRANSFORM_D (REG_TOUCH_TRANSFORM_A + 12UL)
#define REG_TOUCH_TRANSFORM_E (REG_TOUCH_TRANSFORM_A + 16UL)
#define REG_TOUCH_TRANSFORM_F (REG_TOUCH_TRANSFORM_A + 20UL)
#define REG_TOUCH_CONFIG (REG_TOUCH_TRANSFORM_A + 24UL)
#define REG_ANALOG (REG_TOUCH_TRANSFORM_A + 28UL)
#define REG_CTOUCH_GESTURE (REG_TOUCH_TRANSFORM_A + 28UL)
#define REG_CTOUCH_TOUCH4_X (REG_TOUCH_TRANSFORM_A + 28UL)
#define REG_PATCHED_TOUCH_FAULT (REG_TOUCH_TRANSFORM_A + 28UL)
#define REG_EHOST_TOUCH_ACK (REG_TOUCH_TRANSFORM_A + 32UL)
#define REG_PATCHED_ANALOG (REG_TOUCH_TRANSFORM_A + 32UL)
#define REG_TOUCH_FAULT (REG_TOUCH_TRANSFORM_A + 32UL)
#define REG_BIST_EN (REG_TOUCH_TRANSFORM_A + 36UL)

#define REG_CRC (EVE_HAL_REG_CRC + 0UL)
#define REG_SPI_EARLY_TX (REG_CRC + 4UL)

#define REG_TRIM (EVE_HAL_REG_TRIM + 0UL)
#define REG_ANA_COMP (REG_TRIM + 4UL)
#define REG_SPI_WIDTH (REG_TRIM + 8UL)

#define REG_CTOUCH_TOUCH2_XY (EVE_HAL_REG_TOUCH_DIRECT_XY + 0UL)
#define REG_TOUCH_DIRECT_XY (REG_CTOUCH_TOUCH2_XY + 0UL)
#define REG_CTOUCH_TOUCH3_XY (REG_CTOUCH_TOUCH2_XY + 4UL)
#define REG_TOUCH_DIRECT_Z1Z2 (REG_CTOUCH_TOUCH2_XY + 4UL)
#define REG_EJPG_READY (REG_CTOUCH_TOUCH2_XY + 8UL)
#define REG_EJPG_BUSY (REG_CTOUCH_TOUCH2_XY + 12UL)
#define REG_EJPG_DAT (REG_CTOUCH_TOUCH2_XY + 16UL)
#define REG_EJPG_OPTIONS (REG_CTOUCH_TOUCH2_XY + 20UL)
#define REG_EJPG_DST (REG_CTOUCH_TOUCH2_XY + 24UL)
#define REG_EJPG_W (REG_CTOUCH_TOUCH2_XY + 28UL)
#define REG_EJPG_H (REG_CTOUCH_TOUCH2_XY + 32UL)
#define REG_EJPG_FORMAT (REG_CTOUCH_TOUCH2_XY + 36UL)
#define REG_EJPG_RI (REG_CTOUCH_TOUCH2_XY + 40UL)
#define REG_EJPG_TQ (REG_CTOUCH_TOUCH2_XY + 44UL)
#define REG_EJPG_TDA (REG_CTOUCH_TOUCH2_XY + 48UL)
#define REG_EJPG_Q (REG_CTOUCH_TOUCH2_XY + 52UL)
#define REG_EJPG_HT (REG_CTOUCH_TOUCH2_XY + 180UL)
#define REG_EJPG_DCC (REG_CTOUCH_TOUCH2_XY + 436UL)
#define REG_EJPG_ACC (REG_CTOUCH_TOUCH2_XY + 460UL)
#define REG_EJPG_SCALE (REG_CTOUCH_TOUCH2_XY + 972UL)
#define REG_EJPG_DEBUG (REG_CTOUCH_TOUCH2_XY + 976UL)
#define REG_RASTERY (REG_CTOUCH_TOUCH2_XY + 980UL)

#define REG_DATESTAMP (EVE_HAL_REG_DATESTAMP + 0UL)

#define REG_CMDB_SPACE (EVE_HAL_REG_CMDB_SPACE + 0UL)
#define REG_CMDB_WRITE (REG_CMDB_SPACE + 4UL)
#define REG_ADAPTIVE_FRAMERATE (REG_CMDB_SPACE + 8UL)
#define REG_SPIM_DIR (REG_CMDB_SPACE + 12UL)
#define REG_SPIM (REG_CMDB_SPACE + 16UL)
#define REG_ESPIM_READSTART (REG_CMDB_SPACE + 20UL)
#define REG_ESPIM_SEQ (REG_CMDB_SPACE + 24UL)
#define REG_ESPIM_ADD (REG_CMDB_SPACE + 40UL)
#define REG_ESPIM_COUNT (REG_CMDB_SPACE + 44UL)
#define REG_ESPIM_WINDOW (REG_CMDB_SPACE + 48UL)
#define REG_ESPIM_DUMMY (REG_CMDB_SPACE + 112UL)
#define REG_ESPIM_TRIG (REG_CMDB_SPACE + 116UL)
#define REG_PLAYBACK_PAUSE (REG_CMDB_SPACE + 120UL)
#define REG_FLASH_STATUS (REG_CMDB_SPACE + 124UL)
#define REG_FULLBUSYBITS (REG_CMDB_SPACE + 128UL)
#define REG_SHA1KEY (REG_CMDB_SPACE + 144UL)

#if defined(BT_81XA_ENABLE) || defined(EVE_MULTI_TARGET)
#define REG_UNDERRUN (REG_CMDB_SPACE + 152UL)
#define REG_AH_HCYCLE_MAX (REG_CMDB_SPACE + 156UL)
#define REG_PCLK_FREQ (REG_CMDB_SPACE + 160UL)
#define REG_PCLK_2X (REG_CMDB_SPACE + 164UL)
#define REG_FLASH_DTR (REG_CMDB_SPACE + 168UL)
#define REG_ESPIM_DTR (REG_CMDB_SPACE + 172UL)
#define REG_HSF_HSIZE (REG_CMDB_SPACE + 176UL)
#define REG_HSF_FT1 (REG_CMDB_SPACE + 180UL)
#define REG_HSF_FSCALE (REG_CMDB_SPACE + 184UL)
#define REG_HSF_F00 (REG_CMDB_SPACE + 188UL)
#define REG_HSF_F02 (REG_CMDB_SPACE + 192UL)
#define REG_HSF_F03 (REG_CMDB_SPACE + 196UL)
#define REG_HSF_F10 (REG_CMDB_SPACE + 200UL)
#define REG_HSF_F11 (REG_CMDB_SPACE + 204UL)
#define REG_HSF_F12 (REG_CMDB_SPACE + 208UL)
#define REG_HSF_F13 (REG_CMDB_SPACE + 212UL)
#endif

#define REG_TRACKER (EVE_HAL_REG_TRACKER + 0UL)
#define REG_TRACKER_1 (REG_TRACKER + 4UL)
#define REG_TRACKER_2 (REG_TRACKER + 8UL)
#define REG_TRACKER_3 (REG_TRACKER + 12UL)
#define REG_TRACKER_4 (REG_TRACKER + 16UL)
#define REG_MEDIAFIFO_READ (REG_TRACKER + 20UL)
#define REG_MEDIAFIFO_WRITE (REG_TRACKER + 24UL)
#define REG_MEDIAFIFO_BASE (REG_TRACKER + 28UL)
#define REG_MEDIAFIFO_SIZE (REG_TRACKER + 32UL)
#define REG_FLASH_SIZE (REG_TRACKER + 36UL)
#define REG_ANIM_ACTIVE (REG_TRACKER + 44UL)
#define REG_DF_TUNED (REG_TRACKER + 48UL)
#define REG_PLAY_CONTROL (REG_TRACKER + 334UL)

#define REG_CTOUCH_TOUCHA_XY REG_CTOUCH_TOUCH1_XY
#define REG_CTOUCH_TOUCHB_XY REG_CTOUCH_TOUCH2_XY
#define REG_CTOUCH_TOUCHC_XY REG_CTOUCH_TOUCH3_XY
#define REG_CTOUCH_TOUCHD_X REG_CTOUCH_TOUCH4_X
#define REG_CTOUCH_TOUCHD_Y REG_CTOUCH_TOUCH4_Y

#define EVE_GPUDEFS_IMPLEMENT REG_ID,                \
	                          REG_CPURESET,          \
	                          REG_J1_INT,            \
	                          REG_CMD_READ,          \
	                          REG_TOUCH_TRANSFORM_A, \
	                          REG_CRC,               \
	                          REG_TRIM,              \
	                          REG_TOUCH_DIRECT_XY,   \
	                          REG_DATESTAMP,         \
	                          REG_CMDB_SPACE,        \
	                          REG_TRACKER,           \
	                          RAM_DL,                \
	                          ROMFONT_TABLEADDRESS,  \
	                          RAM_G_SIZE,            \
	                          LOW_FREQ_BOUND,        \
	                          BITMAP_ADDR_MASK,      \
	                          SCISSOR_SIZE_SHIFT

/*************
** Commands **
*************/

// clang-format off

// FT800
#define CMD_DLSTART          4294967040UL
#define CMD_SWAP             4294967041UL
#define CMD_INTERRUPT        4294967042UL
#define CMD_BGCOLOR          4294967049UL
#define CMD_FGCOLOR          4294967050UL
#define CMD_GRADIENT         4294967051UL
#define CMD_TEXT             4294967052UL
#define CMD_BUTTON           4294967053UL
#define CMD_KEYS             4294967054UL
#define CMD_PROGRESS         4294967055UL
#define CMD_SLIDER           4294967056UL
#define CMD_SCROLLBAR        4294967057UL
#define CMD_TOGGLE           4294967058UL
#define CMD_GAUGE            4294967059UL
#define CMD_CLOCK            4294967060UL
#define CMD_CALIBRATE        4294967061UL
#define CMD_SPINNER          4294967062UL
#define CMD_STOP             4294967063UL
#define CMD_MEMCRC           4294967064UL
#define CMD_REGREAD          4294967065UL
#define CMD_MEMWRITE         4294967066UL
#define CMD_MEMSET           4294967067UL
#define CMD_MEMZERO          4294967068UL
#define CMD_MEMCPY           4294967069UL
#define CMD_APPEND           4294967070UL
#define CMD_SNAPSHOT         4294967071UL
#define CMD_BITMAP_TRANSFORM 4294967073UL
#define CMD_INFLATE          4294967074UL
#define CMD_GETPTR           4294967075UL
#define CMD_LOADIMAGE        4294967076UL
#define CMD_GETPROPS         4294967077UL
#define CMD_LOADIDENTITY     4294967078UL
#define CMD_TRANSLATE        4294967079UL
#define CMD_SCALE            4294967080UL
#define CMD_ROTATE           4294967081UL
#define CMD_SETMATRIX        4294967082UL
#define CMD_SETFONT          4294967083UL
#define CMD_TRACK            4294967084UL
#define CMD_DIAL             4294967085UL
#define CMD_NUMBER           4294967086UL
#define CMD_SCREENSAVER      4294967087UL
#define CMD_SKETCH           4294967088UL
#define CMD_LOGO             4294967089UL
#define CMD_COLDSTART        4294967090UL
#define CMD_GETMATRIX        4294967091UL
#define CMD_GRADCOLOR        4294967092UL

// FT801
#if defined(FT_80X_ENABLE) || defined(EVE_MULTI_TARGET)
#define CMD_CSKETCH          4294967093UL
#endif

// FT810
#if defined(FT_81X_ENABLE) || defined(BT_81X_ENABLE) || defined(BT_81XA_ENABLE) || defined(EVE_MULTI_TARGET)
#define CMD_SETROTATE        4294967094UL
#define CMD_SNAPSHOT2        4294967095UL
#define CMD_SETBASE          4294967096UL
#define CMD_MEDIAFIFO        4294967097UL
#define CMD_PLAYVIDEO        4294967098UL
#define CMD_SETFONT2         4294967099UL
#define CMD_SETSCRATCH       4294967100UL
#define CMD_ROMFONT          4294967103UL
#define CMD_VIDEOSTART       4294967104UL
#define CMD_VIDEOFRAME       4294967105UL
#define CMD_SYNC             4294967106UL
#define CMD_SETBITMAP        4294967107UL
#endif

// BT815
#if defined(BT_81X_ENABLE) || defined(BT_81XA_ENABLE) || defined(EVE_MULTI_TARGET)
#define CMD_FLASHERASE       4294967108UL
#define CMD_FLASHWRITE       4294967109UL
#define CMD_FLASHREAD        4294967110UL
#define CMD_FLASHUPDATE      4294967111UL
#define CMD_FLASHDETACH      4294967112UL
#define CMD_FLASHATTACH      4294967113UL
#define CMD_FLASHFAST        4294967114UL
#define CMD_FLASHSPIDESEL    4294967115UL
#define CMD_FLASHSPITX       4294967116UL
#define CMD_FLASHSPIRX       4294967117UL
#define CMD_FLASHSOURCE      4294967118UL
#define CMD_CLEARCACHE       4294967119UL
#define CMD_INFLATE2         4294967120UL
#define CMD_ROTATEAROUND     4294967121UL
#define CMD_RESETFONTS       4294967122UL
#define CMD_ANIMSTART        4294967123UL
#define CMD_ANIMSTOP         4294967124UL
#define CMD_ANIMXY           4294967125UL
#define CMD_ANIMDRAW         4294967126UL
#define CMD_GRADIENTA        4294967127UL
#define CMD_FILLWIDTH        4294967128UL
#define CMD_APPENDF          4294967129UL
#define CMD_ANIMFRAME        4294967130UL
#define CMD_NOP              4294967131UL
#define CMD_VIDEOSTARTF      4294967135UL
#endif

// BT817
#if defined(BT_81XA_ENABLE) || defined(EVE_MULTI_TARGET)
#define CMD_CALIBRATESUB     4294967136UL
#define CMD_TESTCARD         4294967137UL
#define CMD_HSF              4294967138UL
#define CMD_APILEVEL         4294967139UL
#define CMD_GETIMAGE         4294967140UL
#define CMD_WAIT             4294967141UL
#define CMD_RETURN           4294967142UL
#define CMD_CALLLIST         4294967143UL
#define CMD_NEWLIST          4294967144UL
#define CMD_ENDLIST          4294967145UL
#define CMD_PCLKFREQ         4294967146UL
#define CMD_FONTCACHE        4294967147UL
#define CMD_FONTCACHEQUERY   4294967148UL
#define CMD_ANIMFRAMERAM     4294967149UL
#define CMD_ANIMSTARTRAM     4294967150UL
#define CMD_RUNANIM          4294967151UL
#define CMD_FLASHPROGRAM     4294967152UL
#endif

// clang-format on

/*****************
** Display List **
*****************/

#define VERTEX2F(x, y) ((1UL << 30) | (((x)&32767UL) << 15) | (((y)&32767UL) << 0))
#define VERTEX2II(x, y, handle, cell) ((2UL << 30) | (((x)&511UL) << 21) | (((y)&511UL) << 12) | (((handle)&31UL) << 7) | (((cell)&127UL) << 0))
#define BITMAP_SOURCE(addr) ((1UL << 24) | ((addr) < 0 ? (((addr) & (BITMAP_ADDR_MASK >> 1))) : ((addr)&BITMAP_ADDR_MASK)))
#define BITMAP_SOURCE2(flash_or_ram, addr) ((1UL << 24) | ((flash_or_ram) << 23) | (((addr) & (BITMAP_ADDR_MASK >> 1)) << 0))
#define CLEAR_COLOR_RGB(red, green, blue) ((2UL << 24) | (((red)&255UL) << 16) | (((green)&255UL) << 8) | (((blue)&255UL) << 0))
#define TAG(s) ((3UL << 24) | (((s)&255UL) << 0))
#define COLOR_RGB(red, green, blue) ((4UL << 24) | (((red)&255UL) << 16) | (((green)&255UL) << 8) | (((blue)&255UL) << 0))
#define BITMAP_HANDLE(handle) ((5UL << 24) | (((handle)&31UL) << 0))
#define CELL(cell) ((6UL << 24) | (((cell)&127UL) << 0))
#define BITMAP_LAYOUT(format, linestride, height) ((7UL << 24) | (((format)&31UL) << 19) | (((linestride)&1023UL) << 9) | (((height)&511UL) << 0))
#define BITMAP_SIZE(filter, wrapx, wrapy, width, height) ((8UL << 24) | (((filter)&1UL) << 20) | (((wrapx)&1UL) << 19) | (((wrapy)&1UL) << 18) | (((width)&511UL) << 9) | (((height)&511UL) << 0))
#define ALPHA_FUNC(func, ref) ((9UL << 24) | (((func)&7UL) << 8) | (((ref)&255UL) << 0))
#define STENCIL_FUNC(func, ref, mask) ((10UL << 24) | (((func)&7UL) << 16) | (((ref)&255UL) << 8) | (((mask)&255UL) << 0))
#define BLEND_FUNC(src, dst) ((11UL << 24) | (((src)&7UL) << 3) | (((dst)&7UL) << 0))
#define STENCIL_OP(sfail, spass) ((12UL << 24) | (((sfail)&7UL) << 3) | (((spass)&7UL) << 0))
#define POINT_SIZE(size) ((13UL << 24) | (((size)&8191UL) << 0))
#define LINE_WIDTH(width) ((14UL << 24) | (((width)&4095UL) << 0))
#define CLEAR_COLOR_A(alpha) ((15UL << 24) | (((alpha)&255UL) << 0))
#define COLOR_A(alpha) ((16UL << 24) | (((alpha)&255UL) << 0))
#define CLEAR_STENCIL(s) ((17UL << 24) | (((s)&255UL) << 0))
#define CLEAR_TAG(s) ((18UL << 24) | (((s)&255UL) << 0))
#define STENCIL_MASK(mask) ((19UL << 24) | (((mask)&255UL) << 0))
#define TAG_MASK(mask) ((20UL << 24) | (((mask)&1UL) << 0))
#define BITMAP_TRANSFORM_C(c) ((23UL << 24) | (((c)&16777215UL) << 0))
#define BITMAP_TRANSFORM_F(f) ((26UL << 24) | (((f)&16777215UL) << 0))
#define BITMAP_TRANSFORM_A_EXT(p, v) ((21UL << 24) | (((p)&1UL) << 17) | (((v)&131071UL) << 0))
#define BITMAP_TRANSFORM_B_EXT(p, v) ((22UL << 24) | (((p)&1UL) << 17) | (((v)&131071UL) << 0))
#define BITMAP_TRANSFORM_D_EXT(p, v) ((24UL << 24) | (((p)&1UL) << 17) | (((v)&131071UL) << 0))
#define BITMAP_TRANSFORM_E_EXT(p, v) ((25UL << 24) | (((p)&1UL) << 17) | (((v)&131071UL) << 0))
#define BITMAP_TRANSFORM_A(a) BITMAP_TRANSFORM_A_EXT(0, a)
#define BITMAP_TRANSFORM_B(b) BITMAP_TRANSFORM_B_EXT(0, b)
#define BITMAP_TRANSFORM_D(d) BITMAP_TRANSFORM_D_EXT(0, d)
#define BITMAP_TRANSFORM_E(e) BITMAP_TRANSFORM_E_EXT(0, e)
#define SCISSOR_XY(x, y) ((27UL << 24) | (((x)&SCISSOR_XY_MASK) << SCISSOR_XY_SHIFT) | (((y)&SCISSOR_XY_MASK) << 0))
#define SCISSOR_SIZE(width, height) ((28UL << 24) | (((width)&SCISSOR_SIZE_MASK) << SCISSOR_SIZE_SHIFT) | (((height)&SCISSOR_SIZE_MASK) << 0))
#define CALL(dest) ((29UL << 24) | (((dest)&65535UL) << 0))
#define JUMP(dest) ((30UL << 24) | (((dest)&65535UL) << 0))
#define BEGIN(prim) ((31UL << 24) | (((prim)&15UL) << 0))
#define COLOR_MASK(r, g, b, a) ((32UL << 24) | (((r)&1UL) << 3) | (((g)&1UL) << 2) | (((b)&1UL) << 1) | (((a)&1UL) << 0))
#define CLEAR(c, s, t) ((38UL << 24) | (((c)&1UL) << 2) | (((s)&1UL) << 1) | (((t)&1UL) << 0))
#define VERTEX_FORMAT(frac) ((39UL << 24) | (((frac)&7UL) << 0))
#if defined(BT_81XA_ENABLE) || defined(EVE_MULTI_TARGET)
#define BITMAP_LAYOUT_H(linestride, height) ((40UL << 24) | (((linestride)&7UL) << 2) | (((height)&3UL) << 0))
#else
#define BITMAP_LAYOUT_H(linestride, height) ((40UL << 24) | (((linestride)&3UL) << 2) | (((height)&3UL) << 0))
#endif
#define BITMAP_SIZE_H(width, height) ((41UL << 24) | (((width)&3UL) << 2) | (((height)&3UL) << 0))
#define PALETTE_SOURCE(addr) ((42UL << 24) | (((addr)&4194303UL) << 0))
#define VERTEX_TRANSLATE_X(x) ((43UL << 24) | (((x)&131071UL) << 0))
#define VERTEX_TRANSLATE_Y(y) ((44UL << 24) | (((y)&131071UL) << 0))
#define NOP() ((45UL << 24))
#define BITMAP_EXT_FORMAT(format) ((46UL << 24) | (((format)&65535UL) << 0))
#define BITMAP_SWIZZLE(r, g, b, a) ((47UL << 24) | (((r)&7UL) << 9) | (((g)&7UL) << 6) | (((b)&7UL) << 3) | (((a)&7UL) << 0))
#define INT_FRR() ((48UL << 24))
#define END() ((33UL << 24))
#define SAVE_CONTEXT() ((34UL << 24))
#define RESTORE_CONTEXT() ((35UL << 24))
#define RETURN() ((36UL << 24))
#define MACRO(m) ((37UL << 24) | (((m)&1UL) << 0))
#define DISPLAY() ((0UL << 24))

/************
** Options **
************/

#ifdef POINTS
#undef POINTS
#endif

#define DLSWAP_DONE 0UL
#define DLSWAP_LINE 1UL
#define DLSWAP_FRAME 2UL

ESD_ENUM(Ft_CoPro_Opt, Type = uint16_t, Include = "EVE_Hal.h", Flags)
#define OPT_MONO 1UL
#define OPT_NODL 2UL
#define OPT_NOTEAR 4UL
#define OPT_FULLSCREEN 8UL
#define OPT_MEDIAFIFO 16UL
#define OPT_SOUND 32UL
#define OPT_FLASH 64UL
#define OPT_OVERLAY 128UL
#define OPT_FLAT 256UL
#define OPT_SIGNED 256UL
#define OPT_DITHER 256UL
#define OPT_CENTERX 512UL
#define OPT_CENTERY 1024UL
#define OPT_CENTER 1536UL
#define OPT_FORMAT 4096UL
#define OPT_NOBACK 4096UL
#define OPT_FILL 8192UL
#define OPT_NOHM 16384UL
#define OPT_NOPOINTER 16384UL
#define OPT_NOSECS 32768UL
#define OPT_NOHANDS 49152UL
#define OPT_NOTICKS 8192UL
#define OPT_RIGHTX 2048UL
ESD_END()

#define ANIM_ONCE 0UL
#define ANIM_LOOP 1UL
#define ANIM_HOLD 2UL

#define COMPRESSED_RGBA_ASTC_4x4_KHR 37808UL
#define COMPRESSED_RGBA_ASTC_5x4_KHR 37809UL
#define COMPRESSED_RGBA_ASTC_5x5_KHR 37810UL
#define COMPRESSED_RGBA_ASTC_6x5_KHR 37811UL
#define COMPRESSED_RGBA_ASTC_6x6_KHR 37812UL
#define COMPRESSED_RGBA_ASTC_8x5_KHR 37813UL
#define COMPRESSED_RGBA_ASTC_8x6_KHR 37814UL
#define COMPRESSED_RGBA_ASTC_8x8_KHR 37815UL
#define COMPRESSED_RGBA_ASTC_10x5_KHR 37816UL
#define COMPRESSED_RGBA_ASTC_10x6_KHR 37817UL
#define COMPRESSED_RGBA_ASTC_10x8_KHR 37818UL
#define COMPRESSED_RGBA_ASTC_10x10_KHR 37819UL
#define COMPRESSED_RGBA_ASTC_12x10_KHR 37820UL
#define COMPRESSED_RGBA_ASTC_12x12_KHR 37821UL

#define FLASH_DEFAULT_SHA1KEY 0xf589cf07

#define KEEP 1UL
#define REPLACE 2UL
#define INCR 3UL
#define DECR 4UL
#define INVERT 5UL

#define ZERO 0UL
#define ONE 1UL
#define SRC_ALPHA 2UL
#define DST_ALPHA 3UL
#define ONE_MINUS_SRC_ALPHA 4UL
#define ONE_MINUS_DST_ALPHA 5UL

#define RED 2UL
#define GREEN 3UL
#define BLUE 4UL
#define ALPHA 5UL

#define NEVER 0UL
#define LESS 1UL
#define LEQUAL 2UL
#define GREATER 3UL
#define GEQUAL 4UL
#define EQUAL 5UL
#define NOTEQUAL 6UL
#define ALWAYS 7UL

#define ARGB1555 0UL
#define L1 1UL
#define L4 2UL
#define L8 3UL
#define RGB332 4UL
#define ARGB2 5UL
#define ARGB4 6UL
#define RGB565 7UL
#define PALETTED 8UL
#define TEXT8X8 9UL
#define TEXTVGA 10UL
#define BARGRAPH 11UL
#define PALETTED565 14UL
#define PALETTED4444 15UL
#define PALETTED8 16UL
#define L2 17UL
#define GLFORMAT 31UL

#define FLASH_STATUS_INIT 0UL
#define FLASH_STATUS_DETACHED 1UL
#define FLASH_STATUS_BASIC 2UL
#define FLASH_STATUS_FULL 3UL

#define NEAREST 0UL
#define BILINEAR 1UL

#define LINEAR_SAMPLES 0UL
#define ULAW_SAMPLES 1UL
#define ADPCM_SAMPLES 2UL

#define BITMAPS 1UL
#define POINTS 2UL
#define LINES 3UL
#define LINE_STRIP 4UL
#define EDGE_STRIP_R 5UL
#define EDGE_STRIP_L 6UL
#define EDGE_STRIP_A 7UL
#define EDGE_STRIP_B 8UL
#define RECTS 9UL

#define FTPOINTS POINTS

#define TOUCHMODE_OFF 0UL
#define TOUCHMODE_ONESHOT 1UL
#define TOUCHMODE_FRAME 2UL
#define TOUCHMODE_CONTINUOUS 3UL

#define BORDER 0UL
#define REPEAT 1UL

#define ADC_SINGLE_ENDED 0UL
#define ADC_DIFFERENTIAL 1UL

#define CTOUCH_MODE_COMPATIBILITY 1UL
#define CTOUCH_MODE_EXTENDED 0UL

#define INT_SWAP 1UL
#define INT_TOUCH 2UL
#define INT_TAG 4UL
#define INT_SOUND 8UL
#define INT_L8C 12UL
#define INT_VGA 13UL
#define INT_G8 18UL
#define INT_PLAYBACK 16UL
#define INT_CMDEMPTY 32UL
#define INT_CMDFLAG 64UL
#define INT_CONVCOMPLETE 128UL
#define INT_UNDERRUN 256UL

#endif /* #ifndef EVE_GPU_DEFS__H */

/* end of file */
