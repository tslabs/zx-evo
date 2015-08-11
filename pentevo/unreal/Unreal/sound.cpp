#include "std.h"
#include "emul.h"
#include "vars.h"
#include "gs.h"
#include "tape.h"
#include "config.h"
#include "sndcounter.h"

extern SNDRENDER sound;
extern SNDCHIP ay[2];
extern SNDCOUNTER sndcounter;

int spkr_dig = 0, mic_dig = 0, covFB_vol = 0, covDD_vol = 0, sd_l = 0, sd_r = 0;

void flush_dig_snd()
{
//   __debugbreak();
   if (temp.sndblock)
       return;
   unsigned mono = (spkr_dig+mic_dig+covFB_vol+covDD_vol);
//   printf("mono=%u\n", mono);
//[vv]   
sound.update(cpu.t - temp.cpu_t_at_frame_start, mono + sd_l, mono + sd_r);
}

void init_snd_frame()
{
   temp.cpu_t_at_frame_start = cpu.t;
//[vv]   
   sound.start_frame();
//   comp.tape.sound.start_frame(); //Alone Coder
   comp.tape_sound.start_frame(); //Alone Coder

   if (conf.sound.ay_scheme)
   {
      ay[0].start_frame();
      if (conf.sound.ay_scheme > AY_SCHEME_SINGLE)
          ay[1].start_frame();
   }

   Saa1099.start_frame();
   zxmmoonsound.start_frame();

   #ifdef MOD_GS
   init_gs_frame();
   #endif
}

float y_1[2] = { 0.0 };
i16 x_1[2] = { 0 };

void flush_snd_frame()
{
   tape_bit();
   #ifdef MOD_GS
   flush_gs_frame();
   #endif

   if (temp.sndblock)
       return;

   unsigned endframe = cpu.t - temp.cpu_t_at_frame_start;

   if (conf.sound.ay_scheme)
   { // sound chip present

      ay[0].end_frame(endframe);
      // if (conf.sound.ay_samples) mix_dig(ay[0]);

      if (conf.sound.ay_scheme > AY_SCHEME_SINGLE)
      {

         ay[1].end_frame(endframe);
         // if (conf.sound.ay_samples) mix_dig(ay[1]);

         if (conf.sound.ay_scheme == AY_SCHEME_PSEUDO)
         {
            u8 last = ay[0].get_r13_reloaded()? 13 : 12;
            for (u8 r = 0; r <= last; r++)
               ay[1].select(r), ay[1].write(0, ay[0].get_reg(r));
         }
      }

      if (savesndtype == 2)
      {
         if (!vtxbuf)
         {
            vtxbuf = (u8*)malloc(32768);
            vtxbufsize = 32768;
            vtxbuffilled = 0;
         }

         if (vtxbuffilled + 14 >= vtxbufsize)
         {
            vtxbufsize += 32768;
            vtxbuf = (u8*)realloc(vtxbuf, vtxbufsize);
         }

         for (u8 r = 0; r < 14; r++)
            vtxbuf[vtxbuffilled+r] = ay[0].get_reg(r);

         if (!ay[0].get_r13_reloaded())
            vtxbuf[vtxbuffilled+13] = 0xFF;

         vtxbuffilled += 14;
      }
   }
   Saa1099.end_frame(endframe);
   zxmmoonsound.end_frame(endframe);

   sound.end_frame(endframe);
   // if (comp.tape.play_pointer) // play tape pulses
//      comp.tape.sound.end_frame(endframe); //Alone Coder
   comp.tape_sound.end_frame(endframe); //Alone Coder
   // else comp.tape.sound.end_empty_frame(endframe);

   unsigned bufplay, n_samples;
   sndcounter.begin();

   sndcounter.count(sound);
//   sndcounter.count(comp.tape.sound); //Alone Coder
   sndcounter.count(comp.tape_sound); //Alone Coder
   if (conf.sound.ay_scheme)
   {
      sndcounter.count(ay[0]);
      if (conf.sound.ay_scheme > AY_SCHEME_SINGLE)
         sndcounter.count(ay[1]);
   }

   sndcounter.count(Saa1099);
   sndcounter.count(zxmmoonsound);

#ifdef MOD_GS
   #ifdef MOD_GSZ80
   if (conf.gs_type==1)
       sndcounter.count(z80gs::sound);
   #endif

   #ifdef MOD_GSBASS
   // if (conf.gs_type==2) { gs.mix_fx(); return; }
   #endif
#endif // MOD_GS
   sndcounter.end(bufplay, n_samples);

  for (unsigned k = 0; k < n_samples; k++, bufplay++)
  {
      u32 v = sndbuf[bufplay & (SNDBUFSIZE-1)];
      u32 Y;
      if (conf.RejectDC) // DC rejection filter
      {
          i16 x[2];
          float y[2];
          x[0] = i16(v & 0xFFFF);
          x[1] = i16(v >> 16U);
          y[0] = 0.995f * (x[0] - x_1[0]) + 0.99f * y_1[0];
          y[1] = 0.995f * (x[1] - x_1[1]) + 0.99f * y_1[1];
          x_1[0] = x[0];
          x_1[1] = x[1];
          y_1[0] = y[0];
          y_1[1] = y[1];
          Y = ((i16(y[1]) & 0xFFFF)<<16) | (i16(y[0]) & 0xFFFF);
      }
      else
      {
          Y = v;
      }

      sndplaybuf[k] = Y;
      sndbuf[bufplay & (SNDBUFSIZE-1)] = 0;
   } 

#if 0
//   printf("n_samples=%u\n", n_samples);
   for (unsigned k = 0; k < n_samples; k++, bufplay++)
   {
      sndplaybuf[k] = sndbuf[bufplay & (SNDBUFSIZE-1)];
/*
      if (sndplaybuf[k] == 0x20002000)
          __debugbreak();
*/
      sndbuf[bufplay & (SNDBUFSIZE-1)] = 0;
   }
#endif
   spbsize = n_samples*4;
//   assert(spbsize != 0);

return;

/*

   // count available samples and copy to sound buffer
   unsigned save_ticks = temp.snd_frame_ticks; // sound output limit = 1 frame
   save_ticks = min(save_ticks, sound.ready_samples());
   save_ticks = min(save_ticks, comp.ay->sound.ready_samples());
   save_ticks = min(save_ticks, comp.tape.sound.ready_samples());
   #ifdef MOD_GSZ80
   if (conf.gs_type == 1)
      save_ticks = min(save_ticks, z80gs::sound.ready_samples());
   #endif
/* // fx player always gives enough samples
   #ifdef MOD_GSBASS
   if (conf.gs_type == 2)
      for (int i = 0; i < 4; i++)
         save_ticks = min(save_ticks, gs.chan[i].sound_state.ready_samples());
   #endif
*/

}

