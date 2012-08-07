#pragma once


#include "sysdefs.h"
#include "defs.h"
#include "sndrender.h"
#include "tsconf.h"

#define EMUL_DEBUG
#define TRASH_PAGE

#define PAGE 0x4000U
#define MAX_RAM_PAGES 256       // 4Mb RAM
#define MAX_CACHE_PAGES 2       // 32K cache
#define MAX_MISC_PAGES 1        // trash page
#define MAX_ROM_PAGES 64        // 1Mb

#define GS4MB //0.37.0
#ifdef MOD_GSZ80
 #define MAX_GSROM_PAGES  32U      // 512Kb
 #ifdef GS4MB
  #define MAX_GSRAM_PAGES 256U     // for gs4mb
 #else
  #define MAX_GSRAM_PAGES 30U      // for gs512 (last 32k unusable)
 #endif
#else
 #define MAX_GSROM_PAGES 0
 #define MAX_GSRAM_PAGES 0
#endif

#define MAX_PAGES (MAX_RAM_PAGES + MAX_CACHE_PAGES + MAX_MISC_PAGES + MAX_ROM_PAGES + MAX_GSROM_PAGES + MAX_GSRAM_PAGES)

#define RAM_BASE_M  memory
#define CACHE_M     (RAM_BASE_M + MAX_RAM_PAGES*PAGE)
#define MISC_BASE_M (CACHE_M + MAX_CACHE_PAGES*PAGE)
#define ROM_BASE_M  (MISC_BASE_M + MAX_MISC_PAGES*PAGE)

#ifdef MOD_GSZ80
#define ROM_GS_M    (ROM_BASE_M + MAX_ROM_PAGES*PAGE)
#define GSRAM_M     (ROM_GS_M + MAX_GSROM_PAGES*PAGE)
#endif

#define TRASH_M     (MISC_BASE_M+0*PAGE)


enum IDE_SCHEME
{
    IDE_NONE = 0,
    IDE_ATM,
    IDE_NEMO, IDE_NEMO_A8, IDE_NEMO_DIVIDE,
    IDE_SMUC,
    IDE_PROFI,
    IDE_DIVIDE,
};

enum MOUSE_WHEEL_MODE { MOUSE_WHEEL_NONE, MOUSE_WHEEL_KEYBOARD, MOUSE_WHEEL_KEMPSTON }; //0.36.6 from 0.35b2

enum MEM_MODEL
{
   MM_PENTAGON = 0,
   MM_SCORP, MM_PROFSCORP,
   MM_PROFI,
   MM_ATM450, MM_ATM710, MM_ATM3,
   MM_KAY,
   MM_PLUS3,
   MM_QUORUM,
   MM_TSL,
   N_MM_MODELS
};

enum ROM_MODE { RM_NOCHANGE=0, RM_SOS, RM_DOS, RM_SYS, RM_128, RM_CACHE };

const int RAM_128 = 128, RAM_256 = 256, RAM_512 = 512, RAM_1024 = 1024, RAM_4096 = 4096;

struct TMemModel
{
   const char *fullname, *shortname;
   MEM_MODEL Model;
   unsigned defaultRAM;
   unsigned availRAMs;
};

typedef void (__fastcall *VOID_FUNC)(void);
typedef void (__fastcall *RENDER_FUNC)(unsigned char*,unsigned);

struct BORDSIZE
{
   const char *name;
   const u32 x;
   const u32 y;
   const u32 xsize;
   const u32 ysize;
};

struct RENDER
{
   const char *name;
   RENDER_FUNC func;
   const char *nick;
   unsigned flags;
};

struct IDE_CONFIG
{
   char image[512];
   unsigned c,h,s,lba;
   unsigned char readonly;
   u8 cd;
};

enum RSM_MODE { RSM_SIMPLE, RSM_FIR0, RSM_FIR1, RSM_FIR2 };

enum SSHOT_FORMAT { SS_SCR = 0, SS_BMP = 1, SS_PNG = 2, SS_GIF = 3, SS_LAST };

struct zxkeymap ;

