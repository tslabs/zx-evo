#include "std.h"
#include "emulator/z80/z80.h"
#include "emul.h"
#include "vars.h"
#include "emulator/debugger/debug.h"
#include "hard/memory.h"
#include "hard/gs/gsz80.h"
#include "hard/cpu/z80main.h"

#include "util.h"
#include "emulator/debugger/dbgoth.h"
#include "hard/cpu/z80main_fn.h"
#include "sound/sndcounter.h"
#include "sound/ayx32.h"

int fmsoundon0 = 4; //Alone Coder
int tfmstatuson0 = 2; //Alone Coder
char pressedit = 0; //Alone Coder


#pragma pack(8)


t_cpu_mgr cpu_mgr;

void t_cpu_mgr::switch_cpu()
{
	current_cpu_++;
	current_cpu_ %= Count;
}

Z80* t_cpu_mgr::cpus_[] =
{
  &cpu,
  &z80gs::gscpu
};


const unsigned t_cpu_mgr::Count = _countof(cpus_);
z80_state_t t_cpu_mgr::prev_cpus_[t_cpu_mgr::Count];
unsigned t_cpu_mgr::current_cpu_ = 0;

#ifdef MOD_GSBASS
GSHLE gs;
#endif

u8 dbgbreak = 0;


CONFIG conf;
COMPUTER comp;
TEMP temp;
ata_port hdd;   // not in `comp' - not cleared in reset()
k_input input;
zf232_t zf232;

SNDRENDER sound;
SNDCHIP ay[2];
SNDAYX32 ayx32;
SNDCOUNTER sndcounter;

u8* base_sos_rom, * base_dos_rom, * base_128_rom, * base_sys_rom;

#ifdef CACHE_ALIGNED
ATTR_ALIGN(4096)
u8 memory[PAGE * MAX_PAGES];
#else // __declspec(align) not available, force u64 align with old method
__int64 memory__[PAGE * MAX_PAGES / sizeof(__int64)];
u8* const memory = (u8*)memory__;
#endif


u8 membits[0x10000];
u8* bankr[4];	// memory pointers to memory (RAM/ROM/cache) mapped in four Z80 windows
u8* bankw[4];
BANKM bankm[4];		// bank mode: 0 - ROM / 1 - RAM
u8 cmos[0x100];
u8 nvram[0x800];

unsigned sndplaybuf[PLAYBUFSIZE];
unsigned spbsize;

FILE* savesnd;
u8 savesndtype; // 0-none,1-wave,2-vtx
u8* vtxbuf; unsigned vtxbufsize, vtxbuffilled;

u8 trdos_load, trdos_save, trdos_format, trdos_seek; // for leds
u8 needclr; // clear screenbuffer before rendering

HWND wnd; HINSTANCE hIn;
HWND debug_wnd;

char droppedFile[512];

const mem_model_t mem_model[N_MM_MODELS] =
{
	{ "Pentagon", "PENTAGON",                MM_PENTAGON, 128,  RAM_128 | RAM_256 | RAM_512 | RAM_1024 },
	{ "TS-Config", "TSL",                    MM_TSL, 4096, RAM_4096 },
};

u8 kbdpc[VK_MAX]; // add cells for mouse & joystick
u8 kbdpcEX[6]; //Dexus
keyports inports[VK_MAX];

char statusline[128];
unsigned statcnt;

char arcbuffer[0x2000]; // extensions and command lines for archivers
char skiparc[0x400]; // ignore this files in archive

u8 exitflag = 0; // avoid call terminate() twice

// beta128 vars
int trd_toload = 0; // drive to load
unsigned default_drive = -1; // Дисковод по умолчанию в который грузятся образы дисков при старте

char trd_loaded[4]; // used to get first free drive with no account of autoloaded images
char ininame[0x200];
char helpname[0x200];
unsigned snapsize;

// conditional breakpoints support
unsigned brk_port_in, brk_port_out;
u8 brk_port_val;
unsigned brk_mem_rd, brk_mem_wr;
u8 brk_mem_val;

