#include "std.h"
#include "emul.h"
#include "vars.h"
#include "gs.h"
#include "gsz80.h"
#include "vs1001.h"

#ifdef MOD_GS

unsigned gs_vfx[0x41];  // logarithmic + mixed with conf.sound.gs
TGsLed gsleds[8];

void make_gs_volume(unsigned level)
{
   for (int i = 0; i <= 0x40; i++) {
      //const double ln_0x40 = 4.15888308336;
      //gs_vfx[i] = (unsigned)(conf.sound.gs*log((double)(i*level/0x3F+1))/ln_0x40);
      gs_vfx[i] = conf.sound.gs_vol*i*level/(0x40*0x40);
   }
}

#if defined(MOD_GSZ80) || defined(MOD_GSBASS)
//#include "snd_bass.cpp"
#endif

#ifdef MOD_GSZ80
//#include "gsz80.cpp"
#endif

#ifdef MOD_GSBASS
//#include "gshlbass.cpp"
//#include "gshle.cpp"
#endif

u8 in_gs(u8 port)
{

#ifdef MOD_GSZ80
   if (conf.gs_type == 1) return z80gs::in_gs(port);
#endif

#ifdef MOD_GSBASS
   if (conf.gs_type == 2) return gs.in(port);
#endif

   return 0xFF;
}

void out_gs(u8 port, u8 byte)
{
#ifdef MOD_GSZ80
   if (conf.gs_type == 1) { z80gs::out_gs(port, byte); return; }
#endif

#ifdef MOD_GSBASS
   if (conf.gs_type == 2) { gs.out(port, byte); return; }
#endif
}

void reset_gs_sound()
{
#ifdef MOD_GSZ80
   z80gs::sound.set_timings(z80gs::GSCPUFQ, conf.sound.fq);
#endif

#ifdef MOD_GSBASS
   gs.reset_sound();
#endif
}

void reset_gs()
{
#ifdef MOD_GSZ80
   if (conf.gs_type == 1) { z80gs::reset(); return; }
#endif

#ifdef MOD_GSBASS
   if (conf.gs_type == 2) { gs.reset(); return; }
#endif
}

void apply_gs()
{
#ifdef MOD_GSZ80
   if (conf.gs_type == 1) z80gs::apply_gs();
#endif

#ifdef MOD_GSBASS
   if (conf.gs_type == 2) gs.applyconfig();
#endif
}

void init_gs()
{
#ifdef MOD_GS
   if (conf.gs_type != 0)
       BASS::Load();
#endif

#ifdef MOD_GSZ80
   make_gs_volume(); // for GS-Z80
#endif

   reset_gs();
}

void done_gs()
{
#ifdef MOD_GS
   Vs1001.ShutDown();
   if (conf.gs_type != 0)
       BASS::Unload();
#endif
}

void init_gs_frame()
{
#ifdef MOD_GSZ80
   if (conf.gs_type==1) z80gs::init_gs_frame();
#endif
}

void flush_gs_frame()
{
#ifdef MOD_GSZ80
   if (conf.gs_type==1) { z80gs::flush_gs_frame(); return; }
#endif

#ifdef MOD_GSBASS
   if (conf.gs_type==2) { gs.flush_gs_frame(); return; }
#endif
}

#endif  // MOD_GS