struct CONFIG
{
   unsigned paper;  // start of paper
   unsigned t_line; // t-states per line
   unsigned frame;  // t-states per frame
   unsigned intfq;  // usually 50Hz
   unsigned intlen; // length of INT signal (for Z80)
   unsigned nopaper;// hide paper

   unsigned render, driver, fontsize;

   unsigned soundbuffer, refresh;

   unsigned char flashcolor, noflic, fast_sl, alt_nf;
   unsigned char frameskip, frameskipmax;
   unsigned char flip, fullscr;

   unsigned char lockmouse;
   unsigned char detect_video;
   unsigned char tape_traps;
   unsigned char tape_autostart;
   SSHOT_FORMAT scrshot;
   char scrshot_path[FILENAME_MAX];

   unsigned char ch_size;
   unsigned char EFF7_mask;
   unsigned char reset_rom;
   unsigned char use_romset;

   unsigned char updateb, bordersize;
   unsigned framex;
   unsigned framexsize;
   unsigned framey;
   unsigned frameysize;
   unsigned char even_M1, border_4T;

   unsigned char floatbus, floatdos;
   bool portff;

   int modem_port; //, modem_scheme;
   unsigned char fdd_noise;

   unsigned char trdos_present, trdos_interleave;
   unsigned char trdos_traps, wd93_nodelay;
   unsigned char trdos_wp[4];

   unsigned char cache;
   unsigned char cmos;
   unsigned char smuc;
   unsigned char ula_preset;

   unsigned char gs_type;
   unsigned char pixelscroll;
   unsigned char sleepidle;
   unsigned char rsrvd1_;
   unsigned char ConfirmExit;

   unsigned char highpriority;
   unsigned char videoscale;

   MEM_MODEL mem_model;
   unsigned ramsize, romsize;

   IDE_SCHEME ide_scheme;
   IDE_CONFIG ide[2];
   unsigned char ide_skip_real;
   unsigned char cd_aspi;

   unsigned char soundfilter; //Alone Coder (IDC_SOUNDFILTER)
   unsigned char RejectDC;
   struct
   {
      unsigned fq, ayfq, saa1099fq;
      int covoxFB, covoxDD, sd, saa1099;
      int beeper_vol, micout_vol, micin_vol, ay_vol, aydig_vol,
          covoxFB_vol, covoxDD_vol, sd_vol, gs_vol, bass_vol;
      VOID_FUNC do_sound;
      unsigned char enabled, gsreset, dsprimary;
      unsigned char ay_chip, ay_scheme, ay_stereo, ay_vols, ay_samples;
      unsigned ay_stereo_tab[6], ay_voltab[32];
   } sound;

   struct {
      unsigned firenum;
      unsigned char altlock, fire, firedelay;
      unsigned char paste_hold, paste_release, paste_newline;
      unsigned char mouse, mouseswap, kjoy, keymatrix, joymouse;
      unsigned char keybpcmode;
      signed char mousescale;
      unsigned char mousewheel; // enum MOUSE_WHEEL_MODE //0.36.6 from 0.35b2
      zxkeymap *active_zxk;
      unsigned JoyId;
   } input;

   struct {
      unsigned char enabled;
      unsigned char flash_ay_kbd;
      unsigned char perf_t;
      unsigned char reserved1;
      unsigned bandBpp;
      #define NUM_LEDS 7
      unsigned ay;
      unsigned perf;
      unsigned load;
      unsigned input;
      unsigned time;
      unsigned osw;
      unsigned memband;
   } led;

   struct {
      unsigned char mem_swap;
      unsigned char xt_kbd;
      unsigned char reserved1;
   } atm;

   unsigned char use_comp_pal;
   unsigned pal, num_pals;      // selected palette and total number of pals
   unsigned minres;             // min. screen x-resolution
   unsigned scanbright;         // scanlines intensity

   struct {
      unsigned char mix_frames;
      unsigned char mode; // RSM_MODE
   } rsm;

