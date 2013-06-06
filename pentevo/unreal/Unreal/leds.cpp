#include "std.h"
#include "emul.h"
#include "vars.h"
#include "font.h"
#include "font16.h"
#include "gs.h"
#include "tape.h"
#include "draw.h"
#include "debug.h"
#include "dbgbpx.h"
#include "memory.h"
#include "util.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

unsigned pitch;

void text_i(unsigned char *dst, const char *text, unsigned char ink, unsigned off = 0)
{
   unsigned char mask = 0xF0; ink &= 0x0F;
   for (unsigned char *x = (unsigned char*)text; *x; x++) {
      unsigned char *d0 = dst;
      for (unsigned y = 0; y < 8; y++) {
         unsigned char byte = font[(*x)*8+y];
         d0[0] = (byte >> off) + (d0[0] & ~(0xFC >> off));
         d0[1] = (d0[1] & 0xF0) + ink;
         if (off > 2) {
            d0[2] = (byte << (8-off)) + (d0[2] & ~(0xFC << (8-off)));
            d0[3] = (d0[3] & 0xF0) + ink;
         }
         d0 += pitch;
      }
      off += 6; if (off & 8) off -= 8, dst += 2;
   }
}

static void text_16(unsigned char *dst, const char *text, unsigned char attr)
{
   for (; *text; text++, dst += 2)
      for (unsigned y = 0; y < 16; y++)
         dst[y*pitch] = font16[16 * *(unsigned char*)text + y],
         dst[y*pitch+1] = attr;
}

unsigned char *aypos;
void paint_led(unsigned level, unsigned char at)
{
   if (level) {
      if (level > 15) level = 15, at = 0x0E;
      unsigned mask = (0xFFFF0000 >> level) & 0xFFFF;
      aypos[0] = mask >> 8;
      aypos[2] = (unsigned char)mask;
      aypos[1] = (aypos[1] & 0xF0) + at, aypos[3] = (aypos[3] & 0xF0) + at;
   }
   aypos += pitch;
}

