#include "std.h"
#include "emul.h"
#include "vars.h"
#include "wd93crc.h"
#include "util.h"

void WD1793::process()
{
   time = comp.t_states + cpu.t;
   // inactive drives disregard HLT bit
   if (time > seldrive->motor && (system & 0x08)) seldrive->motor = 0;
   if (seldrive->rawdata) status &= ~WDS_NOTRDY; else status |= WDS_NOTRDY;

   if (!(cmd & 0x80) || (cmd & 0xF0) == 0xD0) { // seek / step commands
      unsigned old_idx_status = idx_status;

      idx_status &= ~WDS_INDEX;

      status &= ~WDS_INDEX;

      if (state != S_IDLE)
      {
          status &= ~(WDS_TRK00 | WDS_INDEX);
          if (seldrive->motor && (system & 0x08)) status |= WDS_HEADL;
          if (!seldrive->track) status |= WDS_TRK00;
      }

      // todo: test spinning
      if (seldrive->rawdata && seldrive->motor && ((time+tshift) % (Z80FQ/FDD_RPS) < (Z80FQ*4/1000)))
      {
         if (state == S_IDLE)
         {
             if (time < idx_tmo)
                 status |= WDS_INDEX;
         }
         else
         {
             status |= WDS_INDEX;
         }

         idx_status |= WDS_INDEX; // index every turn, len=4ms (if disk present)
      }
   }

   for (;;)
   {

      switch (state)
      {

         // ----------------------------------------------------

         case S_IDLE:
            status &= ~WDS_BUSY;
            if (idx_cnt >= 15 || time > idx_tmo)
            {
                idx_cnt = 15;
                status &= WDS_NOTRDY;
                status |= WDS_NOTRDY;
                seldrive->motor = 0;
            }
            rqs = INTRQ;
            return;

         case S_WAIT:
            if (time < next)
                return;
            state = state2;
            break;

         // ----------------------------------------------------

         case S_DELAY_BEFORE_CMD:
            if (!conf.wd93_nodelay && (cmd & CMD_DELAY)) next += (Z80FQ*15/1000); // 15ms delay
            status = (status | WDS_BUSY) & ~(WDS_DRQ | WDS_LOST | WDS_NOTFOUND | WDS_RECORDT | WDS_WRITEP);
            state2 = S_CMD_RW; state = S_WAIT;
            break;

         case S_CMD_RW:
            if (((cmd & 0xE0) == 0xA0 || (cmd & 0xF0) == 0xF0) && conf.trdos_wp[drive]) {
               status |= WDS_WRITEP;
               state = S_IDLE;
               break;
            }

            if ((cmd & 0xC0) == 0x80 || (cmd & 0xF8) == 0xC0) {
               // read/write sectors or read am - find next AM
               end_waiting_am = next + 5*Z80FQ/FDD_RPS; // max wait disk 5 turns
               find_marker();
               break;
            }

            if ((cmd & 0xF8) == 0xF0) { // write track
               rqs = DRQ; status |= WDS_DRQ;
               next += 3*seldrive->t.ts_byte;
               state2 = S_WRTRACK; state = S_WAIT;
               break;
            }

            if ((cmd & 0xF8) == 0xE0) { // read track
               load(); rwptr = 0; rwlen = seldrive->t.trklen;
               state2 = S_READ; getindex();
               break;
            }

            // else unknown command
            state = S_IDLE;
            break;

        case S_FOUND_NEXT_ID:
            if (!seldrive->rawdata)
            { // no disk - wait again
               end_waiting_am = next + 5*Z80FQ/FDD_RPS;
        nextmk:
               find_marker();
               break;
            }
            if (next >= end_waiting_am)
            {
            nf:
                status |= WDS_NOTFOUND;
                state = S_IDLE;
                break;
            }
            if (foundid == -1)
                goto nf;

            status &= ~WDS_CRCERR;
            load();

            if (!(cmd & 0x80)) { // verify after seek
               if (seldrive->t.hdr[foundid].c != track) goto nextmk;
               if (!seldrive->t.hdr[foundid].c1) { status |= WDS_CRCERR; goto nextmk; }
               state = S_IDLE; break;
            }

            if ((cmd & 0xF0) == 0xC0) { // read AM
               rwptr = seldrive->t.hdr[foundid].id - seldrive->t.trkd;
               rwlen = 6;
         read_first_byte:
               data = seldrive->t.trkd[rwptr++]; rwlen--;
               rqs = DRQ; status |= WDS_DRQ;
               next += seldrive->t.ts_byte;
               state = S_WAIT;
               state2 = S_READ;
               break;
            }

            // else R/W sector(s)
            if (seldrive->t.hdr[foundid].c != track || seldrive->t.hdr[foundid].n != sector) goto nextmk;
            if ((cmd & CMD_SIDE_CMP_FLAG) && (((cmd >> CMD_SIDE_SHIFT) ^ seldrive->t.hdr[foundid].s) & 1)) goto nextmk;
            if (!seldrive->t.hdr[foundid].c1) { status |= WDS_CRCERR; goto nextmk; }

            if (cmd & 0x20) { // write sector(s)
               rqs = DRQ; status |= WDS_DRQ;
               next += seldrive->t.ts_byte*9;
               state = S_WAIT; state2 = S_WRSEC;
               break;
            }

            // read sector(s)
            if (!seldrive->t.hdr[foundid].data) goto nextmk; // Сектор без зоны данных
            if (!conf.wd93_nodelay)
                next += seldrive->t.ts_byte*(seldrive->t.hdr[foundid].data - seldrive->t.hdr[foundid].id); // Задержка на пропуск заголовка сектора и пробела между заголовком и зоной данных
            state = S_WAIT; state2 = S_RDSEC;
            break;

         case S_RDSEC:
            if (seldrive->t.hdr[foundid].data[-1] == 0xF8) status |= WDS_RECORDT; else status &= ~WDS_RECORDT;
            rwptr = seldrive->t.hdr[foundid].data - seldrive->t.trkd; // Смещение зоны данных сектора (в байтах) относительно начала трека
            rwlen = 128 << (seldrive->t.hdr[foundid].l & 3); // [vv]
            goto read_first_byte;

         case S_READ:
            if (notready())
                break;
            load();

            if (!seldrive->t.trkd)
            {
                status |= WDS_NOTFOUND;
                state = S_IDLE;
                break;
            }

            if (rwlen)
            {
               trdos_load = romled_time;
               if (rqs & DRQ)
                   status |= WDS_LOST;
               data = seldrive->t.trkd[rwptr++];
               rwlen--;
               rqs = DRQ;
               status |= WDS_DRQ;
               if (!conf.wd93_nodelay)
                   next += seldrive->t.ts_byte;
               else
                   next = time + 1;
               state = S_WAIT;
               state2 = S_READ;
            }
            else
            {
               if ((cmd & 0xE0) == 0x80)
               { // read sector
                  if (!seldrive->t.hdr[foundid].c2)
                      status |= WDS_CRCERR;
                  if (cmd & CMD_MULTIPLE)
                  {
                      sector++;
                      state = S_CMD_RW;
                      break;
                  }
               }
               
               if ((cmd & 0xF0) == 0xC0) // read address
               {
                  if (!seldrive->t.hdr[foundid].c1)
                      status |= WDS_CRCERR;
               }
               
               else if ((cmd & 0xF0) == 0xE0) // read track
                  status |= WDS_LOST;
               
               state = S_IDLE;
            }
            break;


         case S_WRSEC:
            load();
            if (rqs & DRQ) { status |= WDS_LOST; state = S_IDLE; break; }
            seldrive->optype |= 1;
            rwptr = seldrive->t.hdr[foundid].id + 6 + 11 + 11 - seldrive->t.trkd;
            for (rwlen = 0; rwlen < 12; rwlen++) seldrive->t.write(rwptr++, 0, 0);
            for (rwlen = 0; rwlen < 3; rwlen++)  seldrive->t.write(rwptr++, 0xA1, 1);
            seldrive->t.write(rwptr++, (cmd & CMD_WRITE_DEL)? 0xF8 : 0xFB, 0);
            rwlen = 128 << (seldrive->t.hdr[foundid].l & 3); // [vv]
            state = S_WRITE; break;

         case S_WRITE:
            if (notready()) break;
            if (rqs & DRQ) status |= WDS_LOST, data = 0;
            trdos_save = romled_time;
            seldrive->t.write(rwptr++, data, 0); rwlen--;
            if (rwptr == seldrive->t.trklen) rwptr = 0;
            seldrive->t.sf = JUST_SEEK; // invalidate sectors
            if (rwlen) {
               if (!conf.wd93_nodelay) next += seldrive->t.ts_byte;
               state = S_WAIT; state2 = S_WRITE;
               rqs = DRQ; status |= WDS_DRQ;
            } else {
               unsigned len = (128 << (seldrive->t.hdr[foundid].l & 3)) + 1; //[vv]
               u8 sc[2056];
               if (rwptr < len)
                  memcpy(sc, seldrive->t.trkd + seldrive->t.trklen - rwptr, rwptr), memcpy(sc + rwptr, seldrive->t.trkd, len - rwptr);
               else memcpy(sc, seldrive->t.trkd + rwptr - len, len);
               unsigned crc = wd93_crc(sc, len);
               seldrive->t.write(rwptr++, (BYTE)crc, 0);
               seldrive->t.write(rwptr++, (BYTE)(crc >> 8), 0);
               seldrive->t.write(rwptr, 0xFF, 0);
               if (cmd & CMD_MULTIPLE) { sector++, state = S_CMD_RW; break; }
               state = S_IDLE;
            }
            break;

         case S_WRTRACK:
            if (rqs & DRQ)
            {
                status |= WDS_LOST;
                state = S_IDLE;
                break;
            }
            seldrive->optype |= 2;
            state2 = S_WR_TRACK_DATA;
            start_crc = 0;
            getindex();
            end_waiting_am = next + 5 * Z80FQ /FDD_RPS;
         break;

         case S_WR_TRACK_DATA:
         {
            if (notready())
                break;
            trdos_format = romled_time;
            if (rqs & DRQ)
            {
                status |= WDS_LOST;
                data = 0;
            }
            seldrive->t.seek(seldrive, seldrive->track, side, JUST_SEEK);
            seldrive->t.sf = JUST_SEEK; // invalidate sectors

            if (!seldrive->t.trkd)
            {
                state = S_IDLE;
                break;
            }

            u8 marker = 0, byte = data;
            unsigned crc;
            switch(data)
            {
            case 0xF5:
                byte = 0xA1;
                marker = 1;
                start_crc = rwptr + 1;
            break;

            case 0xF6:
                byte = 0xC2;
                marker = 1;
            break;

            case 0xF7:
                crc = wd93_crc(seldrive->t.trkd + start_crc, rwptr - start_crc);
                byte = crc & 0xFF;
            break;
            }

            seldrive->t.write(rwptr++, byte, marker);
            rwlen--;
            if (data == 0xF7)
            {
                seldrive->t.write(rwptr++, (crc >> 8) & 0xFF, 0);
                rwlen--; // second byte of CRC16
            }

            if ((int)rwlen > 0)
            {
               if (!conf.wd93_nodelay)
                   next += seldrive->t.ts_byte;
               state2 = S_WR_TRACK_DATA;
               state = S_WAIT;
               rqs = DRQ;
               status |= WDS_DRQ;
               break;
            }
            state = S_IDLE;
            break;
         }

         // ----------------------------------------------------

         case S_TYPE1_CMD:
            status = (status | WDS_BUSY) & ~(WDS_DRQ | WDS_CRCERR | WDS_SEEKERR | WDS_WRITEP);
            rqs = 0;

            if (conf.trdos_wp[drive]) status |= WDS_WRITEP;
            seldrive->motor = next + 2*Z80FQ;

            state2 = S_SEEKSTART; // default is seek/restore
            if (cmd & 0xE0) { // single step
               if (cmd & 0x40) stepdirection = (cmd & CMD_SEEK_DIR) ? -1 : 1;
               state2 = S_STEP;
            }
            if (!conf.wd93_nodelay) next += 1*Z80FQ/1000;
            state = S_WAIT; break;


         case S_STEP:
         {
            trdos_seek = romled_time;

            // TRK00 sampled only in RESTORE command
            if (!seldrive->track && !(cmd & 0xF0)) { track = 0; state = S_VERIFY; break; }

            if (!(cmd & 0xE0) || (cmd & CMD_SEEK_TRKUPD)) track += stepdirection;
            seldrive->track += stepdirection;
            if (seldrive->track == (u8)-1) seldrive->track = 0;
            if (seldrive->track >= MAX_PHYS_CYL) seldrive->track = MAX_PHYS_CYL;
            seldrive->t.clear();

            static const unsigned steps[] = { 6,12,20,30 };
            if (!conf.wd93_nodelay)
			{
				next += steps[cmd & CMD_SEEK_RATE]*Z80FQ/1000;
				if ( conf.fdd_noise == 1 )
					Beep((stepdirection > 0)? 600 : 800, 2);
				else if ( conf.fdd_noise == 2 )
					PlaySound((stepdirection > 0)? "trk_inc.wav" : "trk_dec.wav",
							  0,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
			}

            state2 = (cmd & 0xE0)? S_VERIFY : S_SEEK;
            state = S_WAIT; break;
         }

         case S_SEEKSTART:
            if (!(cmd & 0x10)) track = 0xFF, data = 0;
            // state = S_SEEK; break;

         case S_SEEK:
            if (data == track)
            {
                state = S_VERIFY;
                break;
            }
            stepdirection = (data < track) ? -1 : 1;
            state = S_STEP;
            break;

         case S_VERIFY:
            if (!(cmd & CMD_SEEK_VERIFY))
            {
                status |= WDS_BUSY;
                state2 = S_IDLE;
                state = S_WAIT;
                next = time + 1;
                idx_tmo = next + 15 * Z80FQ/FDD_RPS; // 15 disk turns
                break;
            }
            end_waiting_am = next + 6*Z80FQ/FDD_RPS; // max wait disk 6 turns
            load();
            find_marker();
            break;


         // ----------------------------------------------------

         case S_RESET: // seek to trk0, but don't be busy
            if (!seldrive->track) state = S_IDLE;
            else seldrive->track--, seldrive->t.clear();
            // if (!seldrive->track) track = 0;
            next += 6*Z80FQ/1000;
            break;


         default:
            errexit("WD1793 in wrong state");
      }
   }
}

void WD1793::find_marker()
{
   if (conf.wd93_nodelay && seldrive->track != track)
       seldrive->track = track;
   load();

   foundid = -1;
   if (seldrive->motor && seldrive->rawdata)
   {
      unsigned div = seldrive->t.trklen*seldrive->t.ts_byte; // Длина дорожки в тактах cpu
      unsigned i = (unsigned)((next+tshift) % div) / seldrive->t.ts_byte; // Позиция байта соответствующего текущему такту на дорожке
      unsigned wait = -1;

      // Поиск заголовка минимально отстоящего от текущего байта
      for (unsigned is = 0; is < seldrive->t.s; is++)
      {
         unsigned pos = seldrive->t.hdr[is].id - seldrive->t.trkd; // Смещение (в байтах) заголовка относительно начала дорожки
         unsigned dist = (pos > i)? pos-i : seldrive->t.trklen+pos-i; // Расстояние (в байтах) от заголовка до текущего байта
         if (dist < wait)
         {
             wait = dist;
             foundid = is;
         }
      }

      if (foundid != -1)
          wait *= seldrive->t.ts_byte; // Задержка в тактах от текущего такта до такта чтения первого байта заголовка
      else
          wait = 10*Z80FQ/FDD_RPS;

      if (conf.wd93_nodelay && foundid != -1)
      {
         // adjust tshift, that id appares right under head
         unsigned pos = seldrive->t.hdr[foundid].id - seldrive->t.trkd + 2;
         tshift = (unsigned)(((pos * seldrive->t.ts_byte) - (next % div) + div) % div);
         wait = 100; // delay=0 causes fdc to search infinitely, when no matched id on track
      }

      next += wait;
   } // else no index pulses - infinite wait
   else
   {
       next = comp.t_states + cpu.t + 1;
   }

   if (seldrive->rawdata && next > end_waiting_am)
   {
       next = end_waiting_am;
       foundid = -1;
   }
   state = S_WAIT;
   state2 = S_FOUND_NEXT_ID;
}

char WD1793::notready()
{
   // fdc is too fast in no-delay mode, wait until cpu handles DRQ, but not more 'end_waiting_am'
   if (!conf.wd93_nodelay || !(rqs & DRQ))
       return 0;
   if (next > end_waiting_am)
       return 0;
   state2 = state;
   state = S_WAIT;
   next += seldrive->t.ts_byte;
   return 1;
}

void WD1793::getindex()
{
   unsigned trlen = seldrive->t.trklen*seldrive->t.ts_byte;
   unsigned ticks = (unsigned)((next+tshift) % trlen);
   if (!conf.wd93_nodelay) next += (trlen - ticks);
   rwptr = 0; rwlen = seldrive->t.trklen; state = S_WAIT;
}

void WD1793::load()
{
   seldrive->t.seek(seldrive, seldrive->track, side, LOAD_SECTORS);
}

u8 WD1793::in(u8 port)
{
   process();

   if (port & 0x80)
	   return rqs | 0x3F;

   if (port == 0x1F)
   {
	   rqs &= ~INTRQ;
	   return status & ((system & 8) ? 0xFF : ~WDS_HEADL);
   }

   if (port == 0x3F)
	   return track;

   if (port == 0x5F)
	   return sector;

   if (port == 0x7F)
   {
	   status &= ~WDS_DRQ;
	   rqs &= ~DRQ;
	   return data;
   }

   return 0xFF;
}

void WD1793::out(u8 port, u8 val)
{
   process();

   if (port == 0x1F)
   { // cmd

      // force interrupt
      if ((val & 0xF0) == 0xD0)
      {
         u8 Cond = (val & 0xF);
         next = comp.t_states + cpu.t;
         idx_cnt = 0;
         idx_tmo = next + 15 * Z80FQ/FDD_RPS; // 15 disk turns
         cmd = val;

         if (Cond == 0)
         {
             state = S_IDLE; rqs = 0;
             status &= ~WDS_BUSY;
             return;
         }

         if (Cond & 8) // unconditional int
         {
             state = S_IDLE; rqs = INTRQ;
             status &= ~WDS_BUSY;
             return;
         }

         if (Cond & 4) // int by idam (unimplemented yet)
         {
             state = S_IDLE; rqs = INTRQ;
             status &= ~WDS_BUSY;
             return;
         }

         if (Cond & 2) // int 1->0 rdy (unimplemented yet)
         {
             state = S_IDLE; rqs = INTRQ;
             status &= ~WDS_BUSY;
             return;
         }

         if (Cond & 1) // int 0->1 rdy (unimplemented yet)
         {
             state = S_IDLE; rqs = INTRQ;
             status &= ~WDS_BUSY;
             return;
         }

         return;
      }

      if (status & WDS_BUSY)
          return;
      cmd = val;
      next = comp.t_states + cpu.t;
      status |= WDS_BUSY;
      rqs = 0;
      idx_cnt = 0;
      idx_tmo = LLONG_MAX;

      //-----------------------------------------------------------------------

      if (cmd & 0x80) // read/write command
      {
         // abort if no disk
         if (status & WDS_NOTRDY)
         {
             state2 = S_IDLE;
             state = S_WAIT;
             next = comp.t_states + cpu.t + Z80FQ/FDD_RPS;
             rqs = INTRQ;
             return;
         }

         // continue disk spinning
         if (seldrive->motor || conf.wd93_nodelay) seldrive->motor = next + 2*Z80FQ;

         state = S_DELAY_BEFORE_CMD;
         return;
      }

      // else seek/step command
      state = S_TYPE1_CMD;
      return;
   }

   //=======================================================================

   if (port == 0x3F)
   {
       track = val;
       return;
   }

   if (port == 0x5F)
   {
       sector = val;
       return;
   }

   if (port == 0x7F)
   {
       data = val;
       rqs &= ~DRQ;
       status &= ~WDS_DRQ;
       return;
   }

   if (port & 0x80) // FF
   { // system
      drive = val & 3;
      seldrive = &fdd[drive];
      seldrive->t.clear();

      if (conf.mem_model == MM_TSL && comp.ts.vdos)
        return;

      side = ~(val >> 4) & 1;

      if (!(val & 0x04))
      { // reset
         status = WDS_NOTRDY;
         rqs = INTRQ;
         seldrive->motor = 0;
         state = S_IDLE;
         idx_cnt = 0;
         idx_status = 0;

         #if 0 // move head to trk00
         steptime = 6 * (Z80FQ / 1000); // 6ms
         next += 1*Z80FQ/1000; // 1ms before command
         state = S_RESET;
           //seldrive->track = 0;
         #endif
      }
      else
      {
          if ((system ^ val) & SYS_HLT) // hlt 0->1
          {
              if (!(status & WDS_BUSY))
              {
                  idx_cnt++;
              }
          }
      }
      system = val;
   }
}

void WD1793::trdos_traps()
{
   unsigned pc = (cpu.pc & 0xFFFF);
   if (pc < 0x3DFD) return;

   // Позиционирование на соседнюю дорожку (пауза)
   if (pc == 0x3DFD && bankr[0][0x3DFD] == 0x3E && bankr[0][0x3DFF] == 0x0E)
   {
       cpu.pc = cpu.DbgMemIf->rm(cpu.sp++);
       cpu.pc |= (cpu.DbgMemIf->rm(cpu.sp++) << 8);
       cpu.a = 0;
       cpu.c = 0;
   }

   // Позиционирование на произвольную дорожку (пауза)
   if (pc == 0x3EA0 && bankr[0][0x3EA0] == 0x06 && bankr[0][0x3EA2] == 0x3E)
   {
       cpu.pc = cpu.DbgMemIf->rm(cpu.sp++);
       cpu.pc |= (cpu.DbgMemIf->rm(cpu.sp++) << 8);
       cpu.a = 0;
       cpu.b = 0;
   }

   if (pc == 0x3E01 && bankr[0][0x3E01] == 0x0D)
		{ cpu.a = cpu.c = 1; return; } // no delays
   if (pc == 0x3FEC && bankr[0][0x3FED] == 0xA2 && (state == S_READ || (state2 == S_READ && state == S_WAIT)))
   {
      trdos_load = romled_time;
      if (rqs & DRQ) {
         cpu.DbgMemIf->wm(cpu.hl, data); // move byte from controller
         cpu.hl++, cpu.b--;
         rqs &= ~DRQ; status &= ~WDS_DRQ;
      }

      if (seldrive->t.trkd)
      {
          while (rwlen) { // move others
             cpu.DbgMemIf->wm(cpu.hl, seldrive->t.trkd[rwptr++]); rwlen--;
             cpu.hl++; cpu.b--;
          }
      }
      cpu.pc += 2; // skip INI
      return;
   }
   if (pc == 0x3FD1 && bankr[0][0x3FD2] == 0xA3 &&
              (rqs & DRQ) && (rwlen>1) && (state == S_WRITE || (state2 == S_WRITE && state == S_WAIT))) {
      trdos_save = romled_time;
      while (rwlen > 1) {
         seldrive->t.write(rwptr++, cpu.DbgMemIf->rm(cpu.hl), 0); rwlen--;
         cpu.hl++; cpu.b--;
      }
      cpu.pc += 2; // skip OUTI
      return;
   }
}