   char sos_rom_path[FILENAME_MAX];
   char dos_rom_path[FILENAME_MAX];
   char zx128_rom_path[FILENAME_MAX];
   char sys_rom_path[FILENAME_MAX];
   char atm1_rom_path[FILENAME_MAX];
   char atm2_rom_path[FILENAME_MAX];
   char atm3_rom_path[FILENAME_MAX];
   char scorp_rom_path[FILENAME_MAX];
   char prof_rom_path[FILENAME_MAX];
   char profi_rom_path[FILENAME_MAX];
//[vv]   char kay_rom_path[FILENAME_MAX];
   char plus3_rom_path[FILENAME_MAX];
   char quorum_rom_path[FILENAME_MAX];
   char tsl_rom_path[FILENAME_MAX];

   #ifdef MOD_GSZ80
   unsigned gs_ramsize;
   char gs_rom_path[FILENAME_MAX];
   #endif

   #ifdef MOD_MONITOR
   char sos_labels_path[FILENAME_MAX];
   #endif

   char ngs_sd_card_path[FILENAME_MAX];

   unsigned char zc;
   char zc_sd_card_path[FILENAME_MAX];

   char atariset[64]; // preset for atari mode
   char zxkeymap[64]; // name of ZX keys map
   char keyset[64]; // short name of keyboard layout
   char appendboot[FILENAME_MAX];
   char workdir[FILENAME_MAX];
   u8 profi_monochrome;
};

struct TEMP
{
   unsigned rflags;    // render_func flags
   unsigned border_add, border_and;   // for scorpion 4T border update
   unsigned char *base, *base_2;  // pointers to Spectrum screen memory
   unsigned char rom_mask, ram_mask;
   unsigned char evenM1_C0; // C0 for scorpion, 00 for pentagon
   unsigned char hi15; // 0 - 16bit color, 1 - 15bit color, 2 - YUY2 colorspace
   unsigned snd_frame_samples;  // samples / frame
   unsigned snd_frame_ticks;    // sound ticks / frame
   unsigned cpu_t_at_frame_start;

   unsigned gx, gy, gdx, gdy; // updating rectangle (used by GDI renderer)
   RECT client;               // updating rectangle (used by DD_blt renderer)
   bool Minimized;            // window is minimized
   HDC gdidc;
   unsigned ox, oy, obpp, ofq; // destination video format
   unsigned scx, scy; // multicolor area (320x240 or 384x300), used in MCR renderer
   unsigned odx, ody; // offset to border in surface, used only in flip()
   unsigned rsx, rsy; // screen resolution
   unsigned b_top, b_left, b_right, b_bottom; // border frame used to setup MCR
   unsigned scale; // window scale (x1, x2, x3, x4)
   unsigned mon_scale; // window scale in monitor mode(debugger)

   u8 tspal_8[256];		// TS palette LUT for 8 bit mode
   u16 tspal_16[256];	// TS palette LUT for 16 bit mode
   u32 tspal_32[256];	// TS palette LUT for 32 bit mode
   
   u8 fm_tmp;			// temporary reg for FMAPS writes
   
   unsigned ataricolors[0x100];
   unsigned shift_mask; // for 16/32 bit modes masks low bits of color components

   struct { // led coords
      unsigned char *ay;
      unsigned char *perf;
      unsigned char *load;
      unsigned char *input;
      unsigned char *time;
      unsigned char *osw;
      unsigned char *memband;
      unsigned char *fdd;

      __int64 tape_started;
   } led;
   unsigned char profrom_mask;
   unsigned char comp_pal_changed;

   // CPU features
   unsigned char mmx, sse, sse2;
   u64 cpufq;        // x86 t-states per second
   unsigned ticks_frame; // x86 t-states in frame

   unsigned char vidblock, sndblock, inputblock, frameskip;
   bool Gdiplus;
   unsigned gsdmaaddr;
   u8 gsdmaon;
   u8 gs_ram_mask;

   u8 offset_vscroll;
   u8 offset_vscroll_prev;
   u8 offset_hscroll;
   u8 offset_hscroll_prev;

   char RomDir[FILENAME_MAX];
   char SnapDir[FILENAME_MAX];
   char HddDir[FILENAME_MAX];
};

extern TEMP temp;
extern unsigned sb_start_frame;