void ay_led()
{
   aypos = temp.led.ay;
   unsigned char sum=0;

   int max_ay = (conf.sound.ay_scheme > AY_SCHEME_PSEUDO)? 2 : 1;
   for (int n_ay = 0; n_ay < max_ay; n_ay++) {
      for (int i = 0; i < 3; i++) {
         unsigned char r_mix = ay[n_ay].get_reg(7);
         unsigned char tone = (r_mix >> i) & 1,
                       noise = (r_mix >> (i+3)) & 1;
         unsigned char c1 = 0, c2 = 0;
         unsigned char v = ay[n_ay].get_reg(i+8);
         if (!tone) c1 = c2 = 0x0F;
         if (!noise) c2 = 0x0E;
         if (v & 0x10) {
            unsigned r_envT = ay[n_ay].get_reg(11) + 0x100*ay[n_ay].get_reg(12);
            if (r_envT < 0x400) {
               v = (3-(r_envT>>3)) & 0x0F;
               if (!v) v = 6;
            } else v = ay[n_ay].get_env()/2;
            c1 = 0x0C;
         } else v &= 0x0F;
         if (!c1) c1 = c2;
         if (!c2) c2 = c1;
         if (!c1) v = 0;
         sum |= v;
         paint_led(v, c1);
         paint_led(v, c2);
         paint_led(0, 0);
      }
   }

   const unsigned FMvols[]={4,9,15,23,35,48,70,105,140,195,243,335,452,608,761,1023};
   #define _cBlue 0x09
   #define _cRed 0x0a
   #define _cPurp 0x0b
   #define _cGreen 0x0c
   #define _cCyan 0x0d
   #define _cYell 0x0e
   #define _cWhite 0x0f

   const int FMalg1[]={
   _cBlue , _cPurp , _cGreen, _cWhite,
   _cPurp , _cPurp , _cGreen, _cWhite,
   _cGreen, _cPurp , _cGreen, _cWhite,
   _cPurp , _cGreen, _cGreen, _cWhite,

   _cGreen, _cWhite, _cGreen, _cWhite,
   _cPurp , _cWhite, _cWhite, _cWhite,
   _cGreen, _cWhite, _cWhite, _cWhite,
   _cWhite, _cWhite, _cWhite, _cWhite
   };

   const int FMalg2[]={
   _cBlue , _cPurp , _cGreen, _cYell ,
   _cPurp , _cPurp , _cGreen, _cYell ,
   _cGreen, _cPurp , _cGreen, _cYell ,
   _cPurp , _cGreen, _cGreen, _cYell ,

   _cGreen, _cYell , _cGreen, _cYell ,
   _cPurp , _cYell , _cYell , _cYell ,
   _cGreen, _cYell , _cYell , _cYell ,
   _cYell , _cYell , _cYell , _cYell 
   };

   const int FMslots[]={0,2,1,3};
/*
   const int FMalg1[]={
   _cPurp , _cGreen, _cCyan , _cWhite,
   _cPurp , _cPurp , _cGreen, _cWhite,
   _cGreen, _cPurp , _cCyan , _cWhite,
   _cPurp , _cCyan , _cGreen, _cWhite,

   _cPurp , _cWhite, _cGreen, _cWhite,
   _cPurp , _cWhite, _cWhite, _cWhite,
   _cGreen, _cWhite, _cWhite, _cWhite,
   _cWhite, _cWhite, _cWhite, _cWhite
   };

   const int FMalg2[]={
   _cPurp , _cGreen, _cCyan , _cYell ,
   _cPurp , _cPurp , _cGreen, _cYell ,
   _cGreen, _cPurp , _cCyan , _cYell ,
   _cPurp , _cCyan , _cGreen, _cYell ,

   _cPurp , _cYell , _cGreen, _cYell ,
   _cPurp , _cYell , _cYell , _cYell ,
   _cGreen, _cYell , _cYell , _cYell ,
   _cYell , _cYell , _cYell , _cYell
   };
*/
   if ( conf.sound.ay_chip == SNDCHIP::CHIP_YM2203 ) {
      for (int ayN = 0; ayN < max_ay; ayN++) {
         for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
               unsigned v=ay[ayN].Chip2203->CH[i].SLOT[j].vol_out;
               if (v>1023) v=1023;
               int c; //Alone Coder 0.36.7
               for (/*int*/ c=0;c<16;c++)
                  if (FMvols[c]>=v) break;
               if ( (i == 2) && (((ay[ayN].Chip2203->OPN.ST.mode) & 0xc0) == 0x40) )
                  paint_led(15-c, FMalg2[ay[ayN].Chip2203->CH[i].ALGO * 4 + FMslots[j]]);
               else
                  paint_led(15-c, FMalg1[ay[ayN].Chip2203->CH[i].ALGO * 4 + FMslots[j]]);
            }
         paint_led(0, 0);
         }
      }
   } //Dexus

   #ifdef MOD_GS
   if (sum || !conf.gs_type) return; // else show GS indicators
   aypos = temp.led.ay; // reset y-pos, if nothing above
   for (unsigned ch = 0; ch < 8; ch++) {
      unsigned v = gsleds[ch].level, a = gsleds[ch].attrib;
      paint_led(v, a);
      paint_led(v, a);
      paint_led(0, 0);
   }

   #endif
}

