void __declspec(noreturn) exit();
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
unsigned char tape_bit();

void out(unsigned port, unsigned char val);
unsigned char in(unsigned port);
void set_banks();

void applyconfig();
void apply_video();
void apply_gs();
void setup_dlg();
void savesnddialog();
void load_labels(char *filename, unsigned char *base, unsigned size);
unsigned char isbrk();

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

void reset(ROM_MODE mode);

void debug_events(Z80 *cpu);

void __fastcall render_rsm(unsigned char*, unsigned); //Alone Coder
void __fastcall render_advmame(unsigned char *dst, unsigned pitch); //Alone Coder
void __fastcall render_small(unsigned char *dst, unsigned pitch);
void rend_dbl(unsigned char *dst, unsigned pitch);

int loadsnap(char *filename);
unsigned char what_is(char *filename);

unsigned char getcheck(unsigned ID);