virtkeyt pckeys[] =
{
   { "ESC", DIK_ESCAPE, VK_ESCAPE },
   { "F1", DIK_F1, VK_F1 }, { "F2", DIK_F2, VK_F2 }, { "F3", DIK_F3, VK_F3 },
   { "F4", DIK_F4, VK_F4 }, { "F5", DIK_F5, VK_F5 }, { "F6", DIK_F6, VK_F6 },
   { "F7", DIK_F7, VK_F7 }, { "F8", DIK_F8, VK_F8 }, { "F9", DIK_F9, VK_F9 },
   { "F10", DIK_F10, VK_F10 }, { "F11", DIK_F11, VK_F11 }, { "F12", DIK_F12, VK_F12 },
   { "PRSCR", DIK_SYSRQ, VK_PRINT }, { "SCLOCK", DIK_SCROLL, VK_SCROLL }, { "PAUSE", DIK_PAUSE, VK_PAUSE },

   { "1", DIK_1, '1' }, { "2", DIK_2, '2' }, { "3", DIK_3, '3' }, { "4", DIK_4, '4' }, { "5", DIK_5, '5' },
   { "6", DIK_6, '6' }, { "7", DIK_7, '7' }, { "8", DIK_8, '8' }, { "9", DIK_9, '9' }, { "0", DIK_0, '0' },

   { "Q", DIK_Q, 'Q' }, { "W", DIK_W, 'W' }, { "E", DIK_E, 'E' }, { "R", DIK_R, 'R' }, { "T", DIK_T, 'T' },
   { "Y", DIK_Y, 'Y' }, { "U", DIK_U, 'U' }, { "I", DIK_I, 'I' }, { "O", DIK_O, 'O' }, { "P", DIK_P, 'P' },
   { "A", DIK_A, 'A' }, { "S", DIK_S, 'S' }, { "D", DIK_D, 'D' }, { "F", DIK_F, 'F' }, { "G", DIK_G, 'G' },
   { "H", DIK_H, 'H' }, { "J", DIK_J, 'J' }, { "K", DIK_K, 'K' }, { "L", DIK_L, 'L' },
   { "Z", DIK_Z, 'Z' }, { "X", DIK_X, 'X' }, { "C", DIK_C, 'C' }, { "V", DIK_V, 'V' }, { "B", DIK_B, 'B' },
   { "N", DIK_N, 'N' }, { "M", DIK_M, 'M' },

   { "MINUS", DIK_MINUS, VK_OEM_MINUS }, { "PLUS", DIK_EQUALS, VK_OEM_PLUS }, { "BACK", DIK_BACK, VK_BACK },
   { "TAB", DIK_TAB, VK_TAB }, { "LB", DIK_LBRACKET, VK_OEM_4 }, { "RB", DIK_RBRACKET, VK_OEM_6 },
   { "CAPS", DIK_CAPITAL, VK_CAPITAL }, { "TIL", DIK_GRAVE, VK_OEM_3 }, { "SPACE", DIK_SPACE, VK_SPACE },
   { "COL", DIK_SEMICOLON, VK_OEM_1 }, { "QUOTE", DIK_APOSTROPHE, VK_OEM_7 }, { "ENTER", DIK_RETURN, VK_RETURN },
   { "COMMA", DIK_COMMA, VK_OEM_COMMA }, { "POINT", DIK_PERIOD, VK_OEM_PERIOD }, { "SLASH", DIK_SLASH, VK_OEM_2 }, { "BACKSL", DIK_BACKSLASH, VK_OEM_5 },
   { "SHIFT", DIK_SHIFT, VK_SHIFT }, { "ALT", DIK_MENU, VK_MENU }, { "CONTROL", DIK_CONTROL, VK_CONTROL },
   { "LSHIFT", DIK_LSHIFT, VK_LSHIFT }, { "LALT", DIK_LMENU, VK_LMENU }, { "LCONTROL", DIK_LCONTROL, VK_LCONTROL },
   { "RSHIFT", DIK_RSHIFT, VK_RSHIFT }, { "RALT", DIK_RMENU, VK_RMENU }, { "RCONTROL", DIK_RCONTROL, VK_RCONTROL },
   { "MENU", DIK_APPS, VK_APPS },

   { "INS", DIK_INSERT, VK_INSERT }, { "HOME", DIK_HOME, VK_HOME }, { "PGUP", DIK_PRIOR, VK_PRIOR },
   { "DEL", DIK_DELETE, VK_DELETE }, { "END", DIK_END, VK_END },   { "PGDN", DIK_NEXT, VK_NEXT },

   { "UP", DIK_UP, VK_UP }, { "DOWN", DIK_DOWN, VK_DOWN }, { "LEFT", DIK_LEFT, VK_LEFT }, { "RIGHT", DIK_RIGHT, VK_RIGHT },

   { "NUMLOCK", DIK_NUMLOCK, VK_NUMLOCK }, { "GRDIV", DIK_DIVIDE, VK_DIVIDE },
   { "GRMUL", DIK_MULTIPLY, VK_MULTIPLY }, { "GRSUB", DIK_SUBTRACT, VK_SUBTRACT }, { "GRADD", DIK_ADD, VK_ADD },
   { "GRENTER", DIK_NUMPADENTER },

   { "N0", DIK_NUMPAD0, VK_NUMPAD0 }, { "N1", DIK_NUMPAD1, VK_NUMPAD1 }, { "N2", DIK_NUMPAD2, VK_NUMPAD2 },
   { "N3", DIK_NUMPAD3, VK_NUMPAD3 }, { "N4", DIK_NUMPAD4, VK_NUMPAD4 }, { "N5", DIK_NUMPAD5, VK_NUMPAD5 },
   { "N6", DIK_NUMPAD6, VK_NUMPAD6 }, { "N7", DIK_NUMPAD7, VK_NUMPAD7 }, { "N8", DIK_NUMPAD8, VK_NUMPAD8 },
   { "N9", DIK_NUMPAD9, VK_NUMPAD9 }, { "NP", DIK_DECIMAL, VK_DECIMAL },

   { "LMB", VK_LMB, VK_LBUTTON }, { "RMB", VK_RMB, VK_RBUTTON }, { "MMB", VK_MMB, VK_MBUTTON },
   { "MWU", VK_MWU }, { "MWD", VK_MWD },

   { "JLEFT", VK_JLEFT }, { "JRIGHT", VK_JRIGHT },
   { "JUP", VK_JUP }, { "JDOWN", VK_JDOWN }, { "JFIRE", VK_JFIRE },
   { "JB0", VK_JB0 },
   { "JB1", VK_JB1 },
   { "JB2", VK_JB2 },
   { "JB3", VK_JB3 },
   { "JB4", VK_JB4 },
   { "JB5", VK_JB5 },
   { "JB6", VK_JB6 },
   { "JB7", VK_JB7 },
   { "JB8", VK_JB8 },
   { "JB9", VK_JB9 },
   { "JB10", VK_JB10 },
   { "JB11", VK_JB11 },
   { "JB12", VK_JB12 },
   { "JB13", VK_JB13 },
   { "JB14", VK_JB14 },
   { "JB15", VK_JB15 },
   { "JB16", VK_JB16 },
   { "JB17", VK_JB17 },
   { "JB18", VK_JB18 },
   { "JB19", VK_JB19 },
   { "JB20", VK_JB20 },
   { "JB21", VK_JB21 },
   { "JB22", VK_JB22 },
   { "JB23", VK_JB23 },
   { "JB24", VK_JB24 },
   { "JB25", VK_JB25 },
   { "JB26", VK_JB26 },
   { "JB27", VK_JB27 },
   { "JB28", VK_JB28 },
   { "JB29", VK_JB29 },
   { "JB30", VK_JB30 },
   { "JB31", VK_JB31 },
};
const size_t pckeys_count = _countof(pckeys);