void load_led()
{
   char ln[20]; unsigned char diskcolor = 0;

#ifdef GS_BASS
   if (gs.loadmod) {
      text_i(temp.led.load, "", 0x0D);
      gs.loadmod = 0;
   } else if (gs.loadfx) {
      sprintf(ln, "\x0D%d", gs.loadfx);
      text_i(temp.led.load, ln, 0x0D);
      gs.loadfx = 0;
   } else
#endif
   if (trdos_format) {
      diskcolor = (trdos_format < ROMLED_TIME*3/4) ? 0x06 : 0x0E;
      trdos_format--;
   } else if (trdos_save) {
      diskcolor = (trdos_save < ROMLED_TIME*3/4) ? 0x02 : 0x0A;
      trdos_save--;
   } else if (trdos_load) {
      diskcolor = (trdos_load < ROMLED_TIME*3/4) ? 0x01 : 0x09;
      trdos_load--;
   } else if (trdos_seek) {
      trdos_seek--;
   } else if (comp.tape.play_pointer) {
      static unsigned char tapeled[11*2] = {
         0x7F, 0xFE, 0x80, 0x01, 0x80, 0x01, 0x93, 0xC9, 0xAA, 0x55, 0x93, 0xC9,
         0x80, 0x01, 0x8F, 0xF1, 0x80, 0x01, 0xB5, 0xA9, 0xFF, 0xFF };
      const int tapecolor = 0x51;
      for (int i = 0; i < 11; i++)
         temp.led.load[pitch*i+0] = tapeled[2*i],
         temp.led.load[pitch*i+1] = tapecolor,
         temp.led.load[pitch*i+2] = tapeled[2*i+1],
         temp.led.load[pitch*i+3] = tapecolor;
      int time = (int)(temp.led.tape_started + tapeinfo[comp.tape.index].t_size - comp.t_states);
      if (time < 0) {
         find_tape_index(); time = 0;
         temp.led.tape_started = comp.t_states;
         unsigned char *ptr = tape_image + tapeinfo[comp.tape.index].pos;
         if (ptr == comp.tape.play_pointer && comp.tape.index)
            comp.tape.index--, ptr = tape_image + tapeinfo[comp.tape.index].pos;
         for (; ptr < comp.tape.play_pointer; ptr++)
            temp.led.tape_started -= tape_pulse[*ptr];
      }
      time /= (conf.frame * conf.intfq);
      sprintf(ln, "%X:%02d", time/60, time % 60);
      text_i(temp.led.load + pitch*12 - 2, ln, 0x0D);
   }
   if (diskcolor | trdos_seek) {
      if (diskcolor) {
         unsigned *ptr = (unsigned*)temp.led.load;
         int i; //Alone Coder 0.36.7
         for (/*int*/ i = 0; i < 7; i++, ptr = (unsigned*)((char*)ptr+pitch))
            *ptr = (*ptr & WORD4(0,0xF0,0,0xF0)) | WORD4(0x3F,diskcolor,0xFC,diskcolor);
         static unsigned char disk[] = { 0x38, 0x1C, 0x3B, 0x9C, 0x3B, 0x9C, 0x3B, 0x9C, 0x38,0x1C };
         for (i = 0; i < 5; i++, ptr = (unsigned*)((char*)ptr+pitch))
            *ptr = (*ptr & WORD4(0,0xF0,0,0xF0)) | WORD4(disk[2*i],diskcolor,disk[2*i+1],diskcolor);
      }
      if (comp.wd.seldrive->track != 0xFF) {
         sprintf(ln, "%02X", comp.wd.seldrive->track*2 + comp.wd.side);
         text_i(temp.led.load + pitch - 4, ln, 0x05 + (diskcolor & 8));
      }
   }
}

static unsigned p_frames = 1;
static u64 led_updtime, p_time;
double p_fps;
__inline void update_perf_led()
{
   u64 now = led_updtime - p_time;
   if (now >= temp.cpufq) // усреднение за секунду
   {
      p_fps = (p_frames * temp.cpufq) / double(now) + 0.005;
      p_frames = 0;
      p_time = led_updtime;
   }
   p_frames++;
}

void perf_led()
{
   char bf[0x20]; unsigned PSZ;
   if (conf.led.perf_t)
      sprintf(bf, "%6d*%2.2f", cpu.haltpos ? cpu.haltpos : cpu.t, p_fps), PSZ = 7;
   else
      sprintf(bf, "%2.2f fps", p_fps), PSZ = 5;
   text_i(temp.led.perf, bf, 0x0E);
   if (cpu.haltpos) {
      unsigned char *ptr = temp.led.perf + pitch*8;
      unsigned xx; //Alone Coder 0.36.7
      for (/*unsigned*/ xx = 0; xx < PSZ; xx++) *(unsigned short*)(ptr+xx*2) = 0x9A00;
      unsigned mx = cpu.haltpos*PSZ*8/conf.frame;
      for (xx = 1; xx < mx; xx++) ptr[(xx>>2)&0xFE] |= (0x80 >> (xx & 7));
   }
}

