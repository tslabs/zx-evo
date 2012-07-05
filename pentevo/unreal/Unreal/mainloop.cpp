#include "std.h"
#include "emul.h"
#include "vars.h"
#include "sound.h"
#include "draw.h"
#include "dx.h"
#include "dxr_rsm.h"
#include "leds.h"
#include "memory.h"
#include "snapshot.h"
#include "emulkeys.h"
#include "vs1001.h"
#include "z80.h"
#include "zxevo.h"
#include "util.h"

void spectrum_frame()
{
   if (!temp.inputblock || input.keymode != K_INPUT::KM_DEFAULT)
      input.make_matrix();

   init_snd_frame();
   init_frame();

   zxevo_set_readfont_pos(); // init readfont position to simulate correct font reading for zxevo -- lvd
   if (cpu.dbgchk)
   {
       cpu.SetDbgMemIf();
       z80dbg::z80loop();
   }
   else
   {
       cpu.SetFastMemIf();
       z80fast::z80loop();
   }
   if (modem.open_port)
       modem.io();

   flush_snd_frame();
   flush_frame();
   showleds();

   if (!cpu.iff1 || // int disabled in CPU
        ((conf.mem_model == MM_ATM710/* || conf.mem_model == MM_ATM3*/) && !(comp.pFF77 & 0x20))) // int disabled by ATM hardware -- lvd removed int disabling in pentevo (atm3)
   {
      unsigned char *mp = am_r(cpu.pc);
      if (cpu.halted)
      {
          strcpy(statusline, "CPU HALTED");
          statcnt = 10;
      }
      if (*(unsigned short*)mp == WORD2(0x18,0xFE) ||
          ((*mp == 0xC3) && *(unsigned short*)(mp+1) == (unsigned short)cpu.pc))
      {
         strcpy(statusline, "CPU STOPPED");
         statcnt = 10;
      }
   }

   comp.t_states += conf.frame;
   cpu.t -= conf.frame;
   cpu.eipos -= conf.frame;
   comp.frame_counter++;
}

void do_idle()
{
   static volatile unsigned long long last_cpu = rdtsc();
   for (;;)
   {
      asm_pause();
      volatile unsigned long long cpu = rdtsc();
      if ((cpu-last_cpu) >= temp.ticks_frame)
          break;

      if (conf.sleepidle)
      {
          ULONG Delay = ULONG(((temp.ticks_frame - (cpu-last_cpu)) * 1000ULL) / temp.cpufq);

          if (Delay != 0)
          {
              Sleep(Delay);
 //             printf("sleep: %lu\n", Delay);
              break;
          }
      }
   }
   last_cpu = rdtsc();
}

// version before frame resampler
//uncommented by Alone Coder
/*
void mainloop()
{
   unsigned char skipped = 0;
   for (;;)
   {
      if (skipped < temp.frameskip)
      {
          skipped++;
          temp.vidblock = 1;
      }
      else
          skipped = temp.vidblock = 0;

      temp.sndblock = !conf.sound.enabled;
      temp.inputblock = 0; //temp.vidblock; //Alone Coder
      spectrum_frame();

      // message handling before flip (they paint to rbuf)
      if (!temp.inputblock)
          dispatch(conf.atm.xt_kbd ? ac_main_xt : ac_main);
      if (!temp.vidblock)
          flip();
      if (!temp.sndblock)
      {
          do_sound();
          Vs1001.Play();

//          if (conf.sound.do_sound != do_sound_ds)
          do_idle();
      }
   }
}
*/

void mainloop(const bool &Exit)
{
   unsigned char skipped = 0;
   for (;!Exit;)
   {
      if (skipped < temp.frameskip)
      {
          skipped++;
          temp.vidblock = 1;
      }
      else
          skipped = temp.vidblock = 0;

      if (!temp.vidblock)
          flip();

      for (unsigned f = rsm.needframes[rsm.frame]; f; f--)
      {
         temp.sndblock = !conf.sound.enabled;
         temp.inputblock = temp.vidblock && conf.sound.enabled;
         spectrum_frame();
         VideoSaver();

         // message handling before flip (they paint to rbuf)
         if (!temp.inputblock)
         {
             dispatch(conf.atm.xt_kbd ? ac_main_xt : ac_main);
         }
         if (!temp.sndblock)
         {
             do_sound();
             Vs1001.Play();
         }
         if (rsm.mix_frames > 1)
         {
            memcpy(rbuf_s + rsm.rbuf_dst * rb2_offs, rbuf, temp.scx * temp.scy / 4);
            if (++rsm.rbuf_dst == rsm.mix_frames)
                rsm.rbuf_dst = 0;
         }
         if (!temp.sndblock)
         {
/*
             if (conf.sound.do_sound == do_sound_none)
                 do_idle();
*/
         }
      }

      if (++rsm.frame == rsm.period)
          rsm.frame = 0;

   }
   correct_exit();
}
