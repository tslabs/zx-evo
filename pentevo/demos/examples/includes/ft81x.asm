
; -- Ports
SPI_CTRL  equ 0x77
SPI_DATA  equ 0x57

SPI_FT_CS_OFF equ 3
SPI_FT_CS_ON  equ 7

; -- Memory addresses
FT_RAM_G           equ 0x000000   ; Main graphics RAM
FT_ROM_CHIPID      equ 0x0C0000   ; Chip ID and revision
FT_ROM_FONT        equ 0x1E0000   ; Fonts
FT_ROM_FONT_ADDR   equ 0x2FFFFC   ; Font table pointer address
FT_RAM_DL          equ 0x300000   ; Display list RAM
FT_RAM_REG         equ 0x302000   ; Registers
FT_RAM_CMD         equ 0x308000   ; Coprocessor command buffer

; -- Commands
FT_CMD_STANDBY     equ 0x41   ; cc 00 00
FT_CMD_SLEEP       equ 0x42   ; cc 00 00
FT_CMD_PWRDOWN     equ 0x43   ; cc 00 00
FT_CMD_CLKEXT      equ 0x44   ; cc 00 00
FT_CMD_CLKINT      equ 0x48   ; cc 00 00
FT_CMD_PDROMS      equ 0x49   ; cc xx 00
FT_CMD_CLKSEL      equ 0x61   ; cc xx 00 -> [5:0] - mul, [7:6] - PLL range (0 for mul=0..3, 1 for mul=4..5)
FT_CMD_RST_PULSE   equ 0x68   ; cc 00 00

; -- Registers
FT_REG_ID                 equ 0x302000
FT_REG_FRAMES             equ 0x302004
FT_REG_CLOCK              equ 0x302008
FT_REG_FREQUENCY          equ 0x30200C
FT_REG_RENDERMODE         equ 0x302010
FT_REG_SNAPY              equ 0x302014
FT_REG_SNAPSHOT           equ 0x302018
FT_REG_SNAPFORMAT         equ 0x30201C
FT_REG_CPURESET           equ 0x302020
FT_REG_TAP_CRC            equ 0x302024
FT_REG_TAP_MASK           equ 0x302028
FT_REG_HCYCLE             equ 0x30202C
FT_REG_HOFFSET            equ 0x302030
FT_REG_HSIZE              equ 0x302034
FT_REG_HSYNC0             equ 0x302038
FT_REG_HSYNC1             equ 0x30203C
FT_REG_VCYCLE             equ 0x302040
FT_REG_VOFFSET            equ 0x302044
FT_REG_VSIZE              equ 0x302048
FT_REG_VSYNC0             equ 0x30204C
FT_REG_VSYNC1             equ 0x302050
FT_REG_DLSWAP             equ 0x302054
FT_REG_ROTATE             equ 0x302058
FT_REG_OUTBITS            equ 0x30205C
FT_REG_DITHER             equ 0x302060
FT_REG_SWIZZLE            equ 0x302064
FT_REG_CSPREAD            equ 0x302068
FT_REG_PCLK_POL           equ 0x30206C
FT_REG_PCLK               equ 0x302070
FT_REG_TAG_X              equ 0x302074
FT_REG_TAG_Y              equ 0x302078
FT_REG_TAG                equ 0x30207C
FT_REG_VOL_PB             equ 0x302080
FT_REG_VOL_SOUND          equ 0x302084
FT_REG_SOUND              equ 0x302088
FT_REG_PLAY               equ 0x30208C
FT_REG_GPIO_DIR           equ 0x302090
FT_REG_GPIO               equ 0x302094
FT_REG_GPIOX_DIR          equ 0x302098
FT_REG_GPIOX              equ 0x30209C