static zxkey zxk_default[] =
{
   { "KRIGHT", &input.kjoy, ~1 },
   { "KLEFT",  &input.kjoy, ~2 },
   { "KDOWN",  &input.kjoy, ~4 },
   { "KUP",    &input.kjoy, ~8 },
   { "KFIRE",  &input.kjoy, ~16},

   { "ENT", input.kbd + 6, ~0x01 },
   { "SPC", input.kbd + 7, ~0x01 },
   { "SYM", input.kbd + 7, ~0x02 },

   { "CAP", input.kbd + 0, ~0x01 },
   { "Z",   input.kbd + 0, ~0x02 },
   { "X",   input.kbd + 0, ~0x04 },
   { "C",   input.kbd + 0, ~0x08 },
   { "V",   input.kbd + 0, ~0x10 },

   { "A",   input.kbd + 1, ~0x01 },
   { "S",   input.kbd + 1, ~0x02 },
   { "D",   input.kbd + 1, ~0x04 },
   { "F",   input.kbd + 1, ~0x08 },
   { "G",   input.kbd + 1, ~0x10 },

   { "Q",   input.kbd + 2, ~0x01 },
   { "W",   input.kbd + 2, ~0x02 },
   { "E",   input.kbd + 2, ~0x04 },
   { "R",   input.kbd + 2, ~0x08 },
   { "T",   input.kbd + 2, ~0x10 },

   { "1",   input.kbd + 3, ~0x01 },
   { "2",   input.kbd + 3, ~0x02 },
   { "3",   input.kbd + 3, ~0x04 },
   { "4",   input.kbd + 3, ~0x08 },
   { "5",   input.kbd + 3, ~0x10 },

   { "0",   input.kbd + 4, ~0x01 },
   { "9",   input.kbd + 4, ~0x02 },
   { "8",   input.kbd + 4, ~0x04 },
   { "7",   input.kbd + 4, ~0x08 },
   { "6",   input.kbd + 4, ~0x10 },

   { "P",   input.kbd + 5, ~0x01 },
   { "O",   input.kbd + 5, ~0x02 },
   { "I",   input.kbd + 5, ~0x04 },
   { "U",   input.kbd + 5, ~0x08 },
   { "Y",   input.kbd + 5, ~0x10 },

   { "L",   input.kbd + 6, ~0x02 },
   { "K",   input.kbd + 6, ~0x04 },
   { "J",   input.kbd + 6, ~0x08 },
   { "H",   input.kbd + 6, ~0x10 },

   { "M",   input.kbd + 7, ~0x04 },
   { "N",   input.kbd + 7, ~0x08 },
   { "B",   input.kbd + 7, ~0x10 },
};

