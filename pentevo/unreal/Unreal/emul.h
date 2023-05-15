#pragma once

#include <stdio.h>
#include "sysdefs.h"

#include "sound/sndrender.h"
#include "hard/tsconf.h"
#include "savevid.h"

#define EMUL_DEBUG

constexpr unsigned PAGE = 0x4000U;
constexpr auto MAX_RAM_PAGES = 256;       // 4Mb RAM;
constexpr auto MAX_CACHE_PAGES = 2;       // 32K cache;
constexpr auto MAX_MISC_PAGES = 1;        // trash page;
constexpr auto MAX_ROM_PAGES = 64;        // 1Mb;

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

#define page_ram(a) (RAM_BASE_M + PAGE * (a))
#define page_rom(a) (ROM_BASE_M + PAGE * (a))

#define RAM_BASE_M  memory
#define CACHE_M     (RAM_BASE_M + MAX_RAM_PAGES*PAGE)
#define MISC_BASE_M (CACHE_M + MAX_CACHE_PAGES*PAGE)
#define ROM_BASE_M  (MISC_BASE_M + MAX_MISC_PAGES*PAGE)

#ifdef MOD_GSZ80
#define ROM_GS_M    (ROM_BASE_M + MAX_ROM_PAGES*PAGE)
#define GSRAM_M     (ROM_GS_M + MAX_GSROM_PAGES*PAGE)
#endif

#define TRASH_M     (MISC_BASE_M+0*PAGE)

extern bool Exit;

enum class ide_scheme
{
	none = 0,
	nemo = 1,
	nemo_a8 = 2
};

enum class mouse_wheel_mode
{
	none,
	keyboard,
	kempston
}; //0.36.6 from 0.35b2

enum MEM_MODEL
{
	MM_PENTAGON = 0,
	MM_TSL,
	N_MM_MODELS
};

enum class ray_paint_mode
{
	overdraw = 0,
	clear,
	dim
};

enum class rom_mode
{
	nochange = 0,
	sos,
	dos,
	sys,
	s128,
	cache
};

constexpr int RAM_128 = 128, RAM_256 = 256, RAM_512 = 512, RAM_1024 = 1024, RAM_2048 = 2048, RAM_4096 = 4096;

struct mem_model_t
{
	const char* fullname, * shortname;
	MEM_MODEL Model;
	unsigned defaultRAM;
	unsigned availRAMs;
};

using VOID_FUNC = void(*)();

struct BORDSIZE
{
	const char* name;
	const u32 x;
	const u32 y;
	const u32 xsize;
	const u32 ysize;
};

using render_func = void(*)(u8*, u32);
struct RENDER
{
	const char* name;	// displayed on config GUI
	render_func func;	// called function
	const char* nick;	// used in *.ini
	unsigned flags;		// used 
};

using driver_func = void(*)();
struct DRIVER
{
	const char* name;	// displayed on config GUI
	driver_func func;	// called function
	const char* nick;	// used in *.ini
	unsigned flags;		// used 
};

using drawer_func = void(*)(int);
struct DRAWER
{
	drawer_func func;
};

struct IDE_CONFIG
{
	char image[512];
	unsigned c, h, s, lba;
	u8 readonly;
	u8 cd;
};

enum class rsm_mode
{
	simple, fir0, fir1, fir2
};

enum class sshot_format
{
	scr = 0, bmp = 1, png = 2, gif = 3, max_value
};

enum class ulaplus
{
	type1 = 0,
	type2,
	none
};

enum class ay_scheme
{
	none = 0,
	single,
	pseudo,
	quadro,
	pos,
	chrv,
	ayx32,
	max_value
};

struct zxkeymap;

struct CONFIG
{
	unsigned t_line; // t-states per line
	unsigned frame;  // t-states per frame
	unsigned intfq;  // usually 50Hz
	unsigned intstart;	// int start
	unsigned intlen; // length of INT signal (for Z80)
	unsigned nopaper;// hide paper

	unsigned render, driver, fontsize;

	unsigned soundbuffer, refresh;

	u8 flashcolor, noflic, fast_sl, alt_nf;
	u8 frameskip, frameskipmax;
	u8 flip, fullscr;

	u8 lockmouse;
	u8 detect_video;
	u8 tape_traps;
	ulaplus ulaplus;
	u8 tape_autostart;
	sshot_format scrshot;
	char scrshot_path[FILENAME_MAX];

	u8 ch_size;
	u8 EFF7_mask;
	rom_mode reset_rom;
	u8 use_romset;
	u8 spg_mem_init;

	u8 updateb, bordersize;
	unsigned framex;
	unsigned framexsize;
	unsigned framey;
	unsigned frameysize;
	u8 even_M1, border_4T;

	u8 floatbus, floatdos;
	bool portff;

	int modem_port; //, modem_scheme;
	int zifi_port;
	u8 fdd_noise;

