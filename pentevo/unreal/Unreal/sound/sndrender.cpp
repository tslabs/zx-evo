#include "std.h"
#include "emul.h"
#include "vars.h"
#include "sndrender.h"

/*
   sound resampling core for Unreal Speccy project
   created under public domain license by SMT, jan.2006
*/

unsigned SNDRENDER::render(SNDOUT *src, unsigned srclen, unsigned clk_ticks, bufptr_t dst_start)
{
   start_frame(dst_start);
   for (unsigned index = 0; index < srclen; index++)
   {
      // if (src[index].timestamp > clk_ticks) continue; // wrong input data leads to crash
      update(src[index].timestamp, src[index].newvalue.ch.left, src[index].newvalue.ch.right);
   }
   return end_frame(clk_ticks);
}

const unsigned TICK_F = (1<<TICK_FF);

// b = 1+ln2(max_sndtick) = 1+ln2((max_sndfq*TICK_F)/min_intfq) = 1+ln2(48000*64/10) ~= 19.2;
// assert(b+MULT_C <= 32)


void SNDRENDER::start_frame(bufptr_t dst_start)
{
   SNDRENDER::dst_start = dstpos = dst_start;
   base_tick = tick;
   firstsmp = 4; //Alone Coder
}

void SNDRENDER::update(unsigned timestamp, unsigned l, unsigned r)
{
   if (!((l ^ mix_l) | (r ^ mix_r)))
       return;

//[vv]   unsigned endtick = (timestamp * mult_const) >> MULT_C;
   u64 endtick = (timestamp * (u64)sample_rate * TICK_F) / clock_rate;
   flush( (unsigned) (base_tick + endtick) );
   mix_l = l; mix_r = r;
}

unsigned SNDRENDER::end_frame(unsigned clk_ticks)
{
   // adjusting 'clk_ticks' with whole history will fix accumulation of rounding errors
   //u64 endtick = ((passed_clk_ticks + clk_ticks) * mult_const) >> MULT_C;
   u64 endtick = ((passed_clk_ticks + clk_ticks) * (u64)sample_rate * TICK_F) / clock_rate;
   flush( (unsigned) (endtick - passed_snd_ticks) );

   unsigned ready_samples = dstpos - dst_start;
   #ifdef SND_EXTERNAL_BUFFER
   if ((int)ready_samples < 0) ready_samples += SND_EXTERNAL_BUFFER_SIZE;
   #endif

   oldfrmleft = ((long)useleft + (long)olduseleft)/2; //Alone Coder
   oldfrmright = ((long)useright + (long)olduseright)/2; //Alone Coder

   tick -= (ready_samples << TICK_FF);
   passed_snd_ticks += (ready_samples << TICK_FF);
   passed_clk_ticks += clk_ticks;

   return ready_samples;
}

/*
unsigned SNDRENDER::end_empty_frame(unsigned clk_ticks)
{
   // adjusting 'clk_ticks' with whole history will fix accumulation of rounding errors
   //u64 endtick = ((passed_clk_ticks + clk_ticks) * mult_const) >> MULT_C;
   u64 endtick = ((passed_clk_ticks + clk_ticks) * (u64)sample_rate * TICK_F) / clock_rate;
   tick = (unsigned)(endtick-passed_snd_ticks); // flush(endtick-passed_snd_ticks);
   // todo: change dstpos!

   unsigned ready_samples = dstpos - dst_start;
   #ifdef SND_EXTERNAL_BUFFER
   if ((int)ready_samples < 0) ready_samples += SND_EXTERNAL_BUFFER_SIZE;
   #endif

   tick -= (ready_samples << TICK_FF);
   passed_snd_ticks += (ready_samples << TICK_FF);
   passed_clk_ticks += clk_ticks;

   return ready_samples;
}
*/

void SNDRENDER::set_timings(unsigned clock_rate, unsigned sample_rate)
{
   SNDRENDER::clock_rate = clock_rate;
   SNDRENDER::sample_rate = sample_rate;

   tick = 0; dstpos = dst_start = 0;
   passed_snd_ticks = passed_clk_ticks = 0;

   mult_const = (unsigned) (((u64)sample_rate << (MULT_C+TICK_FF)) / clock_rate);
}


