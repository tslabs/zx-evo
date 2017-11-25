
#pragma once

// Types
typedef struct
{
  u8 f_mul;       // PLL multiplier
  u8 f_div;       // Pixel Clock divisor
  u16 h_fporch;   // Horizontal front porch size
  u16 h_sync;     // Horizontal sync size
  u16 h_bporch;   // Horizontal back porch size
  u16 h_visible;  // Horizontal visible area size
  u16 v_fporch;   // Vertical front porch size
  u16 v_sync;     // Vertical sync size
  u16 v_bporch;   // Vertical back porch size
  u16 v_visible;  // Vertical visible area size
} FT_MODE;

enum  // const FT_MODE ft_modes[] in ft812func.c
{
  FT_MODE_0,
  FT_MODE_1,
  FT_MODE_2,
  FT_MODE_3,
  FT_MODE_4,
  FT_MODE_5,
  FT_MODE_6,
  FT_MODE_7,
  FT_MODE_8,
  FT_MODE_9,
  FT_MODE_10,
  FT_MODE_11,
  FT_MODE_12,
};

// SPI
#define SPI_CTRL 0x77
#define SPI_DATA 0x57

#define SPI_FT_CS_ON  7
#define SPI_FT_CS_OFF 3

// Memory addresses
#define FT_RAM_G           0x000000   // Main graphics RAM
#define FT_ROM_CHIPID      0x0C0000   // Chip ID and revision
#define FT_ROM_FONT        0x1E0000   // Fonts
#define FT_ROM_FONT_ADDR   0x2FFFFC   // Font table pointer address
#define FT_RAM_DL          0x300000   // Display list RAM
#define FT_RAM_REG         0x302000   // Registers
#define FT_RAM_CMD         0x308000   // Coprocessor command buffer

// Commands
#define FT_CMD_ACTIVE      0x00   // cc 00 00
#define FT_CMD_STANDBY     0x41   // cc 00 00
#define FT_CMD_SLEEP       0x42   // cc 00 00
#define FT_CMD_PWRDOWN     0x43   // cc 00 00
#define FT_CMD_CLKEXT      0x44   // cc 00 00
#define FT_CMD_CLKINT      0x48   // cc 00 00
#define FT_CMD_PDROMS      0x49   // cc xx 00
#define FT_CMD_CLKSEL      0x61   // cc xx 00 -> [5:0] - mul, [7:6] - PLL range (0 for mul=0..3, 1 for mul=4..5)
#define FT_CMD_RST_PULSE   0x68   // cc 00 00

// ID
#define FT_ID   0x7C