static zxkey zxk_bk08[] =
{
   { "KRIGHT", &input.kjoy, ~1 },
   { "KLEFT",  &input.kjoy, ~2 },
   { "KDOWN",  &input.kjoy, ~4 },
   { "KUP",    &input.kjoy, ~8 },
   { "KFIRE",  &input.kjoy, ~16},

   { "ALT", input.kbd + 0, ~0x01 },
   { "Z",   input.kbd + 0, ~0x02 },
   { "X",   input.kbd + 0, ~0x04 },
   { "C",   input.kbd + 0, ~0x08 },
   { "V",   input.kbd + 0, ~0x10 },
   { "RUS", input.kbd + 0, ~0x20 },
   { "SHF", input.kbd + 0,  0x7F },

   { "A",   input.kbd + 1, ~0x01 },
   { "S",   input.kbd + 1, ~0x02 },
   { "D",   input.kbd + 1, ~0x04 },
   { "F",   input.kbd + 1, ~0x08 },
   { "G",   input.kbd + 1, ~0x10 },
   { "BSL", input.kbd + 1, ~0x20 },
   { "SL",  input.kbd + 1,  0x7F },

   { "Q",   input.kbd + 2, ~0x01 },
   { "W",   input.kbd + 2, ~0x02 },
   { "E",   input.kbd + 2, ~0x04 },
   { "R",   input.kbd + 2, ~0x08 },
   { "T",   input.kbd + 2, ~0x10 },
   { "CMA", input.kbd + 2, ~0x20 },
   { "PNT", input.kbd + 2,  0x7F },

   { "1",   input.kbd + 3, ~0x01 },
   { "2",   input.kbd + 3, ~0x02 },
   { "3",   input.kbd + 3, ~0x04 },
   { "4",   input.kbd + 3, ~0x08 },
   { "5",   input.kbd + 3, ~0x10 },
   { "TIL", input.kbd + 3, ~0x20 },
   { "TAB", input.kbd + 3,  0x7F },

   { "0",   input.kbd + 4, ~0x01 },
   { "9",   input.kbd + 4, ~0x02 },
   { "8",   input.kbd + 4, ~0x04 },
   { "7",   input.kbd + 4, ~0x08 },
   { "6",   input.kbd + 4, ~0x10 },
   { "MNS", input.kbd + 4, ~0x20 },
   { "PLS", input.kbd + 4,  0x7F },

   { "P",   input.kbd + 5, ~0x01 },
   { "O",   input.kbd + 5, ~0x02 },
   { "I",   input.kbd + 5, ~0x04 },
   { "U",   input.kbd + 5, ~0x08 },
   { "Y",   input.kbd + 5, ~0x10 },
   { "LB",  input.kbd + 5, ~0x20 },
   { "RB",  input.kbd + 5,  0x7F },

   { "ENT", input.kbd + 6, ~0x01 },
   { "L",   input.kbd + 6, ~0x02 },
   { "K",   input.kbd + 6, ~0x04 },
   { "J",   input.kbd + 6, ~0x08 },
   { "H",   input.kbd + 6, ~0x10 },
   { "COL", input.kbd + 6, ~0x20 },
   { "QUO", input.kbd + 6,  0x7F },

   { "SPC", input.kbd + 7, ~0x01 },
   { "CTL", input.kbd + 7, ~0x02 },
   { "M",   input.kbd + 7, ~0x04 },
   { "N",   input.kbd + 7, ~0x08 },
   { "B",   input.kbd + 7, ~0x10 },
   { "R/A", input.kbd + 7, ~0x20 },
   { "CPS", input.kbd + 7,  0x7F }
};

