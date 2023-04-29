#include "std.h"
#include "emul.h"
#include "funcs.h"
#include "vars.h"
#include "draw.h"
#include "hard/memory.h"
#include "sound/sound.h"
#include "hard/tsconf.h"
#include "hard/gs/gs.h"
#include "emulkeys.h"
#include "op_system.h"
#include "op_noprefix.h"
#include "z80.h"

#include "sound/saa1099.h"
#include "sound/dev_moonsound.h"

extern VCTR vid;

namespace z80gs
{
void flush_gs_z80();
}

#ifdef MOD_FASTCORE
   namespace z80fast
   {
      #include "z80_main.inl"
   }
#else
   #define z80fast z80dbg
#endif

#ifdef MOD_DEBUGCORE
   namespace z80dbg
   {
      #define Z80_DBG
      #include "z80_main.inl"
      #undef Z80_DBG
   }
#else
   #define z80dbg z80fast
#endif

void out(unsigned port, u8 val);

u8 Rm(u32 addr)
{
    return z80fast::rm(addr);
}

void Wm(u32 addr, u8 val)
{
    z80fast::wm(addr, val);
}

u8 DbgRm(u32 addr)
{
    return z80dbg::rm(addr);
}

void DbgWm(u32 addr, u8 val)
{
    z80dbg::wm(addr, val);
}

void reset_sound(void)
{
   ay[0].reset();
   ay[1].reset();
   Saa1099.reset();
   zxmmoonsound.reset();

   if (conf.sound.ay_scheme == AY_SCHEME_CHRV)
        out(0xfffd,0xff);

   #ifdef MOD_GS
   if (conf.sound.gsreset)
       reset_gs();
   #endif
}

void reset(ROM_MODE mode)
{
   comp.pEFF7 &= conf.EFF7_mask;
   comp.pEFF7 |= EFF7_GIGASCREEN; // [vv] disable turbo
   {
        conf.frame = frametime;
        cpu.SetTpi(conf.frame);
//                if ((conf.mem_model == MM_PENTAGON)&&(comp.pEFF7 & EFF7_GIGASCREEN))conf.frame = 71680; //removed 0.37
        apply_sound();
   } //Alone Coder 0.36.4
   comp.t_states = 0; comp.frame_counter = 0;
   comp.p7FFD =  0;

   comp.ulaplus_mode=0;
   comp.ulaplus_reg=0;

   tsinit();

   if (conf.mem_model == MM_TSL)
		set_clk();		// turbo 2x (7MHz) for TS-Conf
   else
		turbo(1);		// turbo 1x (3.5MHz) for all other clones

   switch (mode)
   {
	case RM_SYS: {comp.ts.memconf = 4; break;}
	case RM_DOS: {comp.ts.memconf = 0; break;}
	case RM_128: {comp.ts.memconf = 0; break;}
	case RM_SOS: {comp.ts.memconf = 0; break;}
   }

   comp.flags = 0;
   comp.active_ay = 0;

   cpu.reset();
   reset_tape();
   reset_sound();

   load_spec_colors();

   comp.ide_hi_byte_r = 0;
   comp.ide_hi_byte_w = 0;
   comp.ide_hi_byte_w1 = 0;
   hdd.reset();
   input.buffer.Enable(false);

   if ((!conf.trdos_present && mode == RM_DOS) ||
       (!conf.cache && mode == RM_CACHE))
       mode = RM_SOS;

   set_mode(mode);
}

void set_clk(void)
{
	switch(comp.ts.zclk)
	{
		case 0: turbo(1); break;
		case 1: turbo(2); break;
		case 2: turbo(4); break;
		case 3: turbo(4); break;
	}
	comp.ts.intctrl.frame_len = (conf.intlen * cpu.rate) >> 8;
}