void input_led()
{
   if (input.kbdled != 0xFF) {
      unsigned char k0 = 0x99, k1 = 0x9F, k2 = 0x90;
      if (input.keymode == K_INPUT::KM_PASTE_HOLD) k0 = 0xAA, k1 = 0xAF, k2 = 0xA0;
      if (input.keymode == K_INPUT::KM_PASTE_RELEASE) k0 = 0x22, k1 = 0x2F, k2 = 0x20;

      int i; //Alone Coder 0.36.7
      for (/*int*/ i = 0; i < 5; i++)
         temp.led.input[1+i*2*pitch] = temp.led.input[3+i*2*pitch] = k0;
      for (i = 0; i < 4; i++)
         temp.led.input[pitch*(2*i+1)] = 0x7F,
         temp.led.input[pitch*(2*i+1)+2] = 0xFE;
      temp.led.input[pitch*1+1] = (input.kbdled & 0x08)? k2 : k1;
      temp.led.input[pitch*3+1] = (input.kbdled & 0x04)? k2 : k1;
      temp.led.input[pitch*5+1] = (input.kbdled & 0x02)? k2 : k1;
      temp.led.input[pitch*7+1] = (input.kbdled & 0x01)? k2 : k1;
      temp.led.input[pitch*1+3] = (input.kbdled & 0x10)? k2 : k1;
      temp.led.input[pitch*3+3] = (input.kbdled & 0x20)? k2 : k1;
      temp.led.input[pitch*5+3] = (input.kbdled & 0x40)? k2 : k1;
      temp.led.input[pitch*7+3] = (input.kbdled & 0x80)? k2 : k1;
   }
   static unsigned char joy[] =   { 0x10, 0x38, 0x1C, 0x1C, 0x1C, 0x1C, 0x08, 0x00, 0x7E, 0xFF, 0x00, 0xE7 };
   static unsigned char mouse[] = { 0x0C, 0x12, 0x01, 0x79, 0xB5, 0xB5, 0xB5, 0xFC, 0xFC, 0xFC, 0xFC, 0x78 };
   if (input.mouse_joy_led & 2)
      for (int i = 0; i < sizeof joy; i++)
         temp.led.input[4 + pitch*i] = joy[i],
         temp.led.input[4 + pitch*i+1] = (temp.led.input[4 + pitch*i+1] & 0xF0) + 0x0F;
   if (input.mouse_joy_led & 1)
      for (int i = 0; i < sizeof mouse; i++)
         temp.led.input[6 + pitch*i] = mouse[i],
         temp.led.input[6 + pitch*i+1] = (temp.led.input[6 + pitch*i+1] & 0xF0) + 0x0F;
   input.mouse_joy_led = 0; input.kbdled = 0xFF;
}

#ifdef MOD_MONITOR
void debug_led()
{
   unsigned char *ptr = temp.led.osw;
   if (trace_rom | trace_ram) {
      set_banks();
      if (trace_rom) {
         const unsigned char off = 0x01, on = 0x0C;
         text_i(ptr + 2,           "B48", used_banks[(base_sos_rom - memory) / PAGE] ? on : off);
         text_i(ptr + 8,           "DOS", used_banks[(base_dos_rom - memory) / PAGE] ? on : off);
         text_i(ptr + pitch*8 + 2, "128", used_banks[(base_128_rom - memory) / PAGE] ? on : off);
         text_i(ptr + pitch*8 + 8, "SYS", used_banks[(base_sys_rom - memory) / PAGE] ? on : off);
         ptr += pitch*16;
      }
      if (trace_ram) {
         unsigned num_rows = conf.ramsize/128;
         unsigned j; //Alone Coder 0.36.7
         for (unsigned  i = 0; i < num_rows; i++) {
            char ln[9];
            for (/*unsigned*/ j = 0; j < 8; j++)
               ln[j] = used_banks[i*8+j]? '*' : '-';
            ln[j] = 0;
            text_i(ptr, ln, 0x0D);
            ptr += pitch*8;
         }
      }
      for (unsigned j = 0; j < MAX_PAGES; j++) used_banks[j] = 0;
   }
   for (unsigned w = 0; w < 4; w++) if (watch_enabled[w])
   {
      char bf[12]; sprintf(bf, "%8X", calc(&cpu, watch_script[w]));
      text_i(ptr,bf,0x0F); ptr += pitch*8;
   }
}
#endif