static zxkey zxk_quorum[] =
{
   { "KRIGHT", &input.kjoy, ~1 },
   { "KLEFT",  &input.kjoy, ~2 },
   { "KDOWN",  &input.kjoy, ~4 },
   { "KUP",    &input.kjoy, ~8 },
   { "KFIRE",  &input.kjoy, ~16},

   { "ENT", input.kbd + 6, ~0x01 },
   { "SPC", input.kbd + 7, ~0x01 },
   { "SYM", input.kbd + 7, ~0x02 },

   { "CAP", input.kbd + 0, ~0x01 },
   { "Z",   input.kbd + 0, ~0x02 },
   { "X",   input.kbd + 0, ~0x04 },
   { "C",   input.kbd + 0, ~0x08 },
   { "V",   input.kbd + 0, ~0x10 },

   { "A",   input.kbd + 1, ~0x01 },
   { "S",   input.kbd + 1, ~0x02 },
   { "D",   input.kbd + 1, ~0x04 },
   { "F",   input.kbd + 1, ~0x08 },
   { "G",   input.kbd + 1, ~0x10 },

   { "Q",   input.kbd + 2, ~0x01 },
   { "W",   input.kbd + 2, ~0x02 },
   { "E",   input.kbd + 2, ~0x04 },
   { "R",   input.kbd + 2, ~0x08 },
   { "T",   input.kbd + 2, ~0x10 },

   { "1",   input.kbd + 3, ~0x01 },
   { "2",   input.kbd + 3, ~0x02 },
   { "3",   input.kbd + 3, ~0x04 },
   { "4",   input.kbd + 3, ~0x08 },
   { "5",   input.kbd + 3, ~0x10 },

   { "0",   input.kbd + 4, ~0x01 },
   { "9",   input.kbd + 4, ~0x02 },
   { "8",   input.kbd + 4, ~0x04 },
   { "7",   input.kbd + 4, ~0x08 },
   { "6",   input.kbd + 4, ~0x10 },

   { "P",   input.kbd + 5, ~0x01 },
   { "O",   input.kbd + 5, ~0x02 },
   { "I",   input.kbd + 5, ~0x04 },
   { "U",   input.kbd + 5, ~0x08 },
   { "Y",   input.kbd + 5, ~0x10 },

   { "L",   input.kbd + 6, ~0x02 },
   { "K",   input.kbd + 6, ~0x04 },
   { "J",   input.kbd + 6, ~0x08 },
   { "H",   input.kbd + 6, ~0x10 },

   { "M",   input.kbd + 7, ~0x04 },
   { "N",   input.kbd + 7, ~0x08 },
   { "B",   input.kbd + 7, ~0x10 },

   // quorum additional keys
   { "RUS",    input.kbd + 8, ~0x01},
   { "LAT",    input.kbd + 8, ~0x02},
   { "N1",     input.kbd + 8, ~0x08},
   { "N2",     input.kbd + 8, ~0x10},
   { ".",      input.kbd + 8, ~0x20},

   { "CAPS",   input.kbd + 9, ~0x01},
   { "F2",     input.kbd + 9, ~0x02},
   { "TILDA",  input.kbd + 9, ~0x04},
   { "N4",     input.kbd + 9, ~0x08},
   { "QUOTE",  input.kbd + 9, ~0x10},
   { "N6",     input.kbd + 9, ~0x20},

   { "TAB",    input.kbd + 10, ~0x01},
   { "F4",     input.kbd + 10, ~0x02},
   { "N7",     input.kbd + 10, ~0x08},
   { "N5",     input.kbd + 10, ~0x10},
   { "N9",     input.kbd + 10, ~0x20},

   { "EBOX",   input.kbd + 11, ~0x01},
   { "F5",     input.kbd + 11, ~0x02},
   { "BS",     input.kbd + 11, ~0x04},
   { "NSLASH", input.kbd + 11, ~0x08},
   { "N8",     input.kbd + 11, ~0x10},
   { "NMINUS", input.kbd + 11, ~0x20},

   { "-",      input.kbd + 12, ~0x01},
   { "+",      input.kbd + 12, ~0x04},
   { "DEL",    input.kbd + 12, ~0x08},
   { "NSTAR",  input.kbd + 12, ~0x10},
   { "GBOX",   input.kbd + 12, ~0x20},

   { "COLON",  input.kbd + 13, ~0x01},
   { "F3",     input.kbd + 13, ~0x02},
   { "\\",     input.kbd + 13, ~0x04},
   { "]",      input.kbd + 13, ~0x10},
   { "[",      input.kbd + 13, ~0x20},

   { ",",      input.kbd + 14, ~0x01},
   { "/",      input.kbd + 14, ~0x10},
   { "N3",     input.kbd + 14, ~0x20},

   { "F1",     input.kbd + 15, ~0x02},
   { "N0",     input.kbd + 15, ~0x08},
   { "NPOINT", input.kbd + 15, ~0x10},
   { "NPLUS",  input.kbd + 15, ~0x20},
};