FT_REG_INT_FLAGS          equ 0x3020A8
FT_REG_INT_EN             equ 0x3020AC
FT_REG_INT_MASK           equ 0x3020B0
FT_REG_PLAYBACK_START     equ 0x3020B4
FT_REG_PLAYBACK_LENGTH    equ 0x3020B8
FT_REG_PLAYBACK_READPTR   equ 0x3020BC
FT_REG_PLAYBACK_FREQ      equ 0x3020C0
FT_REG_PLAYBACK_FORMAT    equ 0x3020C4
FT_REG_PLAYBACK_LOOP      equ 0x3020C8
FT_REG_PLAYBACK_PLAY      equ 0x3020CC
FT_REG_PWM_HZ             equ 0x3020D0
FT_REG_PWM_DUTY           equ 0x3020D4
FT_REG_MACRO_0            equ 0x3020D8
FT_REG_MACRO_1            equ 0x3020DC

FT_REG_CMD_READ           equ 0x3020F8
FT_REG_CMD_WRITE          equ 0x3020FC
FT_REG_CMD_DL             equ 0x302100
FT_REG_TOUCH_MODE         equ 0x302104
FT_REG_TOUCH_ADC_MODE     equ 0x302108
FT_REG_TOUCH_CHARGE       equ 0x30210C
FT_REG_TOUCH_SETTLE       equ 0x302110
FT_REG_TOUCH_OVERSAMPLE   equ 0x302114
FT_REG_TOUCH_RZTHRESH     equ 0x302118
FT_REG_TOUCH_RAW_XY       equ 0x30211C
FT_REG_TOUCH_RZ           equ 0x302120
FT_REG_TOUCH_SCREEN_XY    equ 0x302124
FT_REG_TOUCH_TAG_XY       equ 0x302128
FT_REG_TOUCH_TAG          equ 0x30212C
FT_REG_TOUCH_TAG1_XY      equ 0x302130
FT_REG_TOUCH_TAG1         equ 0x302134
FT_REG_TOUCH_TAG2_XY      equ 0x302138
FT_REG_TOUCH_TAG2         equ 0x30213C
FT_REG_TOUCH_TAG3_XY      equ 0x302140
FT_REG_TOUCH_TAG3         equ 0x302144
FT_REG_TOUCH_TAG4_XY      equ 0x302148
FT_REG_TOUCH_TAG4         equ 0x30214C
FT_REG_TOUCH_TRANSFORM_A  equ 0x302150
FT_REG_TOUCH_TRANSFORM_B  equ 0x302154
FT_REG_TOUCH_TRANSFORM_C  equ 0x302158
FT_REG_TOUCH_TRANSFORM_D  equ 0x30215C
FT_REG_TOUCH_TRANSFORM_E  equ 0x302160
FT_REG_TOUCH_TRANSFORM_F  equ 0x302164
FT_REG_TOUCH_CONFIG       equ 0x302168
FT_REG_CTOUCH_TOUCH4_X    equ 0x30216C

FT_REG_BIST_EN            equ 0x302174

FT_REG_TRIM               equ 0x302180
FT_REG_ANA_COMP           equ 0x302184
FT_REG_SPI_WIDTH          equ 0x302188
FT_REG_TOUCH_DIRECT_XY    equ 0x30218C
FT_REG_TOUCH_DIRECT_Z1Z2  equ 0x302190

FT_REG_DATESTAMP          equ 0x302564

FT_REG_CMDB_SPACE         equ 0x302574
FT_REG_CMDB_WRITE         equ 0x302578

FT_REG_TRACKER            equ 0x309000
FT_REG_TRACKER_1          equ 0x309004
FT_REG_TRACKER_2          equ 0x309008
FT_REG_TRACKER_3          equ 0x30900C
FT_REG_TRACKER_4          equ 0x309010