#ifdef MOD_MEMBAND_LED
void show_mband(unsigned char *dst, unsigned start)
{
   char xx[8]; sprintf(xx, "%02X", start >> 8);
   text_i(dst, xx, 0x0B); dst += 4;

   Z80 &cpu = CpuMgr.Cpu();
   unsigned char band[128];
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < 128; i++) {
      unsigned char res = 0;
      for (unsigned q = 0; q < conf.led.bandBpp; q++)
         res |= cpu.membits[start++];
      band[i] = res;
   }

   for (unsigned p = 0; p < 16; p++, dst+=2) {
      unsigned char r=0, w=0, x=0;
      for (unsigned b = 0; b < 8; b++) {
         r *= 2, w *= 2, x *= 2;
         if (band[p*8+b] & MEMBITS_R) r |= 1;
         if (band[p*8+b] & MEMBITS_W) w |= 1;
         if (band[p*8+b] & MEMBITS_X) x |= 1;
      }

      unsigned char t = (p && !(p & 3))? 0x7F : 0xFF;

      dst[0*pitch] = t; dst[0*pitch+1] = 0x9B;

      dst[1*pitch] = r; dst[1*pitch+1] = 0x1C;
      dst[2*pitch] = r; dst[2*pitch+1] = 0x1C;
      dst[3*pitch] = w; dst[3*pitch+1] = 0x1A;
      dst[4*pitch] = w; dst[4*pitch+1] = 0x1A;
      dst[5*pitch] = x; dst[5*pitch+1] = 0x1F;
      dst[6*pitch] = x; dst[6*pitch+1] = 0x1F;

      dst[7*pitch] = t; dst[7*pitch+1] = 0x9B;
   }

   sprintf(xx, "%02X", (start-1) >> 8);
   text_i(dst, xx, 0x0B, 2);

   for (i = 0; i < 8; i++)
      dst[i*pitch] |= 0x80, dst[i*pitch - 17*2] |= 0x01;
}

void memband_led()
{
   unsigned char *dst = temp.led.memband;
   for (unsigned start = 0x0000; start < 0x10000;) {
      show_mband(dst, start);
      start += conf.led.bandBpp * 128;
      dst += 10*pitch;
   }

   Z80 &cpu = CpuMgr.Cpu();
   for (unsigned i = 0; i < 0x10000; i++)
      cpu.membits[i] &= ripper | ~(MEMBITS_R | MEMBITS_W | MEMBITS_X);
}
#endif


HANDLE hndKbdDev;

