#include "std.h"
#include "emul.h"
#include "vars.h"
#include "bass.h"
#include "snd_bass.h"
#include "gshle.h"
#include "gs.h"
#include "init.h"
#include "util.h"

#ifdef MOD_GSBASS
void GSHLE::reportError(const char *err)
{
   color(CONSCLR_ERROR);
   printf("BASS library reports error in %s\n", err);
   color(CONSCLR_ERRCODE);
   printf("error code is 0x%04X\n", BASS::ErrorGetCode());
   //exit();
}

void GSHLE::runBASS()
{
   static bool Initialized = false;

   if (Initialized)
       return;

   if (BASS::Init(-1, conf.sound.fq, BASS_DEVICE_LATENCY, wnd, 0))
   {
      DWORD len = BASS::GetConfig(BASS_CONFIG_UPDATEPERIOD);
      BASS_INFO info;
      BASS::GetInfo(&info);
      BASS::SetConfig(BASS_CONFIG_BUFFER, len + info.minbuf);
      color(CONSCLR_HARDITEM);
      printf("BASS device latency is ");
      color(CONSCLR_HARDINFO);
      printf("%dms\n", len + info.minbuf);
   }
   else
   {
      color(CONSCLR_WARNING);
      printf("warning: can't use default BASS device, trying silence\n");
      if (!BASS::Init(-2, 11025, 0, wnd, 0))
	  {
          errmsg("can't init BASS");
		  return;
	  }
   }

   Initialized = true;
   hmod = 0;
   for (int ch = 0; ch < 4; ch++)
       chan[ch].bass_ch = 0;
}

void GSHLE::reset_sound()
{
  // runBASS();
  // BASS_Stop(); // todo: move to silent state?
}


DWORD CALLBACK gs_render(HSTREAM handle, void *buffer, DWORD length, void *user);

void GSHLE::initChannels()
{
   if (chan->bass_ch)
       return;
   for (int ch = 0; ch < 4; ch++)
   {
      chan[ch].bass_ch = BASS::StreamCreate(11025, 1, BASS_SAMPLE_8BITS, gs_render, &chan[ch]);
      if (!chan[ch].bass_ch)
          reportError("BASS_StreamCreate()");
   }
}

void GSHLE::setmodvol(unsigned vol)
{
   if (!hmod)
       return;
   runBASS();
   float v = (vol * conf.sound.bass_vol) / float(8000 * 64);
   assert(v<=1.0);
   if (!BASS::ChannelSetAttribute(hmod, BASS_ATTRIB_VOL, v))
       reportError("BASS_ChannelSetAttribute() [music volume]");
}

void GSHLE::init_mod()
{
   runBASS();
   if (hmod)
       BASS::MusicFree(hmod);
   hmod = 0;
   hmod = BASS::MusicLoad(1, mod, 0, modsize, BASS_MUSIC_LOOP | BASS_MUSIC_POSRESET | BASS_MUSIC_RAMP, 0);
   if (!hmod)
       reportError("BASS_MusicLoad()");
}

void GSHLE::restart_mod(unsigned order, unsigned row)
{
   if (!hmod)
       return;
   if (!BASS::ChannelSetPosition(hmod, MAKELONG(order,row), BASS_POS_MUSIC_ORDER))
       reportError("BASS_ChannelSetPosition() [music]");
   if (!BASS::ChannelFlags(hmod, BASS_MUSIC_LOOP | BASS_MUSIC_POSRESET | BASS_MUSIC_RAMP, -1))
       reportError("BASS_ChannelFlags() [music]");
   BASS::Start();
   if (!BASS::ChannelPlay(hmod, FALSE/*TRUE*/))
       reportError("BASS_ChannelPlay() [music]"); //molodcov_alex 0.36.2

   mod_playing = 1;
}

void GSHLE::resetmod()
{
   if (hmod)
       BASS::MusicFree(hmod);
   hmod = 0;
}

void GSHLE::resetfx()
{
   runBASS();
   for (int i = 0; i < 4; i++)
   {
       if (chan[i].bass_ch)
       {
         BASS::StreamFree(chan[i].bass_ch);
         chan[i].bass_ch = 0;
       }
   }
}

DWORD GSHLE::modgetpos()
{
   runBASS();
   return (DWORD)BASS::ChannelGetPosition(hmod, BASS_POS_MUSIC_ORDER);
//   return BASS_MusicGetOrderPosition(hmod);
}

void GSHLE::stop_mod()
{
   runBASS();
   if (!hmod)
       return;
   if (BASS::ChannelIsActive(hmod) != BASS_ACTIVE_PLAYING)
       return;
   if (!BASS::ChannelPause(hmod))
       reportError("BASS_ChannelPause() [music]");
}

void GSHLE::cont_mod()
{
   runBASS();
   if (!hmod)
       return;
   if (!BASS::ChannelPlay(hmod, TRUE))
       reportError("BASS_ChannelPlay() [music]");
}

void GSHLE::startfx(CHANNEL *ch, float pan)
{
	runBASS();
   initChannels();

   float vol = (ch->volume * conf.sound.gs_vol) / float(8000*64);
   if (!BASS::ChannelSetAttribute(ch->bass_ch, BASS_ATTRIB_VOL, vol))
       reportError("BASS_ChannelSetAttribute() [vol]");
   if (!BASS::ChannelSetAttribute(ch->bass_ch, BASS_ATTRIB_FREQ, float(ch->freq)))
       reportError("BASS_ChannelSetAttribute() [freq]");
   if (!BASS::ChannelSetAttribute(ch->bass_ch, BASS_ATTRIB_PAN, pan))
       reportError("BASS_ChannelSetAttribute() [pan]");

   if (!BASS::ChannelPlay(ch->bass_ch, FALSE))
       reportError("BASS_ChannelPlay()");
}

void GSHLE::flush_gs_frame()
{
   unsigned lvl;
   if (!hmod || (lvl = BASS::ChannelGetLevel(hmod)) == -1) lvl = 0;

   gsleds[0].level = LOWORD(lvl) >> (15-4);
   gsleds[0].attrib = 0x0D;
   gsleds[1].level = HIWORD(lvl) >> (15-4);
   gsleds[1].attrib = 0x0D;

   for (int ch = 0; ch < 4; ch++)
   {
      if (chan[ch].bass_ch && (lvl = BASS::ChannelGetLevel(chan[ch].bass_ch)) != -1)
      {
         lvl = max(HIWORD(lvl), LOWORD(lvl));
         lvl >>= (15-4);
      }
      else
         lvl = 0;
      gsleds[ch+2].level = lvl;
      gsleds[ch+2].attrib = 0x0F;
   }
}

void GSHLE::debug_note(unsigned i)
{
	runBASS();
   GSHLE::CHANNEL ch = { 0 };
   ch.volume = 64; ch.ptr = 0;
   ch.start = sample[i].start;
   ch.loop = sample[i].loop;
   ch.end = sample[i].end;
   unsigned note = sample[i].note; if (note == 60) note = 50;
   ch.freq = note2rate[note];
   ch.bass_ch = BASS::StreamCreate(11025, 1, BASS_SAMPLE_8BITS, gs_render, &ch);
   startfx(&ch, 0);
   unsigned mx = (sample[i].loop < sample[i].end)? 5000 : 10000;
   for (unsigned j = 0; j < mx/256; j++)
   {
      if (!ch.start)
          break;
      Sleep(256);
   }
   BASS::StreamFree(ch.bass_ch);
}
#endif
