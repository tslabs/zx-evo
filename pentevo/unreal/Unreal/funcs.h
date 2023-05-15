#pragma once
#include "sysdefs.h"
#include "emulator/z80/z80.h"

void correct_exit();
void wnd_resize(int scale);
void main_mouse();

void do_screenshot();
void qsave(char*);
void qload(char*);
void savesnap(int);

void setpal(char);
void set_vidmode();
void set_video();
void calc_rsm_tables();
void update_screen();

void spectrum_frame();
void do_sound();
void flip();

void trdos_traps();
void tape_traps();
void fast_tape();
void reset_tape();
u8 tape_bit();

void out(unsigned, u8);
u8 in(unsigned port);
void ts_ext_port_wr(u8, u8);
void set_banks();

void applyconfig();
void apply_video();
void apply_gs();
void setup_dlg();
void savesnddialog();
void load_labels(char *filename, u8 *base, unsigned size);
u8 isbrk();

void prepare_chunks();
void prepare_chunks32();

void init_gs_frame();
void flush_gs_frame();
void reset_gs();
void reset_gs_sound();

void load_ay_stereo();
void load_ay_vols();
void load_ula_preset();

void restart_sound();
void create_font_tables();

void reset(rom_mode mode);

void debug_events(Z80& cpu);
void debug_cond_check(Z80& cpu);

void render_small(u8 *dst, unsigned pitch);

int loadsnap(char *filename);
u8 what_is(char *filename);

u8 getcheck(unsigned ID);
