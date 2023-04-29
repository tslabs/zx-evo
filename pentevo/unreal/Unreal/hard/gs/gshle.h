#pragma once
#include "sysdefs.h"

struct GSHLE
{
   u8 gsstat;// command and data bits
   u8 gscmd;
   u8 busy;  // don't play fx
   u8 mod_playing;

   unsigned modvol, fxvol; // module and FX master volumes
   unsigned used;       // used memory
   u8 *mod; unsigned modsize; // for modplayer
   unsigned total_fx;      // samples loaded
   unsigned cur_fx;        // selected sample

   u8 data_in[8];// data register
   u8 gstmp[8], *resptr; unsigned resmode; // from GS
   u8 *to_ptr; unsigned resmod2; // to GS

   u8 *streamstart; unsigned streamsize;
   u8 load_stream;

   u8 loadmod, loadfx; // leds flags

   u8 badgs[0x100]; // unrecognized commands
   unsigned note2rate[0x100];

   struct SAMPLE {
      u8 *start;
      unsigned loop, end;
      u8 volume, note;
   } sample[64];

   struct CHANNEL {
      u8 *start;
      unsigned loop, end, ptr;
      unsigned volume, freq;
      HSTREAM bass_ch;
      u8 busy;
   } chan[4];

   u8 in(u8 port);
   void out(u8 port, u8 byte);
   void reset();
   void reset_sound();
   void applyconfig();

   void set_busy(u8 newval);
   void start_fx(unsigned fx, unsigned chan, u8 vol, u8 note);
   void flush_gs_frame(); // calc volume values for leds

   HMUSIC hmod;

   void runBASS();
   void initChannels();
   void setmodvol(unsigned vol);
   void init_mod();
   void restart_mod(unsigned order, unsigned row);
   void startfx(CHANNEL *ch, float pan);
   void resetmod();
   void resetfx();
   DWORD modgetpos();
   void stop_mod();
   void cont_mod();
   void debug_note(unsigned i);

   void reportError(const char *err);
};