	u8 trdos_present, trdos_interleave;
	u8 trdos_traps, wd93_nodelay;
	u8 trdos_wp[4];

	u8 cache;
	u8 cmos;
	u8 smuc;
	u8 ula_preset;

	u8 gs_type;
	u8 pixelscroll;
	u8 sleepidle;
	u8 rsrvd1_;
	u8 ConfirmExit;

	u8 highpriority;
	u8 videoscale;

	MEM_MODEL mem_model;
	unsigned ramsize, romsize;

	ide_scheme ide_scheme;
	IDE_CONFIG ide[2];
	u8 ide_skip_real;
	u8 cd_aspi;

	u32 sd_delay;

	u8 soundfilter; //Alone Coder (IDC_SOUNDFILTER)
	u8 RejectDC;
	struct
	{
		unsigned fq, ayfq, saa1099fq;
		int covoxFB, covoxDD, sd, saa1099, moonsound;
		int beeper_vol, micout_vol, micin_vol, ay_vol, aydig_vol, saa1099_vol;
		int covoxFB_vol, covoxDD_vol, sd_vol, covoxProfi_vol;
		int gs_vol, bass_vol, moonsound_vol;
		VOID_FUNC do_sound;
		u8 enabled, gsreset, dsprimary;
		u8 ay_chip;
		ay_scheme ay_scheme;
		u8 ay_stereo, ay_vols, ay_samples;
		unsigned ay_stereo_tab[6], ay_voltab[32];
	} sound;

	struct {
		unsigned firenum;
		u8 altlock, fire, firedelay;
		u8 paste_hold, paste_release, paste_newline;
		u8 mouse, mouseswap, kjoy, keymatrix, joymouse;
		u8 keybpcmode;
		char mousescale;
		mouse_wheel_mode mousewheel; // enum MOUSE_WHEEL_MODE //0.36.6 from 0.35b2
		zxkeymap* active_zxk;
		unsigned JoyId;
	} input;

	struct {
		u8 enabled;
		u8 status;
		u8 flash_ay_kbd;
		u8 perf_t;
		u8 reserved1;
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

	u8 use_comp_pal;
	unsigned pal, num_pals;      // selected palette and total number of pals
	unsigned minres;             // min. screen x-resolution
	unsigned scanbright;         // scanlines intensity

	ray_paint_mode ray_paint_mode;     // 0 - overdraw prev frame, 1 - clear frame, 2 - mix gray and old frame

	struct {
		u8 mix_frames;
		rsm_mode mode; // RSM_MODE
	} rsm;

	char sos_rom_path[FILENAME_MAX];
	char dos_rom_path[FILENAME_MAX];
	char zx128_rom_path[FILENAME_MAX];
	char sys_rom_path[FILENAME_MAX];
	char pent_rom_path[FILENAME_MAX];
	char tsl_rom_path[FILENAME_MAX];

#ifdef MOD_GSZ80
	unsigned gs_ramsize;
	char gs_rom_path[FILENAME_MAX];
#endif

	char moonsound_rom_path[FILENAME_MAX];

#ifdef MOD_MONITOR
	char sos_labels_path[FILENAME_MAX];
#endif

	char ngs_sd_card_path[FILENAME_MAX];

	u8 zc;
	char zc_sd_card_path[FILENAME_MAX];

	char atariset[64]; // preset for atari mode
	char zxkeymap[64]; // name of ZX keys map
	char keyset[64]; // short name of keyboard layout
	char appendboot[FILENAME_MAX];
	char workdir[FILENAME_MAX];
	u8 profi_monochrome;

	struct {
		char exec[VS_MAX_FFPATH];  // ffmpeg path/name
		char parm[VS_MAX_FFPARM];  // enc. parameters for ffmpeg
		char vout[VS_MAX_FFVOUT];  // output video file name
		u8 newcons;                // OpenImage new console for ffmpeg
	} ffmpeg;
};

struct TEMP
{
	unsigned rflags, mon_rflags;    // render_func flags
	unsigned border_add, border_and;   // for scorpion 4T border update
	u8* base, * base_2;  // pointers to Spectrum screen memory
	u8 rom_mask, ram_mask;
	u8 evenM1_C0; // C0 for scorpion, 00 for pentagon
	u8 hi15; // 0 - 16bit color, 1 - 15bit color, 2 - YUY2 colorspace
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

	u8 fm_tmp;			// temporary reg for FMAPS writes

	unsigned ataricolors[0x100];
	unsigned shift_mask; // for 16/32 bit modes masks low bits of color components

	struct { // led coords
		u32* ay;
		u32* perf;
		u32* load;
		u32* input;
		u32* time;
		u32* osw;
		u32* memband;
		u32* fdd;

		__int64 tape_started;
	} led;
	u8 profrom_mask;
	u8 comp_pal_changed;

