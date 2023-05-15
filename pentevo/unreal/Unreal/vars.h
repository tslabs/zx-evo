#pragma once
#include "emul.h"
#include "input.h"
#include "sound/sndchip.h"
#include "sound/ayx32.h"
#include "emulator/debugger/debug.h"
#include "hard/zf232.h"
#include "hard/cpu/z80main.h"
#include "hard/gs/gshle.h"
#include "hard/hdd/hdd.h"

constexpr auto romled_time = 16;

constexpr auto snd_buf_sz = (4 * 1048576); // large temporary buffer (for reading snapshots);
constexpr auto gdi_buf_sz = (448 * 320 * 4 * 4 * 4);	// Quad size, 32 bit, 448x320 max;
#define DBG_GDIBUFSZ (DEBUG_WND_WIDTH*DEBUG_WND_HEIGHT)	// Quad size, 32 bit, 448x320 max

#pragma pack(8)
struct palette_options
{ // custom palettes
	char name[33];
	unsigned ZZ, ZN, NN, NB, BB, ZB;
	unsigned r11, r12, r13, r21, r22, r23, r31, r32, r33;
};
#pragma pack()

enum
{
	VK_LMB = 0x101,
	VK_RMB,
	VK_MMB,
	VK_MWU,
	VK_MWD,
	VK_JLEFT,
	VK_JRIGHT,
	VK_JUP,
	VK_JDOWN,
	VK_JFIRE,
	VK_JB0 = VK_JFIRE,
	VK_JB1,
	VK_JB2,
	VK_JB3,
	VK_JB4,
	VK_JB5,
	VK_JB6,
	VK_JB7,
	VK_JB8,
	VK_JB9,
	VK_JB10,
	VK_JB11,
	VK_JB12,
	VK_JB13,
	VK_JB14,
	VK_JB15,
	VK_JB16,
	VK_JB17,
	VK_JB18,
	VK_JB19,
	VK_JB20,
	VK_JB21,
	VK_JB22,
	VK_JB23,
	VK_JB24,
	VK_JB25,
	VK_JB26,
	VK_JB27,
	VK_JB28,
	VK_JB29,
	VK_JB30,
	VK_JB31,
	DIK_MENU,
	DIK_CONTROL,
	DIK_SHIFT,
	VK_MAX
};

enum
{
	MEMBITS_R = 0x01,
	MEMBITS_W = 0x02,
	MEMBITS_X = 0x04,
	MEMBITS_BPR = 0x10,
	MEMBITS_BPW = 0x20,
	MEMBITS_BPX = 0x40,
	MEMBITS_BPC = 0x80
};

struct gdibmp_t
{
	BITMAPINFO header;
	RGBQUAD waste[0x100];
};



enum class BANKM
{
	BANKM_ROM = 0,
	BANKM_RAM
};

extern palette_options pals[32];

extern CONFIG conf;
extern COMPUTER comp;
extern u8 memory[];

extern u8 cmos[0x100];
extern u8 nvram[0x800];

extern char ininame[0x200];
extern char helpname[0x200];

extern unsigned num_ula;
extern char* ulapreset[64];
extern char* setptr;

extern char* aystereo[64];
extern char* ayvols[64];
extern unsigned num_ayvols;
extern unsigned num_aystereo;

extern DWORD WinVerMajor;
extern DWORD WinVerMinor;

extern HWND wnd;
extern HWND dlg;
extern HINSTANCE hIn;
extern HWND debug_wnd;
extern unsigned nowait;

extern action ac_main[];
extern action ac_mon[];
extern action ac_regs[];
extern action ac_trace[];
extern action ac_mem[];
extern action ac_banks[];
extern RENDER renders[];
extern BORDSIZE bordersizes[];
extern VOID_FUNC prebuffers[];
extern TS_VDAC_NAME ts_vdac_names[];

extern const mem_model_t mem_model[N_MM_MODELS];

extern zxkeymap zxk_maps[];
extern const size_t zxk_maps_count;

extern virtkeyt pckeys[];
extern const size_t pckeys_count;

extern keyports inports[VK_MAX];

extern int trd_toload; // drive to load
extern unsigned default_drive; // Дисковод по умолчанию в который грузятся образы дисков при старте

extern u8* base_sos_rom;
extern u8* base_dos_rom;
extern u8* base_128_rom;
extern u8* base_sys_rom;

extern zf232_t zf232;
extern k_input input;

extern unsigned brk_port_in;
extern unsigned brk_port_out;
extern u8 brk_port_val;
extern unsigned brk_mem_rd;
extern unsigned brk_mem_wr;
extern u8 brk_mem_val;

extern unsigned watch_script[4][64];
extern u8 watch_enabled[4];
extern u8 used_banks[MAX_PAGES];
extern u8 trace_rom;
extern u8 trace_ram;

extern u8 dbgbreak;
extern u8 snbuf[snd_buf_sz];		// large temporary buffer (for reading snapshots)
extern u8 gdibuf[gdi_buf_sz];
extern u8 debug_gdibuf[DBG_GDIBUFSZ];

extern SNDCHIP ay[2];
extern SNDAYX32 ayx32;

extern u8* bankr[4];
extern u8* bankw[4];
extern BANKM bankm[4];		// bank mode: 0 - ROM / 1 - RAM

extern GSHLE gs;

extern gdibmp_t gdibmp, debug_gdibmp;
extern u8 needclr; // clear screenbuffer before rendering
extern DWORD mousepos;  // left-clicked point in monitor
extern PALETTEENTRY syspalette[0x100];
extern u8 exitflag; // avoid call terminate() twice

constexpr auto PLAYBUFSIZE = 16384;
extern unsigned sndplaybuf[PLAYBUFSIZE];
extern unsigned spbsize;
extern u8 savesndtype; // 0-none,1-wave,2-vtx
extern FILE* savesnd;

extern HBITMAP hbm;  // bitmap for repaint background
extern DWORD bm_dx;
extern DWORD bm_dy;

extern char droppedFile[512];

extern char statusline[128];
extern unsigned statcnt;

extern bool normal_exit;

extern const char* const ay_schemes[];

extern ata_port hdd;   // not in `comp' - not cleared in reset()
extern char arcbuffer[0x2000]; // extensions and command lines for archivers
extern char skiparc[0x400]; // ignore this files in archive
extern char trd_loaded[4]; // used to get first free drive with no account of autoloaded images
extern u8 kbdpc[VK_MAX]; // add cells for mouse & joystick

extern char pressedit; //Alone Coder
extern int fmsoundon0; //Alone Coder
extern int tfmstatuson0; //Alone Coder

// for leds
extern u8 trdos_load;
extern u8 trdos_save;
extern u8 trdos_format;
extern u8 trdos_seek;
extern u8 membits[0x10000];

extern u8* vtxbuf;
extern unsigned vtxbufsize;
extern unsigned vtxbuffilled;

extern unsigned snapsize;

extern u8 kbdpcEX[6]; //Dexus