enum AY_SCHEME
{
   AY_SCHEME_NONE = 0,
   AY_SCHEME_SINGLE,
   AY_SCHEME_PSEUDO,
   AY_SCHEME_QUADRO,
   AY_SCHEME_POS,
   AY_SCHEME_CHRV,
   AY_SCHEME_MAX
};

#include "wd93.h"
#include "hddio.h"
#include "hdd.h"
#include "input.h"
#include "modem.h"

#if defined(MOD_GSZ80) || defined(MOD_GSBASS)
#include "bass.h"
#include "snd_bass.h"
#endif

#ifdef MOD_GSBASS
#include "gshlbass.h"
#include "gshle.h"
#endif

#define EFF7_4BPP       0x01
#define EFF7_512        0x02
#define EFF7_LOCKMEM    0x04
#define EFF7_ROCACHE    0x08
#define EFF7_GIGASCREEN 0x10
#define EFF7_HWMC       0x20
#define EFF7_384        0x40
#define EFF7_CMOS       0x80

// Биты порта 00 для кворума
static const u8 Q_F_RAM = 0x01;
static const u8 Q_RAM_8 = 0x08;
static const u8 Q_B_ROM = 0x20;
static const u8 Q_BLK_WR = 0x40;
static const u8 Q_TR_DOS = 0x80;

enum SNAP
{
   snNOFILE, snUNKNOWN, snTOOLARGE,
   snSP, snZ80, snSNA_48, snSNA_128,
   snTAP, snTZX, snCSW,
   snHOB, snSCL, snTRD, snFDI, snTD0, snUDI, snISD, snPRO
};

struct NVRAM
{
   enum EEPROM_STATE { IDLE = 0, RCV_CMD, RCV_ADDR, RCV_DATA, SEND_DATA, RD_ACK };
   unsigned address;
   unsigned char datain, dataout, bitsin, bitsout;
   unsigned char state;
   unsigned char prev;
   unsigned char out;
   unsigned char out_z;

   void write(unsigned char val);
};

struct COMPUTER
{
   unsigned char p7FFD, pFE, pEFF7, pXXXX;
   unsigned char pDFFD, pFDFD, p1FFD, pFF77;
   TS_t ts;
   u8 p7EFD; // gmx
   u8 p00, p80FD; // quorum
   __int64 t_states; // inc with conf.frame by each frame
   unsigned frame_counter; // inc each frame
   unsigned char aFE, aFB; // ATM 4.50 system ports
   unsigned pFFF7[8]; // ATM 7.10 / ATM3(4Mb) memory map
   // |7ffd|rom|b7b6|b5..b0| b7b6 = 0 for atm2

   u8 wd_shadow[4]; // 2F, 4F, 6F, 8F

   unsigned aFF77;
   unsigned active_ay;
   u8 pBF; // ATM3
   u8 pBE; // ATM3

   unsigned char flags;
   unsigned char border_attr;
   unsigned char cmos_addr;
   unsigned char pVD;

   #ifdef MOD_VID_VD
   unsigned char *vdbase;
   #endif

   unsigned char pFFBA, p7FBA; // SMUC
   unsigned char res1, res2;

   unsigned char p0F, p1F, p4F, p5F; // soundrive
   struct WD1793 wd;
   struct NVRAM nvram;
   struct {
      __int64 edge_change;
      unsigned char *play_pointer; // or NULL if tape stopped
      unsigned char *end_of_tape;  // where to stop tape
      unsigned index;    // current tape block
      unsigned tape_bit;
//      SNDRENDER sound; //Alone Coder
   } tape;
   SNDRENDER tape_sound; //Alone Coder
   unsigned char comp_pal[0x10];
   unsigned char ide_hi_byte_r, ide_hi_byte_w, ide_hi_byte_w1, ide_read, ide_write; // high byte in IDE i/o
   unsigned char profrom_bank;
};

// bits for COMPUTER::flags
#define CF_DOSPORTS     0x01    // tr-dos ports are accessible
#define CF_TRDOS        0x02    // DOSEN trigger
#define CF_SETDOSROM    0x04    // tr-dos ROM become active at #3Dxx
#define CF_LEAVEDOSRAM  0x08    // DOS ROM will be closed at executing RAM
#define CF_LEAVEDOSADR  0x10    // DOS ROM will be closed at pc>#4000
#define CF_CACHEON      0x20    // cache active
#define CF_Z80FBUS      0x40    // unstable data bus
#define CF_PROFROM      0x80    // PROF-ROM active

