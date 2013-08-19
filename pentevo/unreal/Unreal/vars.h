#pragma once
#include "emul_2203.h"
#include "sndchip.h"
#include "saa1099.h"

#define ROMLED_TIME 16

#pragma pack(8)
struct PALETTE_OPTIONS
{ // custom palettes
   char name[33];
   unsigned ZZ,ZN,NN,NB,BB,ZB;
   unsigned r11,r12,r13,r21,r22,r23,r31,r32,r33;
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
   MEMBITS_R = 0x01, MEMBITS_W = 0x02, MEMBITS_X = 0x04,
   MEMBITS_BPR = 0x10, MEMBITS_BPW = 0x20, MEMBITS_BPX = 0x40
};

struct GDIBMP
{
   BITMAPINFO header;
   RGBQUAD waste[0x100];
};

class TMainZ80 : public Z80
{
public:
   TMainZ80(u32 Idx,
       TBankNames BankNames, TStep Step, TDelta Delta,
       TSetLastT SetLastT, u8 *membits, const TMemIf *FastMemIf, const TMemIf *DbgMemIf) :
       Z80(Idx, BankNames, Step, Delta, SetLastT, membits, FastMemIf, DbgMemIf) { }
/*
   virtual unsigned char rm(unsigned addr) override;
   virtual unsigned char dbgrm(unsigned addr) override;
   virtual void wm(unsigned addr, unsigned char val) override;
   virtual void dbgwm(unsigned addr, unsigned char val) override;
*/
   virtual u8 *DirectMem(unsigned addr) const override; // get direct memory pointer in debuger

   virtual unsigned char m1_cycle() override;
   virtual unsigned char in(unsigned port) override;
   virtual void out(unsigned port, unsigned char val) override;
   virtual u8 IntVec() override;
   virtual void CheckNextFrame() override;
   virtual void retn() override;
};

extern PALETTE_OPTIONS pals[32];

extern CONFIG conf;
extern COMPUTER comp;
extern unsigned char memory[];

extern unsigned char cmos[0x100];
extern unsigned char nvram[0x800];

extern char ininame[0x200];
extern char helpname[0x200];

extern unsigned num_ula;
extern char *ulapreset[64];
extern char *setptr;

extern char *aystereo[64];
extern char *ayvols[64];
extern unsigned num_ayvols;
extern unsigned num_aystereo;

extern DWORD WinVerMajor;
extern DWORD WinVerMinor;

extern HWND wnd;
extern HWND dlg;
extern HINSTANCE hIn;
extern unsigned nowait;

extern action ac_main[];
extern action ac_main_xt[];
extern action ac_mon[];
extern action ac_regs[];
extern action ac_trace[];
extern action ac_mem[];
extern RENDER renders[];
extern BORDSIZE bordersizes[];
extern VOID_FUNC prebuffers[];

extern const TMemModel mem_model[N_MM_MODELS];

extern zxkeymap zxk_maps[];
extern const size_t zxk_maps_count;

extern virtkeyt pckeys[];
extern const size_t pckeys_count;

extern keyports inports[VK_MAX];

extern unsigned trd_toload; // drive to load
extern unsigned DefaultDrive; // Дисковод по умолчанию в который грузятся образы дисков при старте

extern unsigned char *base_sos_rom;
extern unsigned char *base_dos_rom;
extern unsigned char *base_128_rom;
extern unsigned char *base_sys_rom;

extern ISA_MODEM modem;
extern K_INPUT input;

extern unsigned brk_port_in;
extern unsigned brk_port_out;
extern unsigned char brk_port_val;

extern unsigned watch_script[4][64];
extern unsigned char watch_enabled[4];
extern unsigned char used_banks[MAX_PAGES];
extern unsigned char trace_rom;
extern unsigned char trace_ram;

extern TMainZ80 cpu;
extern unsigned char dbgbreak;
extern unsigned char snbuf[4*1048576]; // large temporary buffer (for reading snapshots)
extern unsigned char gdibuf[2*1048576];

extern SNDCHIP ay[2];

extern unsigned char *bankr[4];
extern unsigned char *bankw[4];
extern unsigned char bankm[4];		// bank mode: 0 - ROM / 1 - RAM

#ifdef MOD_GSBASS
extern GSHLE gs;
#endif

extern GDIBMP gdibmp;
extern unsigned char needclr; // clear screenbuffer before rendering
extern DWORD mousepos;  // left-clicked point in monitor
extern PALETTEENTRY syspalette[0x100];
extern unsigned char exitflag; // avoid call exit() twice

#define PLAYBUFSIZE 16384
extern unsigned sndplaybuf[PLAYBUFSIZE];
extern unsigned spbsize;
extern unsigned char savesndtype; // 0-none,1-wave,2-vtx
extern FILE *savesnd;

extern HBITMAP hbm;  // bitmap for repaint background
extern DWORD bm_dx;
extern DWORD bm_dy;

extern char droppedFile[512];

extern char statusline[128];
extern unsigned statcnt;

extern bool normal_exit;

extern const char * const ay_schemes[];

#ifdef MOD_GSZ80
extern class TGsZ80 gscpu;

namespace z80gs
{
    extern SNDRENDER sound;
    extern u8 membits[];
}
#endif

extern ATA_PORT hdd;   // not in `comp' - not cleared in reset()
extern char arcbuffer[0x2000]; // extensions and command lines for archivers
extern char skiparc[0x400]; // ignore this files in archive
extern char trd_loaded[4]; // used to get first free drive with no account of autoloaded images
extern unsigned char kbdpc[VK_MAX]; // add cells for mouse & joystick

extern char pressedit; //Alone Coder
extern int fmsoundon0; //Alone Coder
extern int tfmstatuson0; //Alone Coder

// for leds
extern unsigned char trdos_load;
extern unsigned char trdos_save;
extern unsigned char trdos_format;
extern unsigned char trdos_seek;
extern unsigned char membits[0x10000];

extern unsigned char *vtxbuf;
extern unsigned vtxbufsize;
extern unsigned vtxbuffilled;

extern unsigned snapsize;

extern unsigned char kbdpcEX[6]; //Dexus
