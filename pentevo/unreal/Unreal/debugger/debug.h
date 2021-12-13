#pragma once
#include <defs.h>

constexpr auto regs_x = 1;
constexpr auto regs_y = 1;

constexpr auto stack_x = 72;
constexpr auto stack_y = 12;
constexpr auto stack_size = 10;

constexpr auto ay_x = 31;
constexpr auto ay_y = 28;

constexpr auto time_x = 1;
constexpr auto time_y = 28;

constexpr auto copy_x = 1;
constexpr auto copy_y = 28;

constexpr auto banks_x = 72;
constexpr auto banks_y = 22;

constexpr auto ports_x = 72;
constexpr auto ports_y = 1;

constexpr auto dos_x = 72;
constexpr auto dos_y = 6;
constexpr auto trace_size = 21;
constexpr auto trace_x = 1;
constexpr auto trace_y = 6;

constexpr auto wat_x = 34;
constexpr auto wat_y = 1;
constexpr auto wat_sz_x = 37;
constexpr auto wat_sz_y = 13;
constexpr auto wat_sz = wat_sz_y;

constexpr auto mem_size = 12;
constexpr auto mem_x = 34;
constexpr auto mem_y = 15;

constexpr auto w_sel = 0x17;
constexpr auto w_norm = 0x07;
constexpr auto w_curs = 0x30;
constexpr auto backgr = 0x50;
constexpr auto frame_curs = 0x02;
constexpr auto w_title = 0x59;
constexpr auto w_other = 0x40;
constexpr auto w_otheroff = 0x47;
constexpr auto backgr_ch = 0xB1;
constexpr auto w_aynum = 0x4F;
constexpr auto w_ayon = 0x41;
constexpr auto w_ayoff = 0x40;
constexpr auto w_bank = 0x40;
constexpr auto w_bankro = 0x41;
constexpr auto w_dihalt1 = 0x1A;
constexpr auto w_dihalt2 = 0x0A;
constexpr auto w_tracepos = 0x70;
constexpr auto w_inputcur = 0x60;
constexpr auto w_inputbg = 0x40;
constexpr auto w_48_k = 0x20;
constexpr auto w_dos = 0x20;

constexpr auto pc_history_x = 80;
constexpr auto pc_history_y = 0;
constexpr auto pc_history_wx = 7;
constexpr auto pc_history_wy = 28;

constexpr auto w_trace_jinfo_curs_fg = 0x0D;
constexpr auto w_trace_jinfo_nocurs_fg = 0x02;
constexpr auto w_trace_jarrow_foregr = 0x0D;

constexpr auto FRAME = 0x01;
constexpr auto fframe_frame = 0x04;

constexpr auto fframe_inside = 0x50;
constexpr auto fframe_error = 0x52;
constexpr auto frm_header = 0xD0;

constexpr auto menu_inside = 0x70;
constexpr auto menu_header = 0xF0;

constexpr auto menu_cursor = 0xE0;
constexpr auto menu_item = menu_inside;
constexpr auto menu_item_dis = 0x7A;

constexpr auto tsconf_base_x = 88;
constexpr auto tsconf_base_y = 0;

constexpr auto debug_text_width = 157;
constexpr auto debug_text_height = 30;
constexpr auto debug_text_size = (debug_text_width * debug_text_height);
constexpr auto DEBUG_WND_WIDTH = (debug_text_width * 8);
constexpr auto DEBUG_WND_HEIGHT = (debug_text_height * 16);

constexpr auto debug_text_width_notsconf = 88;
constexpr auto DEBUG_WND_WIDTH_NOTSCONF = (debug_text_width_notsconf * 8);

enum dbgwnd
{
	wndno = 0,
	wndmem,
	wndtrace,
	wndregs,
	wndbanks
};

enum { ed_mem, ed_phys, ed_log, ed_cmos, ed_nvram, ed_max };

class t_cpu_mgr final
{
	static const unsigned Count;
	static Z80* cpus_[];
	static TZ80State prev_cpus_[];
	static unsigned current_cpu_;
public:
	static Z80 &get_cpu() { return *cpus_[current_cpu_]; }
	static Z80 &get_cpu(const u32 idx) { return *cpus_[idx]; }
	static TZ80State &prev_cpu(const u32 idx) { return prev_cpus_[idx]; }
	static TZ80State &prev_cpu() { return prev_cpus_[current_cpu_]; }
	static void switch_cpu();
	static unsigned get_current_cpu() { return current_cpu_; }
	static void set_current_cpu(const u32 idx) { current_cpu_ = idx; }
	static void copy_to_prev();
	static unsigned get_count() { return Count; }
};

// debug breakpoints format descriptor
typedef struct
{
	unsigned reg;       // token string (e.g. 'DOS','OUT')
	const void *ptr;    // pointer to variable, containing queried value
	u8 size;            // size of variable (1, 2, 4) or 0 for address of function: bool func()
} BPXR;

extern t_cpu_mgr cpu_mgr;
extern dbgwnd activedbg;
extern unsigned dbg_extport;
extern u8 dgb_extval; // extended memory port like 1FFD or DFFD

extern unsigned mem_sz;
extern unsigned mem_disk;
extern unsigned mem_track;
extern unsigned mem_max;

extern u8 mem_dump;
extern unsigned show_scrshot;
extern unsigned scrshot_page_mask;
extern u8 editor;
extern u8 mem_ascii;

extern unsigned ripper; // ripper mode (none/read/write)

extern unsigned user_watches[3];
extern unsigned regs_curs;

extern u8 trace_labels;

u8 isbrk(const Z80 &cpu); // is there breakpoints active or any other reason to use debug z80 loop?
void debugscr();
void debug_events(Z80 *cpu);

void flip_from_debug();

void init_debug();
void set_debug_window_size();
