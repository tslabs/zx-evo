#include "std.h"
#include "emul.h"
#include "vars.h"
#include "bass.h"
#include "snd_bass.h"
#include "gshle.h"
#include "gs.h"
#include "sound.h"

#ifdef MOD_GSBASS
void GSHLE::set_busy(u8 newval)
{
   busy = chan[0].busy = chan[1].busy = chan[2].busy = chan[3].busy = newval;
}

void GSHLE::reset()
{
   fxvol = modvol = 0x3F;
   make_gs_volume(fxvol);
   to_ptr = data_in; mod_playing = 0;
   used = 0; mod = 0; modsize = 0; set_busy(0);
   resetmod(); total_fx = 1;
   memset(sample, 0, sizeof sample);
   memset(chan,  0, sizeof chan);
   out(0xBB, 0x23); // send 'get pages' command
}

void GSHLE::applyconfig()
{
   double period = 6848, mx = 0.943874312682; // mx=1/root(2,12)
   double basefq = 7093789.0/2;
   int i; //Alone Coder 0.36.7
   for (/*int*/ i = 0; i < 96; i++, period *= mx)
      note2rate[i] = (unsigned)(basefq/period);
   for (; i < 0x100; i++) note2rate[i] = note2rate[i-1];
   if (BASS::Bass)
       setmodvol(modvol);
}

u8 GSHLE::in(u8 port)
{
   if (port == 0xBB) return gsstat;
   if (!resptr) return 0xFF; // no data available
   u8 byte = *resptr;
   if (resmode) resptr++, resmode--; // goto next byte
   return byte;
}

void GSHLE::out(u8 port, u8 byte)
{
   if (port == 0xB3) {
      if (!load_stream) {
         *to_ptr = byte;
         if (resmod2) to_ptr++, resmod2--;
         if (!resmod2) {
            if ((gscmd & 0xF8) == 0x88) start_fx(data_in[0], gscmd & 3, 0xFF, data_in[1]);
            if ((gscmd & 0xF8) == 0x90) start_fx(data_in[0], gscmd & 3, data_in[1], 0xFF);
            if ((gscmd & 0xF8) == 0x98) start_fx(data_in[0], gscmd & 3, data_in[2], data_in[1]);
            to_ptr = data_in;
         }
      } else {
         if (load_stream == 4) { // covox
            flush_dig_snd();
            covFB_vol = byte*conf.sound.gs_vol/0x100;
            return;
         }
         streamstart[streamsize++] = byte;
         if (load_stream == 1) loadmod = 1;
         else loadfx = cur_fx;
      }
      return;
   }
   // else command
   unsigned i;
   gsstat = 0x7E; resmode = 0; // default - 1 byte ready
   resptr = gstmp; to_ptr = data_in; resmod2 = 0;
   gscmd = byte;
   if (load_stream == 4) load_stream = 0; // close covox mode
   switch (byte) {
      case 0x0E: // LPT covox
         load_stream = 4;
         break;
      case 0xF3: case 0xF4: // reset
         reset_gs();
         break;
      case 0xF5:
         set_busy(1); break;
      case 0xF6:
         set_busy(0); break;
      case 0x20: // get total memory
         *(unsigned*)gstmp = 32768*15-16384;
         resmode = 2; gsstat = 0xFE;
         break;
      case 0x21: // get free memory
         *(unsigned*)gstmp = (32768*15-16384) - used;
         resmode = 2; gsstat = 0xFE;
         break;
      case 0x23: // get pages
         *gstmp = 14;
         break;
      case 0x2A: // set module vol
      case 0x35:
         *gstmp = modvol;
         modvol = *data_in;
         setmodvol(modvol);
         break;
      case 0x2B: // set FX vol
      case 0x3D:
         *gstmp = fxvol;
         fxvol = *data_in;
         make_gs_volume(fxvol);
         break;
      case 0x2E: // set FX
         cur_fx = *data_in;
         break;
      case 0x30: // load MOD
//         if (mod) break;
         reset_gs();
         streamstart = mod = GSRAM_M + used;
         streamsize = 0;
         *gstmp = 1; load_stream = 1;
         break;
      case 0x31: // play MOD
         restart_mod(0,0);
         break;
      case 0x32: // stop MOD
         mod_playing = 0;
         stop_mod();
         break;
      case 0x33: // continue MOD
         if (mod) mod_playing = 1, cont_mod();
         break;
      case 0x36: // data = #FF
         *gstmp = 0xFF;
         break;
      case 0x38: // load FX
      case 0x3E:
         if (total_fx == 64) break;
         cur_fx = *gstmp = total_fx;
         load_stream = (byte == 0x38) ? 2 : 3;
         streamstart = GSRAM_M + used;
         sample[total_fx].start = streamstart;
         streamsize = 0;
         break;
      case 0x39: // play FX
         start_fx(*data_in, 0xFF, 0xFF, 0xFF);
         break;
      case 0x3A: // stop fx
         for (i = 0; i < 4; i++)
            if (*data_in & (1<<i)) chan[i].start = 0;
         break;
      case 0x40: // set note
         sample[cur_fx].note = *data_in;
         break;
      case 0x41: // set vol
         sample[cur_fx].volume = *data_in;
         break;
      case 0x48: // set loop start
         resmod2 = 2;
         *(u8*)&sample[cur_fx].loop = *data_in;
         to_ptr = 1+(u8*)&sample[cur_fx].loop;
         break;
      case 0x49: // set loop end
         resmod2 = 2;
         *(u8*)&sample[cur_fx].end = *data_in;
         to_ptr = 1+(u8*)&sample[cur_fx].end;
         break;
      case 0x60: // get song pos
         *gstmp = (u8)modgetpos();
         break;
      case 0x61: // get pattern pos
         *gstmp = (u8)(modgetpos() >> 16);
         break;
      case 0x62: // get mixed pos
         i = modgetpos();
         *gstmp = ((i>>16) & 0x3F) | (i << 6);
         break;
      case 0x63: // get module notes
        resmode = 3; gsstat = 0xFE;
        break;
      case 0x64: // get module vols
        *(unsigned*)gstmp = 0;
        resmode = 3; gsstat = 0xFE;
        break;
     case 0x65: // jmp to pos
        restart_mod(*data_in,0);
        break;
     case 0x80: // direct play 1
     case 0x81:
     case 0x82:
     case 0x83:
        start_fx(*data_in, byte & 3, 0xFF, 0xFF);
        break;
     case 0x88: // direct play 2
     case 0x89:
     case 0x8A:
     case 0x8B:
     case 0x90: // direct play 3
     case 0x91:
     case 0x92:
     case 0x93:
        resmod2 = 1; to_ptr++;
        break;
     case 0x98: // direct play 4
     case 0x99:
     case 0x9A:
     case 0x9B:
        resmod2 = 2; to_ptr++;
        break;

      case 0xD2: // close stream
         if (!load_stream) break;
         // bug?? command #3E loads unsigned samples (REX 1,2)
//         if (load_stream == 3) // unsigned sample -> convert to unsigned
//            for (unsigned ptr = 0; ptr < streamsize; sample[total_fx].start[ptr++] ^= 0x80);
         if (load_stream == 1) { modsize = streamsize, init_mod(); }
         else {
            sample[total_fx].end = streamsize;
            sample[total_fx].loop = 0xFFFFFF;
            sample[total_fx].volume = 0x40;
            sample[total_fx].note = 60;
            //{char fn[200];sprintf(fn,"s-%d.raw",total_fx); FILE*ff=fopen(fn,"wb");fwrite(sample[total_fx].start,1,streamsize,ff);fclose(ff);}
            total_fx++;
         }
         used += streamsize;
         load_stream = 0;
         break;

      case 0x00: // reset flags - already done
      case 0x08:
      case 0xD1: // start stream
         break;
      default:
         badgs[byte] = 1;
         break;
   }
}