#define TAPE_QUANTUM 64
struct tzx_block
{
   unsigned char *data;
   unsigned datasize;    // data size, in bytes
   unsigned pause;
   union {
      struct {
         unsigned pilot_t, s1_t, s2_t, zero_t, one_t;
         unsigned pilot_len;
      };
      struct {
         unsigned numblocks, numpulses;
      };
      unsigned param;
   };
   unsigned char type; // 0-playable, 1-pulses, 10-20 - info, etc...
   unsigned char crc; // xor of all bytes
   char desc[128];
};

struct SNDVAL {
   union {
      unsigned data;
      struct {
         short left, right;
      };
   };
};

struct virtkeyt {
   const char *name;
   unsigned short virtkey;
};

struct keyports
{
  volatile unsigned char *port1, *port2;
  unsigned char mask1, mask2;
};

struct zxkey {
  const char *name;
  volatile unsigned char * /*const*/ port; //Alone Coder
  /*const*/ unsigned char mask; //Alone Coder
};

struct zxkeymap {
  const char *name;
  zxkey *zxk;
  unsigned zxk_size ;
} ;

struct action {
   const char *name;
   void (*func)();
   unsigned short k1, k2, k3, k4;
};

typedef void (*TColorConverter)(u8 *dst, u8 *scrbuf, int dx);
void ConvBgr32ToBgr24(u8 *dst, u8 *scrbuf, int dx);
void ConvYuy2ToBgr24(u8 *dst, u8 *scrbuf, int dx);
void ConvRgb16ToBgr24(u8 *dst, u8 *scrbuf, int dx);
void ConvRgb15ToBgr24(u8 *dst, u8 *scrbuf, int dx);
void ConvPal8ToBgr24(u8 *dst, u8 *scrbuf, int dx);
extern TColorConverter ConvBgr24;
// flags for video filters
                                // misc options
#define RF_BORDER   0x00000002   // no multicolor painter, read directly from spectrum memory
#define RF_MON      0x00000004   // not-flippable surface, show mouse cursor
#define RF_DRIVER   0x00000008   // use options from driver
//#define RF_VSYNC    0x00000010   // force VSYNC
#define RF_D3D      0x00000010   // use d3d for windowed mode
#define RF_GDI      0x00000020   // output to window
#define RF_CLIP     0x00000040   // use DirectDraw clipper for windowed mode
#define RF_OVR      0x00000080   // output to overlay

                                // main area size
#define RF_1X       0x00000000   // 256x192,320x240 with border (default)
#define RF_2X       0x00000100   // default x2
#define RF_3X       0x00000001   // default x3
#define RF_4X       0x00000200   // default x4
#define RF_64x48    0x00000400   // 64x48  (for chunky 4x4)
#define RF_128x96   0x00000800   // 128x96 (for chunky 2x2)

#define RF_8        0x00000000   // 8 bit (default)
#define RF_8BPCH    0x00001000   // 8-bit per color channel. GDI mode => 32-bit surface. 8-bit mode => grayscale palette
#define RF_YUY2     0x00002000   // pixel format: 16bit, YUY2
#define RF_16       0x00004000   // hicolor mode (RGB555/RGB565)
#define RF_32       0x00008000   // true color

#define RF_USEC32   0x00010000  // use c32tab
#define RF_USE32AS16 0x0020000  // c32tab contain hi-color WORDS
#define RF_USEFONT  0x00040000  // use font_tables
#define RF_PALB     0x00080000  // palette for bilinear filter
#define RF_COMPPAL  0x00100000  // use palette from ATM palette registers
#define RF_GRAY     0x00200000  // grayscale palette

#define RF_MONITOR (RF_MON | RF_GDI | RF_2X)

extern unsigned frametime;
extern int nmi_pending;

bool ConfirmExit();
BOOL WINAPI ConsoleHandler(DWORD CtrlType);
void showhelp(const char *anchor = 0);