// Registers
#define FT_REG_ID                 0x302000
#define FT_REG_FRAMES             0x302004
#define FT_REG_CLOCK              0x302008
#define FT_REG_FREQUENCY          0x30200C
#define FT_REG_RENDERMODE         0x302010
#define FT_REG_SNAPY              0x302014
#define FT_REG_SNAPSHOT           0x302018
#define FT_REG_SNAPFORMAT         0x30201C
#define FT_REG_CPURESET           0x302020
#define FT_REG_TAP_CRC            0x302024
#define FT_REG_TAP_MASK           0x302028
#define FT_REG_HCYCLE             0x30202C
#define FT_REG_HOFFSET            0x302030
#define FT_REG_HSIZE              0x302034
#define FT_REG_HSYNC0             0x302038
#define FT_REG_HSYNC1             0x30203C
#define FT_REG_VCYCLE             0x302040
#define FT_REG_VOFFSET            0x302044
#define FT_REG_VSIZE              0x302048
#define FT_REG_VSYNC0             0x30204C
#define FT_REG_VSYNC1             0x302050
#define FT_REG_DLSWAP             0x302054
#define FT_REG_ROTATE             0x302058
#define FT_REG_OUTBITS            0x30205C
#define FT_REG_DITHER             0x302060
#define FT_REG_SWIZZLE            0x302064
#define FT_REG_CSPREAD            0x302068
#define FT_REG_PCLK_POL           0x30206C
#define FT_REG_PCLK               0x302070
#define FT_REG_TAG_X              0x302074
#define FT_REG_TAG_Y              0x302078
#define FT_REG_TAG                0x30207C
#define FT_REG_VOL_PB             0x302080
#define FT_REG_VOL_SOUND          0x302084
#define FT_REG_SOUND              0x302088
#define FT_REG_PLAY               0x30208C
#define FT_REG_GPIO_DIR           0x302090
#define FT_REG_GPIO               0x302094
#define FT_REG_GPIOX_DIR          0x302098
#define FT_REG_GPIOX              0x30209C
#define FT_REG_INT_FLAGS          0x3020A8
#define FT_REG_INT_EN             0x3020AC
#define FT_REG_INT_MASK           0x3020B0
#define FT_REG_PLAYBACK_START     0x3020B4
#define FT_REG_PLAYBACK_LENGTH    0x3020B8
#define FT_REG_PLAYBACK_READPTR   0x3020BC
#define FT_REG_PLAYBACK_FREQ      0x3020C0
#define FT_REG_PLAYBACK_FORMAT    0x3020C4
#define FT_REG_PLAYBACK_LOOP      0x3020C8
#define FT_REG_PLAYBACK_PLAY      0x3020CC
#define FT_REG_PWM_HZ             0x3020D0
#define FT_REG_PWM_DUTY           0x3020D4
#define FT_REG_MACRO_0            0x3020D8
#define FT_REG_MACRO_1            0x3020DC
#define FT_REG_CMD_READ           0x3020F8
#define FT_REG_CMD_WRITE          0x3020FC
#define FT_REG_CMD_DL             0x302100
#define FT_REG_TOUCH_MODE         0x302104
#define FT_REG_TOUCH_ADC_MODE     0x302108
#define FT_REG_TOUCH_CHARGE       0x30210C
#define FT_REG_TOUCH_SETTLE       0x302110
#define FT_REG_TOUCH_OVERSAMPLE   0x302114
#define FT_REG_TOUCH_RZTHRESH     0x302118
#define FT_REG_TOUCH_RAW_XY       0x30211C
#define FT_REG_TOUCH_RZ           0x302120
#define FT_REG_TOUCH_SCREEN_XY    0x302124
#define FT_REG_TOUCH_TAG_XY       0x302128
#define FT_REG_TOUCH_TAG          0x30212C
#define FT_REG_TOUCH_TAG1_XY      0x302130
#define FT_REG_TOUCH_TAG1         0x302134
#define FT_REG_TOUCH_TAG2_XY      0x302138
#define FT_REG_TOUCH_TAG2         0x30213C
#define FT_REG_TOUCH_TAG3_XY      0x302140
#define FT_REG_TOUCH_TAG3         0x302144
#define FT_REG_TOUCH_TAG4_XY      0x302148
#define FT_REG_TOUCH_TAG4         0x30214C
#define FT_REG_TOUCH_TRANSFORM_A  0x302150
#define FT_REG_TOUCH_TRANSFORM_B  0x302154
#define FT_REG_TOUCH_TRANSFORM_C  0x302158
#define FT_REG_TOUCH_TRANSFORM_D  0x30215C
#define FT_REG_TOUCH_TRANSFORM_E  0x302160
#define FT_REG_TOUCH_TRANSFORM_F  0x302164
#define FT_REG_TOUCH_CONFIG       0x302168
#define FT_REG_CTOUCH_TOUCH4_X    0x30216C
#define FT_REG_BIST_EN            0x302174
#define FT_REG_TRIM               0x302180
#define FT_REG_ANA_COMP           0x302184
#define FT_REG_SPI_WIDTH          0x302188
#define FT_REG_TOUCH_DIRECT_XY    0x30218C
#define FT_REG_TOUCH_DIRECT_Z1Z2  0x302190
#define FT_REG_DATESTAMP          0x302564
#define FT_REG_CMDB_SPACE         0x302574
#define FT_REG_CMDB_WRITE         0x302578
#define FT_REG_TRACKER            0x309000
#define FT_REG_TRACKER_1          0x309004
#define FT_REG_TRACKER_2          0x309008
#define FT_REG_TRACKER_3          0x30900C
#define FT_REG_TRACKER_4          0x309010
#define FT_REG_MEDIAFIFO_READ     0x309014
#define FT_REG_MEDIAFIFO_WRITE    0x309018