void restart_sound()
{
//   printf("%s\n", __FUNCTION__);

   unsigned cpufq = conf.intfq * conf.frame;
   sound.set_timings(cpufq, conf.sound.fq);
//   comp.tape.sound.set_timings(cpufq, conf.sound.fq); //Alone Coder
   comp.tape_sound.set_timings(cpufq, conf.sound.fq); //Alone Coder
   if (conf.sound.ay_scheme)
   {
      ay[0].set_timings(cpufq, conf.sound.ayfq, conf.sound.fq);
      if (conf.sound.ay_scheme > AY_SCHEME_SINGLE) ay[1].set_timings(cpufq, conf.sound.ayfq, conf.sound.fq);
   }

   Saa1099.set_timings(cpufq, conf.sound.saa1099fq, conf.sound.fq);
   zxmmoonsound.set_timings(cpufq, 33868800, conf.sound.fq);

   // comp.tape.sound.clear();
   #ifdef MOD_GS
   reset_gs_sound();
   #endif

   sndcounter.reset();

   memset(sndbuf, 0, sizeof sndbuf);
}

void apply_sound()
{
   if (conf.sound.ay_scheme < AY_SCHEME_QUADRO) comp.active_ay = 0;

   load_ay_stereo();
   load_ay_vols();

   ay[0].set_chip((SNDCHIP::CHIP_TYPE)conf.sound.ay_chip);
   ay[1].set_chip((SNDCHIP::CHIP_TYPE)conf.sound.ay_chip);

   const SNDCHIP_VOLTAB *voltab = (SNDCHIP_VOLTAB*)&conf.sound.ay_voltab;
   const SNDCHIP_PANTAB *stereo = (SNDCHIP_PANTAB*)&conf.sound.ay_stereo_tab;
   ay[0].set_volumes(conf.sound.ay_vol, voltab, stereo);

   SNDCHIP_PANTAB reversed;
   if (conf.sound.ay_scheme == AY_SCHEME_PSEUDO) {
      for (int i = 0; i < 6; i++)
         reversed.raw[i] = stereo->raw[i^1]; // swap left/right
      stereo = &reversed;
   }
   ay[1].set_volumes(conf.sound.ay_vol, voltab, stereo);


   #ifdef MOD_GS
   apply_gs();
   #endif

   restart_sound();
}