; -- DL macros
  macro FT_DISPLAY
    defd 0 << 24
  endm

  macro FT_VERTEX2F x, y
    defd (1 << 30) | (((x) & 32767) << 15) | (((y) & 32767) << 0)
  endm

  macro FT_VERTEX2II x, y, handle, cell
    defd (2<<30) | (((x) & 511) << 21) | (((y) & 511) << 12) | (((handle) & 31) << 7) | (((cell) & 127) << 0)
  endm

  macro FT_BITMAP_SOURCE addr
    defd (1 << 24) | (((addr) & 1048575) << 0)
  endm

  macro FT_CLEAR_COLOR_RGB red, green, blue
    defd (2<<24) | (((red) & 255) << 16) | (((green) & 255) << 8) | (((blue) & 255) << 0)
  endm

  macro FT_TAG s
    defd (3 << 24) | (((s) & 255) << 0)
  endm

  macro FT_COLOR_RGB red, green, blue
    defd (4 << 24) | (((red) & 255) << 16) | (((green) & 255) << 8) | (((blue) & 255) << 0)
  endm

  macro FT_BITMAP_HANDLE handle
    defd (5 << 24) | (((handle) & 31) << 0)
  endm

  macro FT_CELL cell
    defd (6 << 24) | (((cell) & 127) << 0)
  endm

  macro FT_BITMAP_LAYOUT format, linestride, height
    defd (7 << 24) | (((format) & 31) << 19) | (((linestride) & 1023) << 9) | (((height) & 511) << 0)
  endm

  macro FT_BITMAP_SIZE filter, wrapx, wrapy, width, height
    defd (8 << 24) | (((filter) & 1) << 20) | (((wrapx) & 1) << 19) | (((wrapy) & 1) << 18) | (((width) & 511) << 9) | (((height) & 511) << 0)
  endm

  macro FT_ALPHA_FUNC func, ref
    defd (9 << 24) | (((func) & 7) << 8) | (((ref) & 255) << 0)
  endm

  macro FT_STENCIL_FUNC func, ref, mask
    defd (10 << 24) | (((func) & 7) << 16) | (((ref) & 255) << 8) | (((mask) & 255) << 0)
  endm

  macro FT_BLEND_FUNC src, dst
    defd (11 << 24) | (((src) & 7) << 3) | (((dst) & 7) << 0)
  endm

  macro FT_STENCIL_OP sfail, spass
    defd (12 << 24) | (((sfail) & 7) << 3) | (((spass) & 7) << 0)
  endm

  macro FT_POINT_SIZE size
    defd (13 << 24) | (((size) & 8191) << 0)
  endm

  macro FT_LINE_WIDTH width
    defd (14 << 24) | (((width) & 4095) << 0)
  endm

  macro FT_CLEAR_COLOR_A alpha
    defd (15 << 24) | (((alpha) & 255) << 0)
  endm

  macro FT_COLOR_A alpha
    defd (16 << 24) | (((alpha) & 255) << 0)
  endm

  macro FT_CLEAR_STENCIL s
    defd (17 << 24) | (((s) & 255) << 0)
  endm

  macro FT_CLEAR_TAG s
    defd (18 << 24) | (((s) & 255) << 0)
  endm

  macro FT_STENCIL_MASK mask
    defd (19 << 24) | (((mask) & 255) << 0)
  endm

  macro FT_TAG_MASK mask
    defd (20 << 24) | (((mask) & 1) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_A a
    defd (21 << 24) | (((a) & 131071) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_B b
    defd (22 << 24) | (((b) & 131071) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_C c
    defd (23 << 24) | (((c) & 16777215) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_D d
    defd (24 << 24) | (((d) & 131071) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_E e
    defd (25 << 24) | (((e) & 131071) << 0)
  endm

  macro FT_BITMAP_TRANSFORM_F f
    defd (26 << 24) | (((f) & 16777215) << 0)
  endm

  macro FT_SCISSOR_XY x, y
    defd (27 << 24) | (((x) & 511) << 9) | (((y) & 511) << 0)
  endm

  macro FT_SCISSOR_SIZE width, height
    defd (28 << 24) | (((width) & 1023) << 10) | (((height) & 1023) << 0)
  endm

  macro FT_CALL dest
    defd (29 << 24) | (((dest) & 65535) << 0)
  endm

  macro FT_JUMP dest
    defd (30 << 24) | (((dest) & 65535) << 0)
  endm

  macro FT_BEGIN prim
    defd (31 << 24) | (((prim) & 15) << 0)
  endm

  macro FT_COLOR_MASK r, g, b, a
    defd (32 << 24) | (((r) & 1) << 3) | (((g) & 1) << 2) | (((b) & 1) << 1) | (((a) & 1) << 0)
  endm

  macro FT_END
    defd (33 << 24)
  endm

  macro FT_SAVE_CONTEXT
    defd (34 << 24)
  endm

  macro FT_RESTORE_CONTEXT
    defd (35 << 24)
  endm

  macro FT_RETURN
    defd (36 << 24)
  endm

  macro FT_MACRO m
    defd (37 << 24) | (((m) & 1) << 0))
  endm

  macro FT_CLEAR c, s, t
    defd (38 << 24) | (((c) & 1) <<2 ) | (((s) & 1) << 1) | (((t) & 1) << 0)
  endm

; -- Test functions
FT_NEVER          equ 0
FT_LESS           equ 1
FT_LEQUAL         equ 2
FT_GREATER        equ 3
FT_GEQUAL         equ 4
FT_EQUAL          equ 5
FT_NOTEQUAL       equ 6
FT_ALWAYS         equ 7

; -- Primitives
FT_BITMAPS        equ 1
FT_POINTS         equ 2
FT_LINES          equ 3
FT_LINE_STRIP     equ 4
FT_EDGE_STRIP_R   equ 5
FT_EDGE_STRIP_L   equ 6
FT_EDGE_STRIP_A   equ 7
FT_EDGE_STRIP_B   equ 8
FT_RECTS          equ 9

; -- Formats
FT_ARGB1555          equ 0
FT_L1                equ 1
FT_L4                equ 2
FT_L8                equ 3
FT_RGB332            equ 4
FT_ARGB2             equ 5
FT_ARGB4             equ 6
FT_RGB565            equ 7
FT_PALETTED          equ 8  ; deprecated???
FT_TEXT8X8           equ 9
FT_TEXTVGA           equ 10
FT_BARGRAPH          equ 11
FT_PALETTED565       equ 14
FT_PALETTED4444      equ 15
FT_PALETTED8         equ 16
FT_L2                equ 17

; -- Blend functions
FT_ZERO                 equ 0
FT_ONE                  equ 1
FT_SRC_ALPHA            equ 2
FT_DST_ALPHA            equ 3
FT_ONE_MINUS_SRC_ALPHA  equ 4
FT_ONE_MINUS_DST_ALPHA  equ 5

; -- Stencil operations
FT_KEEP                 equ 1
FT_REPLACE              equ 2
FT_INCR                 equ 3
FT_DECR                 equ 4
FT_INVERT               equ 5
FT_INCR_WRAP            equ 6 ; undocumented???
FT_DECR_WRAP            equ 7 ; undocumented???

; -- Bitmap wrap mode
FT_REPEAT     equ 1
FT_BORDER     equ 0

; -- Bitmap filtering mode
FT_NEAREST    equ 0
FT_BILINEAR   equ 1

; -- DL_SWAP modes
FT_DLSWAP_DONE          equ 0
FT_DLSWAP_LINE          equ 1
FT_DLSWAP_FRAME         equ 2

; -- INT sources
FT_INT_SWAP             equ 1
FT_INT_TOUCH            equ 2
FT_INT_TAG              equ 4
FT_INT_SOUND            equ 8
FT_INT_PLAYBACK         equ 16
FT_INT_CMDEMPTY         equ 32
FT_INT_CMDFLAG          equ 64
FT_INT_CONVCOMPLETE     equ 128

; -- Playback formats
FT_LINEAR_SAMPLES       equ 0
FT_ULAW_SAMPLES         equ 1
FT_ADPCM_SAMPLES        equ 2

; -- Touch ADC modes
FT_ADC_SINGLE_ENDED     equ 0
FT_ADC_DIFFERENTIAL     equ 1

; -- Touch modes
FT_TOUCHMODE_OFF        equ 0
FT_TOUCHMODE_ONESHOT    equ 1
FT_TOUCHMODE_FRAME      equ 2
FT_TOUCHMODE_CONTINUOUS equ 3

; -- Constants
FT_DL_SIZE              equ 8 * 1024  ; 8KB Display List buffer size
FT_CMD_FIFO_SIZE        equ 4 * 1024  ; 4KB coprocessor Fifo size
FT_CMD_SIZE             equ 4         ; 4 byte per coprocessor command of EVE
FT_CMDBUF_SIZE          equ 4096
FT_GPU_NUMCHAR_PERFONT  equ 128
FT_GPU_FONT_TABLE_SIZE  equ 148

; -- Coprocessor commands
FT_CMD_APPEND           equ 0xFFFFFF00 + 30
FT_CMD_BGCOLOR          equ 0xFFFFFF00 + 9
FT_CMD_BITMAP_TRANSFORM equ 0xFFFFFF00 + 33
FT_CMD_BUTTON           equ 0xFFFFFF00 + 13
FT_CMD_CALIBRATE        equ 0xFFFFFF00 + 21
FT_CMD_CLOCK            equ 0xFFFFFF00 + 20
FT_CMD_COLDSTART        equ 0xFFFFFF00 + 50
FT_CMD_CRC              equ 0xFFFFFF00 + 3
FT_CMD_DIAL             equ 0xFFFFFF00 + 45
FT_CMD_DLSTART          equ 0xFFFFFF00 + 0
FT_CMD_EXECUTE          equ 0xFFFFFF00 + 7
FT_CMD_FGCOLOR          equ 0xFFFFFF00 + 10
FT_CMD_GAUGE            equ 0xFFFFFF00 + 19
FT_CMD_GETMATRIX        equ 0xFFFFFF00 + 51
FT_CMD_GETPOINT         equ 0xFFFFFF00 + 8
FT_CMD_GETPROPS         equ 0xFFFFFF00 + 37
FT_CMD_GETPTR           equ 0xFFFFFF00 + 35
FT_CMD_GRADCOLOR        equ 0xFFFFFF00 + 52
FT_CMD_GRADIENT         equ 0xFFFFFF00 + 11
FT_CMD_HAMMERAUX        equ 0xFFFFFF00 + 4
FT_CMD_IDCT             equ 0xFFFFFF00 + 6
FT_CMD_INFLATE          equ 0xFFFFFF00 + 34
FT_CMD_INTERRUPT        equ 0xFFFFFF00 + 2
FT_CMD_KEYS             equ 0xFFFFFF00 + 14
FT_CMD_LOADIDENTITY     equ 0xFFFFFF00 + 38
FT_CMD_LOADIMAGE        equ 0xFFFFFF00 + 36
FT_CMD_LOGO             equ 0xFFFFFF00 + 49
FT_CMD_MARCH            equ 0xFFFFFF00 + 5
FT_CMD_MEMCPY           equ 0xFFFFFF00 + 29
FT_CMD_MEMCRC           equ 0xFFFFFF00 + 24
FT_CMD_MEMSET           equ 0xFFFFFF00 + 27
FT_CMD_MEMWRITE         equ 0xFFFFFF00 + 26
FT_CMD_MEMZERO          equ 0xFFFFFF00 + 28
FT_CMD_NUMBER           equ 0xFFFFFF00 + 46
FT_CMD_PROGRESS         equ 0xFFFFFF00 + 15
FT_CMD_REGREAD          equ 0xFFFFFF00 + 25
FT_CMD_ROTATE           equ 0xFFFFFF00 + 41
FT_CMD_SCALE            equ 0xFFFFFF00 + 40
FT_CMD_SCREENSAVER      equ 0xFFFFFF00 + 47
FT_CMD_SCROLLBAR        equ 0xFFFFFF00 + 17
FT_CMD_SETFONT          equ 0xFFFFFF00 + 43
FT_CMD_SETMATRIX        equ 0xFFFFFF00 + 42
FT_CMD_SKETCH           equ 0xFFFFFF00 + 48
FT_CMD_SLIDER           equ 0xFFFFFF00 + 16
FT_CMD_SNAPSHOT         equ 0xFFFFFF00 + 31
FT_CMD_SPINNER          equ 0xFFFFFF00 + 22
FT_CMD_STOP             equ 0xFFFFFF00 + 23
FT_CMD_SWAP             equ 0xFFFFFF00 + 1
FT_CMD_TEXT             equ 0xFFFFFF00 + 12
FT_CMD_TOGGLE           equ 0xFFFFFF00 + 18
FT_CMD_TOUCH_TRANSFORM  equ 0xFFFFFF00 + 32
FT_CMD_TRACK            equ 0xFFFFFF00 + 44
FT_CMD_TRANSLATE        equ 0xFFFFFF00 + 39

; -- Parameter options
FT_OPT_3D               equ 0
FT_OPT_RGB565           equ 0
FT_OPT_MONO             equ 1
FT_OPT_NODL             equ 2
FT_OPT_FLAT             equ 256
FT_OPT_SIGNED           equ 256
FT_OPT_CENTERX          equ 512
FT_OPT_CENTERY          equ 1024
FT_OPT_CENTER           equ 1536
FT_OPT_RIGHTX           equ 2048
FT_OPT_NOBACK           equ 4096
FT_OPT_NOTICKS          equ 8192
FT_OPT_NOHM             equ 16384
FT_OPT_NOPOINTER        equ 16384
FT_OPT_NOSECS           equ 32768
FT_OPT_NOHANDS          equ 49152
FT_OPT_NOTEAR           equ 4
FT_OPT_FULLSCREEN       equ 8
FT_OPT_MEDIAFIFO        equ 16
FT_OPT_SOUND            equ 32

; -- Function macros
  macro FT_ON
    ld a, SPI_FT_CS_ON
    out (SPI_CTRL), a
  endm

  macro FT_OFF
    ld a, SPI_FT_CS_OFF
    out (SPI_CTRL), a
  endm

  macro FT_ACTIVE
    FT_ON
    xor a
    out (SPI_DATA), a
    out (SPI_DATA), a
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FT_CMD cmd
    FT_ON
    ld a, cmd
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FT_CMDP cmd, par
    FT_ON
    ld a, cmd
    out (SPI_DATA), a
    ld a, par
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FT_WR8 addr, data
    FT_ON
    ld a, (addr >> 16) | 0x80
    out (SPI_DATA), a
    ld a, (addr >> 8) & 0xFF
    out (SPI_DATA), a
    ld a, addr & 0xFF
    out (SPI_DATA), a
    ld a, data
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FT_WR16 addr, data
    FT_ON
    ld a, (addr >> 16) | 0x80
    out (SPI_DATA), a
    ld a, (addr >> 8) & 0xFF
    out (SPI_DATA), a
    ld a, addr & 0xFF
    out (SPI_DATA), a
    ld a, data & 0xFF
    out (SPI_DATA), a
    ld a, (data >> 8) & 0xFF
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FT_WR32 addr, data
    FT_ON
    ld a, (addr >> 16) | 0x80
    out (SPI_DATA), a
    ld a, (addr >> 8) & 0xFF
    out (SPI_DATA), a
    ld a, addr & 0xFF
    out (SPI_DATA), a
    ld a, data & 0xFF
    out (SPI_DATA), a
    ld a, (data >> 8) & 0xFF
    out (SPI_DATA), a
    ld a, (data >> 16) & 0xFF
    out (SPI_DATA), a
    ld a, (data >> 24) & 0xFF
    out (SPI_DATA), a
    FT_OFF
  endm

  macro FL_LOAD_DL addr, num
    FT_ON

    ld a, (FT_RAM_DL >> 16) | 0x80
    out (SPI_DATA), a
    ld a, (FT_RAM_DL >> 8) & 0xFF
    out (SPI_DATA), a
    ld a, FT_RAM_DL & 0xFF
    out (SPI_DATA), a

    ld hl, addr
    ld a, num
    ld c, SPI_DATA
.l1 outi
    outi
    outi
    outi
    dec a
    jr nz, .l1

    FT_OFF
  endm

  macro FT_DELAY del ; !!! will be deprecated
    ld bc, del
.l1 dec bc
    ld a, b
    or c
    jr nz, .l1
  endm