	// CPU features
	u8 mmx, sse, sse2;
	u64 cpufq;        // x86 t-states per second
	unsigned ticks_frame; // x86 t-states in frame

	u8 vidblock, sndblock, inputblock, frameskip;
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

#include "hard/fdd/wd93.h"
#include "hard/hdd/hddio.h"

#define EFF7_4BPP       0x01
#define EFF7_512        0x02
#define EFF7_LOCKMEM    0x04
#define EFF7_ROCACHE    0x08
#define EFF7_GIGASCREEN 0x10	// It is also Turbo bit0 at ATM3 (tsl)
#define EFF7_HWMC       0x20
#define EFF7_384        0x40
#define EFF7_CMOS       0x80

#define aFE_16          0x00
#define aFE_MC          0x01

enum SNAP
{
	snNOFILE, snUNKNOWN, snTOOLARGE,
	snSP, snZ80, snSNA_48, snSNA_128, snSPG,
	snTAP, snTZX, snCSW,
	snHOB, snSCL, snTRD, snFDI, snTD0, snUDI, snISD, snPRO
};

struct NVRAM
{
	enum EEPROM_STATE { IDLE = 0, RCV_CMD, RCV_ADDR, RCV_DATA, SEND_DATA, RD_ACK };
	unsigned address;
	u8 datain, dataout, bitsin, bitsout;
	u8 state;
	u8 prev;
	u8 out;
	u8 out_z;

	void write(u8 val);
};

struct COMPUTER
{
	u8 p7FFD, pFE, pEFF7, pXXXX;
	TSPORTS_t ts;
	u16 cram[256];
	u16 sfile[256];
	__int64 t_states; // inc with conf.frame by each frame
	unsigned frame_counter; // inc each frame

	unsigned active_ay;

	u8 flags;
	u8 border_attr;
	u8 cmos_addr;
	u8 pVD;


	u8 pFFBA, p7FBA; // SMUC
	u8 res1, res2;

	u8 p0F, p1F, p4F, p5F; // soundrive
	WD1793 wd;
	NVRAM nvram;
	struct {
		__int64 edge_change;
		u8* play_pointer; // or NULL if tape stopped
		u8* end_of_tape;  // where to stop tape
		unsigned index;    // current tape block
		unsigned tape_bit;
		//      SNDRENDER sound; //Alone Coder
	} tape;
	SNDRENDER tape_sound; //Alone Coder
	u8 comp_pal[0x10];
	u8 ulaplus_cram[64];
	u8 ulaplus_mode;
	u8 ulaplus_reg;
	u8 ide_hi_byte_r, ide_hi_byte_w, ide_hi_byte_w1, ide_read, ide_write; // high byte in IDE i/o
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

// LSY256 - BarmaleyM's Orel' extension
#define PF_DV0			0x01	// RAM r/w at #0000 page selector
#define PF_BLKROM		0x02	// RAM/!ROM at #0000
#define PF_EMUL			0x08	// page mode at #0000
#define PF_PA3			0x10	// page bit3 at #C000

#define TAPE_QUANTUM 64
struct tzx_block
{
	u8* data;
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
	u8 type; // 0-playable, 1-pulses, 10-20 - info, etc...
	u8 crc; // xor of all bytes
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
	const char* name;
	u16 di_key, virtkey;
};

struct keyports
{
	volatile u8* port1, * port2;
	u8 mask1, mask2;
};

struct zxkey {
	const char* name;
	volatile u8* /*const*/ port; //Alone Coder
	/*const*/ u8 mask; //Alone Coder
};

struct zxkeymap {
	const char* name;
	zxkey* zxk;
	unsigned zxk_size;
};

struct action {
	const char* name;
	void (*func)();
	u16 k1, k2, k3, k4;
};

typedef void (*TColorConverter)(u8* dst, u8* scrbuf, int dx);
void ConvBgr32ToBgr24(u8* dst, u8* scrbuf, int dx);
void ConvYuy2ToBgr24(u8* dst, u8* scrbuf, int dx);
void ConvRgb16ToBgr24(u8* dst, u8* scrbuf, int dx);
void ConvRgb15ToBgr24(u8* dst, u8* scrbuf, int dx);
void ConvPal8ToBgr24(u8* dst, u8* scrbuf, int dx);
extern TColorConverter ConvBgr24;

// Guys, I REALLY HATE those jerks who made all that mess I'm about to re-factor.
// Now and today I'm starting this heroic challenge.
// TSL
// 17.10.2013

// video flippers



// video overlay 

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
#define RF_GRAY     0x00200000  // grayscale palette

#define RF_MONITOR (RF_MON | RF_GDI | RF_2X)

extern unsigned frametime;
extern int nmi_pending;

bool ConfirmExit();
BOOL WINAPI ConsoleHandler(DWORD CtrlType);
void showhelp(const char* anchor = 0);