void GSHLE::start_fx(unsigned fx, unsigned ch, u8 vol, u8 note)
{  
   unsigned i; //Alone Coder 0.36.7
   if (!fx) fx = cur_fx; // fx=0 - use default
   if (vol == 0xFF) vol = sample[fx].volume;
   if (note == 0xFF) note = sample[fx].note;
   if (ch == 0xFF) { // find free channel
      for (/*unsigned*/ i = 0; i < 4; i++)
         if (!chan[i].start) ch = i;
      if (ch == 0xFF)
         for (i = 1, ch = 0; i < 4; i++)
            if (chan[i].ptr > chan[ch].ptr) ch = i;
   }
   chan[ch].volume = vol;
   chan[ch].start = sample[fx].start;
   chan[ch].loop = sample[fx].loop;
   chan[ch].end = sample[fx].end;
   chan[ch].ptr = 0;
   chan[ch].freq = note2rate[note];
   // ch0,1 - left, ch2,3 - right
   startfx(&chan[ch], (ch & 2)? 1.0f : -1.0f);
}

DWORD CALLBACK gs_render(HSTREAM handle, void *buffer, DWORD length, void *user)
{
   GSHLE::CHANNEL *ch = (GSHLE::CHANNEL*)user;

   if (!ch->start)
       return BASS_STREAMPROC_END;
   if (ch->busy)
   {
       memset(buffer, 0, length);
       return length;
   }
   unsigned sample_pos = 0;
   for (unsigned i = 0; i < length; i++)
   {
      ((BYTE*)buffer)[i] = ch->start[ch->ptr++];
      if (ch->ptr >= ch->end)
      {
         if (ch->end < ch->loop)
         {
             ch->start = 0;
             return i + BASS_STREAMPROC_END;
         }
         else
             ch->ptr = ch->loop;
      }
   }
   return length;
}
#endif