static unsigned filter_diff[TICK_F*2];
const double filter_sum_full = 1.0, filter_sum_half = 0.5;
const unsigned filter_sum_full_u = (unsigned)(filter_sum_full * 0x10000),
               filter_sum_half_u = (unsigned)(filter_sum_half * 0x10000);

void SNDRENDER::flush(unsigned endtick)
{
   unsigned scale;
   if (!((endtick ^ tick) & ~(TICK_F-1)))
   {
      //same discrete as before
      scale = filter_diff[(endtick & (TICK_F-1)) + TICK_F] - filter_diff[(tick & (TICK_F-1)) + TICK_F];
      s2_l += mix_l * scale;
      s2_r += mix_r * scale;

      scale = filter_diff[endtick & (TICK_F-1)] - filter_diff[tick & (TICK_F-1)];
      s1_l += mix_l * scale;
      s1_r += mix_r * scale;

      tick = endtick;

   }
   else
   {
      scale = filter_sum_full_u - filter_diff[(tick & (TICK_F-1)) + TICK_F];

      unsigned sample_value;
      if (conf.soundfilter)
      {
          /*lame noise reduction by Alone Coder*/
          int templeft = mix_l*scale + s2_l;
          /*olduseleft = useleft;
          if (firstsmp)useleft=oldfrmleft,firstsmp--;
              else*/ useleft = ((long)templeft + (long)oldleft)/2;
          oldleft = templeft;
          int tempright = mix_r*scale + s2_r;
          /*olduseright = useright;
          if (firstsmp)useright=oldfrmright,firstsmp--;
              else*/ useright = ((long)tempright + (long)oldright)/2;
          oldright = tempright;
          sample_value = (useleft >> 16) + (useright & 0xFFFF0000);
          /**/
      }
      else
      {
          sample_value = ((mix_l*scale + s2_l) >> 16) +
                         ((mix_r*scale + s2_r) & 0xFFFF0000);
      }

      #ifdef SND_EXTERNAL_BUFFER
      SND_EXTERNAL_BUFFER[dstpos].sample += sample_value;
      dstpos = (dstpos+1) & (SND_EXTERNAL_BUFFER_SIZE-1);
      #else
      dstpos->sample = sample_value;
      dstpos++;
      #endif

      scale = filter_sum_half_u - filter_diff[tick & (TICK_F-1)];
      s2_l = s1_l + mix_l * scale;
      s2_r = s1_r + mix_r * scale;

      tick = (tick | (TICK_F-1))+1;

      if ((endtick ^ tick) & ~(TICK_F-1))
      {
         // assume filter_coeff is symmetric
         unsigned val_l = mix_l * filter_sum_half_u;
         unsigned val_r = mix_r * filter_sum_half_u;
         do
         {
            unsigned sample_value;
            if (conf.soundfilter)
            {
                /*lame noise reduction by Alone Coder*/
                int templeft = s2_l+val_l;
                /*olduseleft = useleft;
                if (firstsmp)useleft=oldfrmleft,firstsmp--;
                   else*/ useleft = ((long)templeft + (long)oldleft)/2;
                oldleft = templeft;
                int tempright = s2_r+val_r;
                /*olduseright = useright;
                if (firstsmp)useright=oldfrmright,firstsmp--;
                   else*/ useright = ((long)tempright + (long)oldright)/2;
                oldright = tempright;
                sample_value = (useleft >> 16) + (useright & 0xFFFF0000);
                /**/
            }
            else
            {
                sample_value = ((s2_l + val_l) >> 16) +
                               ((s2_r + val_r) & 0xFFFF0000); // save s2+val
            }

            #ifdef SND_EXTERNAL_BUFFER
            SND_EXTERNAL_BUFFER[dstpos].sample += sample_value;
            dstpos = (dstpos+1) & (SND_EXTERNAL_BUFFER_SIZE-1);
            #else
            dstpos->sample = sample_value;
            dstpos++;
            #endif

            tick += TICK_F;
            s2_l = val_l, s2_r = val_r; // s2=s1, s1=0;

         } while ((endtick ^ tick) & ~(TICK_F-1));
      }

      tick = endtick;

      scale = filter_diff[(endtick & (TICK_F-1)) + TICK_F] - filter_sum_half_u;
      s2_l += mix_l * scale;
      s2_r += mix_r * scale;

      scale = filter_diff[endtick & (TICK_F-1)];
      s1_l = mix_l * scale;
      s1_r = mix_r * scale;
   }
}

