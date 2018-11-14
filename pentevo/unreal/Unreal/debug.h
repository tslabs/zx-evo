#pragma once

#define regs_x 1
#define regs_y 1

#define stack_x 72
#define stack_y 12
#define stack_size 10

#define ay_x  31
#define ay_y  28

#define time_x 1
#define time_y 28

#define copy_x 1
#define copy_y 28

#define banks_x 72
#define banks_y 22

#define ports_x 72
#define ports_y 1

#define dos_x 72
#define dos_y 6
#define trace_size 21
#define trace_x 1
#define trace_y 6

#define wat_x 34
#define wat_y 1
#define wat_sz 13

#define mem_size 12
#define mem_x 34
#define mem_y 15

#define W_SEL      0x17
#define W_NORM     0x07
#define W_CURS     0x30
#define BACKGR     0x50
#define FRAME_CURS 0x02
#define W_TITLE    0x59
#define W_OTHER    0x40
#define W_OTHEROFF 0x47
#define BACKGR_CH  0xB1
#define W_AYNUM    0x4F
#define W_AYON     0x41
#define W_AYOFF    0x40
#define W_BANK     0x40
#define W_BANKRO   0x41
#define W_DIHALT1  0x1A
#define W_DIHALT2  0x0A
#define W_TRACEPOS 0x70
#define W_INPUTCUR 0x60
#define W_INPUTBG  0x40
#define W_48K 0x20
#define W_DOS 0x20

#define W_TRACE_JINFO_CURS_FG   0x0D
#define W_TRACE_JINFO_NOCURS_FG 0x02
#define W_TRACE_JARROW_FOREGR   0x0D

#define FRAME         0x01
#define FFRAME_FRAME  0x04

#define FFRAME_INSIDE 0x50
#define FFRAME_ERROR  0x52
#define FRM_HEADER    0xD0

#define MENU_INSIDE   0x70
#define MENU_HEADER   0xF0

#define MENU_CURSOR   0xE0
#define MENU_ITEM     MENU_INSIDE
#define MENU_ITEM_DIS 0x7A

#define DEBUG_TEXT_WIDTH	150
#define	DEBUG_TEXT_HEIGHT	30
#define DEBUG_TEXT_SIZE		(DEBUG_TEXT_WIDTH * DEBUG_TEXT_HEIGHT)
#define DEBUG_WND_WIDTH		(DEBUG_TEXT_WIDTH * 8)
#define DEBUG_WND_HEIGHT	(DEBUG_TEXT_HEIGHT * 16)

enum DBGWND
{
   WNDNO = 0,
   WNDMEM,
   WNDTRACE,
   WNDREGS,
   WNDBANKS
};

enum { ED_MEM, ED_PHYS, ED_LOG, ED_CMOS, ED_NVRAM, ED_MAX };

class TCpuMgr
{
    static const unsigned Count;
    static Z80* Cpus[];
    static TZ80State PrevCpus[];
    static unsigned CurrentCpu;
public:
    static Z80 &Cpu() { return *Cpus[CurrentCpu]; }
    static Z80 &Cpu(u32 Idx) { return *Cpus[Idx]; }
    static TZ80State &PrevCpu(u32 Idx) { return PrevCpus[Idx]; }
    static TZ80State &PrevCpu() { return PrevCpus[CurrentCpu]; }
    static void SwitchCpu();
    static unsigned GetCurrentCpu() { return CurrentCpu; }
    static void SetCurrentCpu(u32 Idx) { CurrentCpu = Idx; }
    static void CopyToPrev();
    static unsigned GetCount() { return Count; }
};

// debug breakpoints format descriptor
typedef struct
{
  unsigned reg;       // token string (e.g. 'DOS','OUT')
  const void *ptr;    // pointer to variable, containing queried value
  u8 size;            // size of variable (1, 2, 4) or 0 for address of function: bool func()
} BPXR;

extern TCpuMgr CpuMgr;
extern DBGWND activedbg;
extern unsigned dbg_extport;
extern u8 dgb_extval; // extended memory port like 1FFD or DFFD

extern unsigned mem_sz;
extern unsigned mem_disk;
extern unsigned mem_track;
extern unsigned mem_max;

extern u8 mem_dump;
extern unsigned show_scrshot;
extern u8 editor;
extern u8 mem_ascii;
extern u8 mem_dump;

extern unsigned ripper; // ripper mode (none/read/write)

extern unsigned user_watches[3];
extern unsigned regs_curs;

extern u8 trace_labels;

u8 isbrk(const Z80 &cpu); // is there breakpoints active or any other reason to use debug z80 loop?
void debugscr();
void debug_events(Z80 *cpu);

void init_debug();