// Co-processor commands
#define FT_CCMD_APPEND            0xFFFFFF1E
#define FT_CCMD_BGCOLOR           0xFFFFFF09
#define FT_CCMD_BITMAP_TRANSFORM  0xFFFFFF21
#define FT_CCMD_BUTTON            0xFFFFFF0D
#define FT_CCMD_CALIBRATE         0xFFFFFF15
#define FT_CCMD_CLOCK             0xFFFFFF14
#define FT_CCMD_COLDSTART         0xFFFFFF32
#define FT_CCMD_CRC               0xFFFFFF03
#define FT_CCMD_CSKETCH           0xFFFFFF35
#define FT_CCMD_DIAL              0xFFFFFF2D
#define FT_CCMD_DLSTART           0xFFFFFF00
#define FT_CCMD_EXECUTE           0xFFFFFF07
#define FT_CCMD_FGCOLOR           0xFFFFFF0A
#define FT_CCMD_GAUGE             0xFFFFFF13
#define FT_CCMD_GETMATRIX         0xFFFFFF33
#define FT_CCMD_GETPOINT          0xFFFFFF08
#define FT_CCMD_GETPROPS          0xFFFFFF25
#define FT_CCMD_GETPTR            0xFFFFFF23
#define FT_CCMD_GRADCOLOR         0xFFFFFF34
#define FT_CCMD_GRADIENT          0xFFFFFF0B
#define FT_CCMD_HAMMERAUX         0xFFFFFF04
#define FT_CCMD_IDCT_DELETED      0xFFFFFF06
#define FT_CCMD_INFLATE           0xFFFFFF22
#define FT_CCMD_INTERRUPT         0xFFFFFF02
#define FT_CCMD_INT_RAMSHARED     0xFFFFFF3D
#define FT_CCMD_INT_SWLOADIMAGE   0xFFFFFF3E
#define FT_CCMD_KEYS              0xFFFFFF0E
#define FT_CCMD_LOADIDENTITY      0xFFFFFF26
#define FT_CCMD_LOADIMAGE         0xFFFFFF24
#define FT_CCMD_LOGO              0xFFFFFF31
#define FT_CCMD_MARCH             0xFFFFFF05
#define FT_CCMD_MEDIAFIFO         0xFFFFFF39
#define FT_CCMD_MEMCPY            0xFFFFFF1D
#define FT_CCMD_MEMCRC            0xFFFFFF18
#define FT_CCMD_MEMSET            0xFFFFFF1B
#define FT_CCMD_MEMWRITE          0xFFFFFF1A
#define FT_CCMD_MEMZERO           0xFFFFFF1C
#define FT_CCMD_NUMBER            0xFFFFFF2E
#define FT_CCMD_PLAYVIDEO         0xFFFFFF3A
#define FT_CCMD_PROGRESS          0xFFFFFF0F
#define FT_CCMD_REGREAD           0xFFFFFF19
#define FT_CCMD_ROMFONT           0xFFFFFF3F
#define FT_CCMD_ROTATE            0xFFFFFF29
#define FT_CCMD_SCALE             0xFFFFFF28
#define FT_CCMD_SCREENSAVER       0xFFFFFF2F
#define FT_CCMD_SCROLLBAR         0xFFFFFF11
#define FT_CCMD_SETBASE           0xFFFFFF38
#define FT_CCMD_SETBITMAP         0xFFFFFF43
#define FT_CCMD_SETFONT           0xFFFFFF2B
#define FT_CCMD_SETFONT2          0xFFFFFF3B
#define FT_CCMD_SETMATRIX         0xFFFFFF2A
#define FT_CCMD_SETROTATE         0xFFFFFF36
#define FT_CCMD_SETSCRATCH        0xFFFFFF3C
#define FT_CCMD_SKETCH            0xFFFFFF30
#define FT_CCMD_SLIDER            0xFFFFFF10
#define FT_CCMD_SNAPSHOT          0xFFFFFF1F
#define FT_CCMD_SNAPSHOT2         0xFFFFFF37
#define FT_CCMD_SPINNER           0xFFFFFF16
#define FT_CCMD_STOP              0xFFFFFF17
#define FT_CCMD_SWAP              0xFFFFFF01
#define FT_CCMD_SYNC              0xFFFFFF42
#define FT_CCMD_TEXT              0xFFFFFF0C
#define FT_CCMD_TOGGLE            0xFFFFFF12
#define FT_CCMD_TOUCH_TRANSFORM   0xFFFFFF20
#define FT_CCMD_TRACK             0xFFFFFF2C
#define FT_CCMD_TRANSLATE         0xFFFFFF27
#define FT_CCMD_VIDEOFRAME        0xFFFFFF41
#define FT_CCMD_VIDEOSTART        0xFFFFFF40