void init_leds()
{
   DefineDosDevice(DDD_RAW_TARGET_PATH, "Kbd_unreal_spec", "\\Device\\KeyboardClass0");
   hndKbdDev = CreateFile("\\\\.\\Kbd_unreal_spec", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
   if (hndKbdDev == INVALID_HANDLE_VALUE) hndKbdDev = 0, conf.led.flash_ay_kbd = 0;
}

void done_leds()
{
   if (hndKbdDev) {
      DefineDosDevice(DDD_REMOVE_DEFINITION, "Kbd_unreal_spec", 0);
      CloseHandle(hndKbdDev); hndKbdDev = 0;
   }
}

void ay_kbd()
{
   static unsigned char pA, pB, pC;
   static unsigned prev_keyled = -1;

   KEYBOARD_INDICATOR_PARAMETERS InputBuffer;
   InputBuffer.LedFlags = InputBuffer.UnitId = 0;

   if (ay->get_reg( 8) > pA) InputBuffer.LedFlags |= KEYBOARD_NUM_LOCK_ON;
   if (ay->get_reg( 9) > pB) InputBuffer.LedFlags |= KEYBOARD_CAPS_LOCK_ON;
   if (ay->get_reg(10) > pC) InputBuffer.LedFlags |= KEYBOARD_SCROLL_LOCK_ON;

   pA = ay->get_reg(8), pB = ay->get_reg(9), pC = ay->get_reg(10);

   DWORD xx;
   if (prev_keyled != InputBuffer.LedFlags)
      prev_keyled = InputBuffer.LedFlags,
      DeviceIoControl(hndKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
               &InputBuffer, sizeof(KEYBOARD_INDICATOR_PARAMETERS), 0, 0, &xx, 0);
}

void key_led()
{
   #define key_x 1
   #define key_y 1
   int i; //Alone Coder 0.36.7
   for (/*int*/ i = 0; i < 9; i++) text_16(rbuf+(key_y+i)*pitch*16+key_x*2, "                                 ", 0x40);
   static char ks[] = "cZXCVASDFGQWERT1234509876POIUYeLKJHssMNB";
   for (i = 0; i < 8; i++) {
      for (int j = 0; j < 5; j++) {
         unsigned x, y, at;
         if (i < 4) y = 7-2*i+key_y, x = 3*j+2+key_x;
         else y = 2*(i-4)+1+key_y, x = 29-3*j+key_x;
         unsigned a = ks[i*5+j]*0x100+' ';
         at = (input.kbd[i] & (1<<j))? 0x07 : ((input.rkbd[i] & (1<<j)) ? 0xA0:0xD0);
         text_16(rbuf+2*x+y*pitch*16,(char*)&a,at);
      }
   }
}

void time_led()
{
   static u64 prev_time;
   static char bf[8];
   if (led_updtime - prev_time > 5000) {
      prev_time = led_updtime;
      SYSTEMTIME st; GetLocalTime(&st);
      sprintf(bf, "%2d:%02d", st.wHour, st.wMinute);
   }
   text_i(temp.led.time, bf, 0x0D);
}

void init_memcycles(void)
{
   memset(vid.memcpucyc, 0, 320 * sizeof(vid.memcpucyc[0]));
   memset(vid.memvidcyc, 0, 320 * sizeof(vid.memvidcyc[0]));
   memset(vid.memtscyc,  0, 320 * sizeof( vid.memtscyc[0]));
}

void show_memcycle(u16 num, u32 col, u8 init, u32 vbpi)
{
static int cnt;
static u32 vbp;

	if (init)
	{
		cnt = 0;
		vbp = vbpi;
	}

	num /= 4;
	while (num && (cnt < 112))
		{ vbuf[vid.buf][vbp+cnt] = col; num--; cnt++; }
	while (num-- && (++cnt < 128))
		vbuf[vid.buf][vbp+cnt] = 0xFF0000;
}

void show_memcycles(void)
{
	u32 vbp;

	for (int i = 0; i < 320; i++)
	{
		vbp = i * 896;
		vbuf[vid.buf][vbp+15]  = 0x6040FF;
		vbuf[vid.buf][vbp+128] = 0x6040FF;
		show_memcycle(vid.memcpucyc[i], 0x40FF90, 1, vbp + 16);
		show_memcycle(vid.memvidcyc[i], 0xFFFF90, 0, 0);
		show_memcycle(vid.memtscyc[i], 0xFF9090, 0, 0);
	}
	
}

// Вызывается раз в кадр
void showleds()
{
   led_updtime = rdtsc();
   update_perf_led();

   if (temp.vidblock) return;

   pitch = temp.scx/4;

   if (statcnt) { statcnt--; text_i(rbuf + ((pitch/2-strlen(statusline)*6/8) & 0xFE) + (temp.scy-10)*pitch, statusline, 0x09); }

   if (!conf.led.enabled) return;

   if (temp.led.ay) ay_led();
   if (temp.led.perf) perf_led();
   if (temp.led.load) load_led();
   if (temp.led.input) input_led();
   if (temp.led.time) time_led();
#ifdef MOD_MONITOR
   if (temp.led.osw) debug_led();
#endif
#ifdef MOD_MEMBAND_LED
   if (temp.led.memband) memband_led();
#endif
   if (conf.led.flash_ay_kbd && hndKbdDev) ay_kbd();
   if (input.keymode == K_INPUT::KM_KEYSTICK) key_led();
}
