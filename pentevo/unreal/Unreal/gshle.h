#pragma once

struct GSHLE
{
   unsigned char gsstat;// command and data bits
   unsigned char gscmd;
   unsigned char busy;  // don't play fx
   unsigned char mod_playing;

   unsigned modvol, fxvol; // module and FX master volumes
   unsigned used;       // used memory
   unsigned char *mod; unsigned modsize; // for modplayer
   unsigned total_fx;      // samples loaded
   unsigned cur_fx;        // selected sample

   unsigned char data_in[8];// data register
   unsigned char gstmp[8], *resptr; unsigned resmode; // from GS
   unsigned char *to_ptr; unsigned resmod2; // to GS

   unsigned char *streamstart; unsigned streamsize;
   unsigned char load_stream;

   unsigned char loadmod, loadfx; // leds flags

   unsigned char badgs[0x100]; // unrecognized commands
   unsigned note2rate[0x100];

   struct SAMPLE {
      unsigned char *start;
      unsigned loop, end;
      unsigned char volume, note;
   } sample[64];

   struct CHANNEL {
      unsigned char *start;
      unsigned loop, end, ptr;
      unsigned volume, freq;
      HSTREAM bass_ch;
      unsigned char busy;
   } chan[4];

   unsigned char in(unsigned char port);
   void out(unsigned char port, unsigned char byte);
   void reset();
   void reset_sound();
   void applyconfig();

   void set_busy(unsigned char newval);
   void start_fx(unsigned fx, unsigned chan, unsigned char vol, unsigned char note);
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