zxkeymap zxk_maps[] =
{
   { "default", zxk_default, _countof(zxk_default) },
   { "BK08", zxk_bk08, _countof(zxk_bk08) },
   { "quorum", zxk_quorum, _countof(zxk_quorum) },
};

const size_t zxk_maps_count = _countof(zxk_maps);

PALETTEENTRY syspalette[0x100];

gdibmp_t gdibmp = { { { sizeof(BITMAPINFOHEADER), 320, -240, 1, 32, BI_RGB, 0 } } };
gdibmp_t debug_gdibmp = { { { sizeof(BITMAPINFOHEADER), DEBUG_WND_WIDTH, -DEBUG_WND_HEIGHT, 1, 8, BI_RGB, 0 } } };

palette_options pals[32] = { {"default",0x00,0x80,0xC0,0xE0,0xFF,0xC8,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0xFF} };

#pragma pack()

u8 snbuf[snd_buf_sz];
u8 gdibuf[gdi_buf_sz];
u8 debug_gdibuf[DBG_GDIBUFSZ];

// on-screen watches block
unsigned watch_script[4][64];
u8 watch_enabled[4];
u8 used_banks[MAX_PAGES];
u8 trace_rom = 1, trace_ram = 1;

DWORD WinVerMajor;
DWORD WinVerMinor;
HWND dlg; // used in setcheck/getcheck: gui settings, monitor dialogs

HBITMAP hbm;  // bitmap for repaint background
DWORD bm_dx, bm_dy;
DWORD mousepos;  // left-clicked point in monitor
unsigned nowait; // don't close console window after error if started from GUI
bool normal_exit = false; // true on correct terminate, false on failure terminate

char* ayvols[64]; unsigned num_ayvols;
char* aystereo[64]; unsigned num_aystereo;
char* ulapreset[64]; unsigned num_ula;
char presetbuf[0x4000], * setptr = presetbuf;

/*
#include "fontdata.cpp"
#include "font8.cpp"
#include "font14.cpp"
#include "font16.cpp"
#include "fontatm2.cpp"
*/

const char* const ay_schemes[] = { "no soundchip", "single chip", "pseudo-turbo", "quadro-AY", "turbo-AY // POS", "turbo-sound // NedoPC", "AYX-32" };