#define FT_OPT_CENTER           1536UL
#define FT_OPT_CENTERX          512UL
#define FT_OPT_CENTERY          1024UL
#define FT_OPT_FLAT             256UL
#define FT_OPT_MONO             1UL
#define FT_OPT_NOBACK           4096UL
#define FT_OPT_NODL             2UL
#define FT_OPT_NOHANDS          49152UL
#define FT_OPT_NOHM             16384UL
#define FT_OPT_NOPOINTER        16384UL
#define FT_OPT_NOSECS           32768UL
#define FT_OPT_NOTICKS          8192UL
#define FT_OPT_RIGHTX           2048UL
#define FT_OPT_SIGNED           256UL

// Primitives
#define FT_BITMAPS        1
#define FT_POINTS         2
#define FT_LINES          3
#define FT_LINE_STRIP     4
#define FT_EDGE_STRIP_R   5
#define FT_EDGE_STRIP_L   6
#define FT_EDGE_STRIP_A   7
#define FT_EDGE_STRIP_B   8
#define FT_RECTS          9

// Formats
#define FT_ARGB1555          0
#define FT_L1                1
#define FT_L4                2
#define FT_L8                3
#define FT_RGB332            4
#define FT_ARGB2             5
#define FT_ARGB4             6
#define FT_RGB565            7
#define FT_TEXT8X8           9
#define FT_TEXTVGA           10
#define FT_BARGRAPH          11
#define FT_PALETTED565       14
#define FT_PALETTED4444      15
#define FT_PALETTED8         16
#define FT_L2                17

// Blend functions
#define FT_ZERO                 0
#define FT_ONE                  1
#define FT_SRC_ALPHA            2
#define FT_DST_ALPHA            3
#define FT_ONE_MINUS_SRC_ALPHA  4
#define FT_ONE_MINUS_DST_ALPHA  5

// Stencil test functions
#define FT_NEVER          0
#define FT_LESS           1
#define FT_LEQUAL         2
#define FT_GREATER        3
#define FT_GEQUAL         4
#define FT_EQUAL          5
#define FT_NOTEQUAL       6
#define FT_ALWAYS         7

// Stencil operations
#define FT_KEEP           1
#define FT_REPLACE        2
#define FT_INCR           3
#define FT_DECR           4
#define FT_INVERT         5
#define FT_INCR_WRAP      6 // undocumented???
#define FT_DECR_WRAP      7 // undocumented???

// Bitmap wrap mode
#define FT_REPEAT     1
#define FT_BORDER     0

// Bitmap filtering mode
#define FT_NEAREST    0
#define FT_BILINEAR   1

// DL_SWAP modes
#define FT_DLSWAP_DONE          0
#define FT_DLSWAP_LINE          1
#define FT_DLSWAP_FRAME         2

// INT sources
#define FT_INT_SWAP             1
#define FT_INT_TOUCH            2
#define FT_INT_TAG              4
#define FT_INT_SOUND            8
#define FT_INT_PLAYBACK         16
#define FT_INT_CMDEMPTY         32
#define FT_INT_CMDFLAG          64
#define FT_INT_CONVCOMPLETE     128

// Playback formats
#define FT_LINEAR_SAMPLES       0
#define FT_ULAW_SAMPLES         1
#define FT_ADPCM_SAMPLES        2

// Touch ADC modes
#define FT_ADC_SINGLE_ENDED     0
#define FT_ADC_DIFFERENTIAL     1

// Touch modes
#define FT_TOUCHMODE_OFF        0
#define FT_TOUCHMODE_ONESHOT    1
#define FT_TOUCHMODE_FRAME      2
#define FT_TOUCHMODE_CONTINUOUS 3

// Constants
#define FT_DL_SIZE              8192  // 8KB Display List buffer size
#define FT_CMD_FIFO_SIZE        4096  // 4KB coprocessor Fifo size
#define FT_CMD_SIZE             4     // 4 byte per coprocessor command of EVE
#define FT_GPU_NUMCHAR_PERFONT  128
#define FT_GPU_FONT_TABLE_SIZE  148