const double filter_coeff[TICK_F*2] =
{
   // filter designed with Matlab's DSP toolbox
   0.000797243121022152, 0.000815206499600866, 0.000844792477531490, 0.000886460636664257,
   0.000940630171246217, 0.001007677515787512, 0.001087934129054332, 0.001181684445143001,
   0.001289164001921830, 0.001410557756409498, 0.001545998595893740, 0.001695566052785407,
   0.001859285230354019, 0.002037125945605404, 0.002229002094643918, 0.002434771244914945,
   0.002654234457752337, 0.002887136343664226, 0.003133165351783907, 0.003391954293894633,
   0.003663081102412781, 0.003946069820687711, 0.004240391822953223, 0.004545467260249598,
   0.004860666727631453, 0.005185313146989532, 0.005518683858848785, 0.005860012915564928,
   0.006208493567431684, 0.006563280932335042, 0.006923494838753613, 0.007288222831108771,
   0.007656523325719262, 0.008027428904915214, 0.008399949736219575, 0.008773077102914008,
   0.009145787031773989, 0.009517044003286715, 0.009885804729257883, 0.010251021982371376,
   0.010611648461991030, 0.010966640680287394, 0.011314962852635887, 0.011655590776166550,
   0.011987515680350414, 0.012309748033583185, 0.012621321289873522, 0.012921295559959939,
   0.013208761191466523, 0.013482842243062109, 0.013742699838008606, 0.013987535382970279,
   0.014216593638504731, 0.014429165628265581, 0.014624591374614174, 0.014802262449059521,
   0.014961624326719471, 0.015102178534818147, 0.015223484586101132, 0.015325161688957322,
   0.015406890226980602, 0.015468413001680802, 0.015509536233058410, 0.015530130313785910,
   0.015530130313785910, 0.015509536233058410, 0.015468413001680802, 0.015406890226980602,
   0.015325161688957322, 0.015223484586101132, 0.015102178534818147, 0.014961624326719471,
   0.014802262449059521, 0.014624591374614174, 0.014429165628265581, 0.014216593638504731,
   0.013987535382970279, 0.013742699838008606, 0.013482842243062109, 0.013208761191466523,
   0.012921295559959939, 0.012621321289873522, 0.012309748033583185, 0.011987515680350414,
   0.011655590776166550, 0.011314962852635887, 0.010966640680287394, 0.010611648461991030,
   0.010251021982371376, 0.009885804729257883, 0.009517044003286715, 0.009145787031773989,
   0.008773077102914008, 0.008399949736219575, 0.008027428904915214, 0.007656523325719262,
   0.007288222831108771, 0.006923494838753613, 0.006563280932335042, 0.006208493567431684,
   0.005860012915564928, 0.005518683858848785, 0.005185313146989532, 0.004860666727631453,
   0.004545467260249598, 0.004240391822953223, 0.003946069820687711, 0.003663081102412781,
   0.003391954293894633, 0.003133165351783907, 0.002887136343664226, 0.002654234457752337,
   0.002434771244914945, 0.002229002094643918, 0.002037125945605404, 0.001859285230354019,
   0.001695566052785407, 0.001545998595893740, 0.001410557756409498, 0.001289164001921830,
   0.001181684445143001, 0.001087934129054332, 0.001007677515787512, 0.000940630171246217,
   0.000886460636664257, 0.000844792477531490, 0.000815206499600866, 0.000797243121022152
};

SNDRENDER::SNDRENDER()
{
   set_timings(SNDR_DEFAULT_SYSTICK_RATE, SNDR_DEFAULT_SAMPLE_RATE);

   static char diff_ready = 0;
   if (!diff_ready) {
      double sum = 0;
      for (int i = 0; i < TICK_F*2; i++) {
         filter_diff[i] = (int)(sum * 0x10000);
         sum += filter_coeff[i];
      }
      diff_ready = 1;
   }
}