/*
#define SAMPLE_SIZE (1024*3)
#define SAMPLE_T    256
int waveA[SAMPLE_SIZE], waveB[SAMPLE_SIZE], waveC[SAMPLE_SIZE];

void mix_dig(SNDCHIP &chip)
{
   unsigned base = sb_start_frame >> TICK_FF;
   for (unsigned i = 0; i < temp.snd_frame_samples; i++) {

      ta += fa; while (ta >= SAMPLE_SIZE*0x100) ta -= SAMPLE_SIZE*0x100;
      tb += fb; while (tb >= SAMPLE_SIZE*0x100) tb -= SAMPLE_SIZE*0x100;
      tc += fc; while (tc >= SAMPLE_SIZE*0x100) tc -= SAMPLE_SIZE*0x100;
      tn += fn;
      while (tn >= 0x10000) {
         ns = (ns*2+1) ^ (((ns>>16)^(ns>>13)) & 1);
         bitN = 0 - ((ns >> 16) & 1);
         tn -= 0x10000;
      }
      te += fe;
      while (te >= 0x10000) {
         env += denv;
         if (env & ~31) {
            unsigned mask = 1 << r_env;
            if (mask & ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<9)|(1<<15)))
               env = denv = 0;
            else if (mask & ((1<<8)|(1<<12)))
               env &= 31;
            else if (mask & ((1<<10)|(1<<14)))
               denv = -(int)denv, env = env + denv;
            else env = 31, denv = 0; //11,13
         }
         te -= 0x10000;
      }

      unsigned left = 0, right = 0, en, vol;

      en = (r_vA & 0x10) ? env : (r_vA & 0x0F)*2+1;
      vol = (bitN | bit3) & (waveA[ta/0x100] | bit0) & 0xFFFF;
      left += vol*vols[0][en], right += vol*vols[1][en];

      en = (r_vB & 0x10) ? env : (r_vB & 0x0F)*2+1;
      vol = (bitN | bit4) & (waveB[tb/0x100] | bit1) & 0xFFFF;
      left += vol*vols[2][en], right += vol*vols[3][en];

      en = (r_vC & 0x10) ? env : (r_vC & 0x0F)*2+1;
      vol = (bitN | bit5) & (waveC[tc/0x100] | bit2) & 0xFFFF;
      left += vol*vols[4][en], right += vol*vols[5][en];

      *(unsigned*)&sndbuf[(i+base) & (SNDBUFSIZE-1)] += (left >> 16) + (right & 0xFFFF0000);
   }
   sound.flush_empty();
}

#define PI 3.14159265359

double sin1(int i) {
   while (i > SAMPLE_SIZE) i -= SAMPLE_SIZE;
   if (i < SAMPLE_SIZE/2) return (double)i*2/SAMPLE_SIZE;
   return 2-(double)i*2/SAMPLE_SIZE;
}
double cos1(int i) {
   return 1-sin1(i);
}

int *wavs[3] = { waveA, waveB, waveC };
void make_samples()
{
   #define cl (0.35)
   #define cl2 (0.25)
   #define clip(x) (((x>cl) ? cl : (x < cl) ? -cl : x)/cl)
   #define clip2(x) ((x < -cl2) ? 0 : (x+cl2))
   for (int i = 0; i < SAMPLE_SIZE; i++) {
      double p1 = 0.8+0.2*sin1(i*4);
      double p2 = 0.7+0.3*cos1(i*2);
      double p3 = 0.9+0.1*sin1(i);
      double t = (double)(i % SAMPLE_T)*2*PI/SAMPLE_T;
//      #define fabs(x) (x)
      waveA[i] = (unsigned)(fabs(p1*clip(1+sin(3*t/2))*0.7+p3*clip(sin(t))+p1*sin(4*t)*0.25+p2*clip2(cos(1+6*t)))*0x3FFF);
      waveB[i] = (unsigned)(fabs(p1*clip(2+sin(3*t/2))*0.7+p3*clip(sin(t))+p1*sin(1+7*t/2)*0.4+p2*clip2(cos(2+5*t)))*0x3FFF);
      waveC[i] = (unsigned)(fabs(p1*clip(0.5+sin(3*t/2))*0.7+p3*clip(sin(t))+p1*sin(0.2+9*t/2)*0.6+p2*clip2(cos(3+5*t)))*0x3FFF);
//      #undef fabs
   }
   #undef clip
   #undef cl
   #undef cl2
   #undef clip2
   for (int ind = 0; ind < 3; ind++) {
      int *arr = wavs[ind], max = -0x7FFFFFFF, min = 0x7FFFFFFF;
      for (int i1 = 0; i1 < SAMPLE_SIZE; i1++) {
         if (arr[i1] > max) max = arr[i1];
         if (arr[i1] < min) min = arr[i1];
      }
      for (i1 = 0; i1 < SAMPLE_SIZE; i1++)
         arr[i1] = (int)(((double)arr[i1] - min)*0x10000/(max-min));
   }
}
*/
