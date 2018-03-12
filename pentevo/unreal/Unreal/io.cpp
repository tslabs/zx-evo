#include "std.h"
#include "emul.h"
#include "funcs.h"
#include "vars.h"
#include "draw.h"
#include "memory.h"
#include "atm.h"
#include "sound.h"
#include "gs.h"
#include "sdcard.h"
#include "zc.h"
#include "z80.h"
#include "tape.h"
#include "zxevo.h"
#include "sound/ayx32.h"
#include "util.h"

#ifdef LOG_FE_OUT
  extern FILE *f_log_FE_in;
#endif
#ifdef LOG_FE_IN
  extern FILE *f_log_FE_out;
#endif

bool assert_dma_active()
{
  bool rc = true;

  if (comp.ts.dma.state != DMA_ST_NOP)
  {
    color(CONSCLR_WARNING);
    printf("Cannot write to DMA when it is active! PC = %02X:%04X\r\n", comp.ts.page[cpu.pc >> 14], cpu.pc);
    color(CONSCLR_DEFAULT);
    rc = false;
  }

  return rc;
}

void out(unsigned port, u8 val)
{

  port &= 0xFFFF;
  u8 p1 = (port & 0xFF);          // lower 8 bits of port address
  u8 p2 = ((port >> 8) & 0xFF);   // higher 8 bits of port address
  brk_port_out = port;
  brk_port_val = val;

  // if (p1 == 0xFD)
  // printf("out (%04X), %02X\n", port, val);

  if (conf.ulaplus != UPLS_NONE)
  {
    /* ULA+ register select */
    if (port == 0xBF3B)
    {
    comp.ulaplus_reg = val;
    }

    /* ULA+ data */
    if (port == 0xFF3B)
    {
      switch (comp.ulaplus_reg & 0xC0)
      {
        case 0:  // CRAM
          comp.ulaplus_cram[comp.ulaplus_reg] = val;
        break;

        case 64:  // MODE
          comp.ulaplus_mode = val & 1;
        break;

        default:
          break;
      }
    }
  }

  #ifdef MOD_GS
  // 10111011 | BB
  // 10110011 | B3
  // 00110011 | 33
  if (p1 == 0x33 && conf.gs_type) // 33
  {
    out_gs(port, val);
    return;
  }

  if ((port & 0xF7) == 0xB3 && conf.gs_type) // BB, B3
  {
    out_gs(port, val);
    return;
  }
  #endif

  // ZXM-MoonSound
  if ( conf.sound.moonsound &&
  !(comp.flags & CF_DOSPORTS) &&
  (conf.mem_model == MM_PROFI ? !(comp.pDFFD & 0x80) : 1) &&
    (((p1 & 0xFC) == 0xC4) ||
      ((p1 & 0xFE) == 0x7E)) )
  {
    if ( zxmmoonsound.write( port, val ) )
    return;
  }

  // z-controller
  if (conf.mem_model == MM_ATM3)
  {
    if (((port & 0x80FF) == 0x8057) && (comp.flags & CF_DOSPORTS))
      return;

    if (conf.zc && (p1 == 0x57))
    {
      Zc.Wr(port, val);
      return;
    }
  }
  else if (conf.zc && ((p1 == 0x57) || (p1 == 0x77)))
  {
    Zc.Wr(port, val);
    return;
  }

  // divide на nemo портах
  if (conf.ide_scheme == IDE_NEMO_DIVIDE)
  {
      if ((port & 0x1E) == 0x10) // rrr1000x
      {
          if (p1 == 0x11)
          {
              comp.ide_write = val;
              comp.ide_hi_byte_w = 0;
              comp.ide_hi_byte_w1 = 1;
              return;
          }

          if ((port & 0xFE) == 0x10)
          {
              comp.ide_hi_byte_w ^= 1;

              if (comp.ide_hi_byte_w1) // Была запись в порт 0x11 (старший байт уже запомнен)
              {
                  comp.ide_hi_byte_w1 = 0;
              }
              else
              {
                  if (comp.ide_hi_byte_w) // Запоминаем младший байт
                  {
                      comp.ide_write = val;
                      return;
                  }
                  else // Меняем старший и младший байты местами (как этого ожидает write_hdd_5)
                  {
                      u8 tmp = comp.ide_write;
                      comp.ide_write = val;
                      val = tmp;
                  }
              }
          }
          else
          {
              comp.ide_hi_byte_w = 0;
          }
          goto write_hdd_5;
      }
      else if (p1 == 0xC8)
      {
          return hdd.write(8, val);
      }
  }

   if ((conf.mem_model == MM_LSY256) && (p1 == 0x7B))
   {
    comp.pLSY256 = val;
    set_banks();
   }

  if ((conf.mem_model == MM_TSL) && (p1 == 0xAF))
    ts_ext_port_wr(p2, val);

   if (conf.mem_model == MM_ATM3)
   {
       // Порт расширений АТМ3
       if (p1 == 0xBF)
       {
           if ((comp.pBF ^ val) & comp.pBF & 8) // D3: 1->0
               nmi_pending  = 1;
           comp.pBF = val;
           set_banks();
           return;
       }

       // Порт разблокировки RAM0 АТМ3
       if (p1 == 0xBE)
       {
           comp.pBE = 2; // счетчик для выхода из nmi
           return;
       }

       // NMI breakpoints ATM3
       if (p1 == 0xBD)
     {
      if (p2 & 1)
        comp.pBDh = val; // BP Address MSB
      else
        comp.pBDl = val; // BP Address LSB
      return;
       }
   }

   if (comp.flags & CF_DOSPORTS)
   {
      if (conf.mem_model == MM_ATM3 && (p1 & 0x1F) == 0x0F && !(((p1 >> 5) - 1) & 4))
      {
          // 2F = 001|01111b
          // 4F = 010|01111b
          // 6F = 011|01111b
          // 8F = 100|01111b
          comp.wd_shadow[(p1 >> 5) - 1] = val;
      }


      if (conf.ide_scheme == IDE_ATM && (port & 0x1F) == 0x0F)
      {
         if (port & 0x100) { comp.ide_write = val; return; }
      write_hdd_5:
         port >>= 5;
      write_hdd:
         port &= 7;
         if (port)
             hdd.write(port, val);
         else
             hdd.write_data(val | (comp.ide_write << 8));
         return;
      }

      if ((port & 0x18A3) == (0xFFFE & 0x18A3))
      { // SMUC
         if (conf.smuc)
         {
            if ((port & 0xA044) == (0xDFBA & 0xA044))
            { // clock
               if (comp.pFFBA & 0x80)
                   cmos_write(val);
               else
                   comp.cmos_addr = val;
               return;
            }
            if ((port & 0xA044) == (0xFFBA & 0xA044))
            { // SMUC system port
               if ((val & 1) && (conf.ide_scheme == IDE_SMUC))
                   hdd.reset();
               comp.nvram.write(val);
               comp.pFFBA = val;
               return;
            }
            if ((port & 0xA044) == (0x7FBA & 0xA044))
            {
                comp.p7FBA = val;
                return;
            }
         }
         if ((port & 0x8044) == (0xFFBE & 0x8044) && conf.ide_scheme == IDE_SMUC)
         { // FFBE, FEBE
            if (comp.pFFBA & 0x80)
            {
                if (!(port & 0x100))
                    hdd.write(8, val); // control register
                return;
            }

            if (!(port & 0x2000))
            {
                comp.ide_write = val;
                return;
            }
            port >>= 8;
                goto write_hdd;
         }
      }

      if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3)
      {
         // x7f7 ATM3 4Mb memory manager
         if ((conf.mem_model == MM_ATM3) && ((port & 0x3FFF) == 0x37F7))
         {
             unsigned idx = ((comp.p7FFD & 0x10) >> 2) | ((port >> 14) & 3);
             comp.pFFF7[idx] = (comp.pFFF7[idx] & ~0x1FF) | (val ^ 0xFF); // always ram
             set_banks();
             return;
         }

         if (p1 == 0x77) // xx77
         {
       set_atm_FF77(port, val);
       set_turbo();
       init_raster();
             return;
         }

         // lvd fix: pentevo hardware decodes fully only low byte,
     // so using eff7 in shadow mode lead to outting to fff7,
         // unlike this was in unreal!
         u32 mask = (conf.mem_model == MM_ATM3) ? /*0x3FFF*/ 0x0FFF : 0x00FF;
         if ((port & mask) == (0x3FF7 & mask)) // xff7
         {
             comp.pFFF7[((comp.p7FFD & 0x10) >> 2) | ((port >> 14) & 3)] = (((val & 0xC0) << 2) | (val & 0x3F)) ^ 0x33F;
             set_banks();
             return;
         }

         if ((p1 & 0x9F) == 0x9F && !(comp.aFF77 & 0x4000))
             atm_writepal(val); // don't return - write to TR-DOS system port
      }

      if (conf.mem_model == MM_PROFI)
      {
          if ((comp.p7FFD & 0x10) && (comp.pDFFD & 0x20)) // modified ports
          {
              if ((p1 & 0xE3) == 0x23) // port FF
              {
                  comp.wd.out(0xFF, val);
                  return;
              }

        switch ( p1 & 0x9F )
        {
          case 0x83:  // WG93
          comp.wd.out((p1 & 0x60) | 0x1F, val);
          return;

        case 0x87:  // VV55
          if ( conf.sound.covoxProfi_vol > 0 )
          {
            // with covox
            flush_dig_snd();
            if ( (port & 0x60) == 0x40 )
              covProfiL = (val*conf.sound.covoxProfi_vol) >> 8;
            if ( (port & 0x60) == 0x20 )
              covProfiR = (val*conf.sound.covoxProfi_vol) >> 8;
          }
          return;

        case 0x9F:  // RTC
          if ( conf.cmos )
          {
            if (port & 0x20)
            {
              comp.cmos_addr = val;
              return;
            }
            cmos_write(val);
            return;
          }
          break;

        case 0x8B:  // IDE (AB=10101011, CB=11001011, EB=11101011)
          if ( conf.ide_scheme == IDE_PROFI )
          {
            if (p1 & 0x40)
            {    // cs1
              if (!(p1 & 0x20))
              {
                comp.ide_write = val;
                return;
              }
              port >>= 8;
              goto write_hdd;
            }

            // cs3
            if (p1 & 0x20)
            {
              if (((port>>8) & 7) == 6)
                hdd.write(8, val);
              return;
            }
          }
          break;
        }
          }
          else
          {
              // BDI ports
              if ((p1 & 0x83) == 0x03)  // WD93 ports 1F, 3F, 5F, 7F
              {
                  comp.wd.out((p1 & 0x60) | 0x1F,val);
                  return;
              }

              if ((p1 & 0xE3) == ((comp.pDFFD & 0x20) ? 0xA3 : 0xE3)) // port FF
              {
                  comp.wd.out(0xFF,val);
                  return;
              }
          }
      } // profi

      if (conf.mem_model == MM_QUORUM /* && !(comp.p00 & Q_TR_DOS)*/) // cpm ports
      {
          if ((p1 & 0xFC) == 0x80) // 80, 81, 82, 83
          {
              p1 = ((p1 & 3) << 5) | 0x1F;

              comp.wd.out(p1, val);
              return;
          }

          if (p1 == 0x85) // 85
          {
//            01 -> 00 A
//            10 -> 01 B
//            00 -> 11 D (unused)
//            11 -> 11 D (unused)
              static const u8 drv_decode[] = { 3, 0, 1, 3 };
              u8 drv = drv_decode[val & 3];
              comp.wd.out(0xFF, ((val & ~3) ^ 0x10) | drv);
              return;
          }
      } // quorum
      else if ((p1 & 0x1F) == 0x1F) // 1F, 3F, 5F, 7F, FF
      {
        if (conf.mem_model == MM_TSL)
        {
          if (comp.ts.vdos)
          { // vdos_on
            if (p1 == 0xFF)
            { // out vgsys
              comp.wd.out(p1, val);
              return;
            }
            else
            { // out vg
              comp.ts.vdos = 0;
              set_banks();
              return;
            }
          }
          else if ((1<<comp.wd.drive) & comp.ts.fddvirt)
          { // vdos_off
            comp.ts.vdos = 1;
            set_banks();
            return;
          }
        }
        // physical drive
        comp.wd.out(p1, val);
        return;
      }
      // don't return - out to port #FE works in trdos!
   }
   else // не dos
   {
         // VG93 free access in TS-Conf (FDDVirt.7 = 1)
         if ((conf.mem_model == MM_TSL) && (comp.ts.fddvirt & 0x80) && ((p1 & 0x1F) == 0x1F)) // 1F, 3F, 5F, 7F, FF
           comp.wd.out(p1, val);

         if (((port & 0xA3) == 0xA3) && (conf.ide_scheme == IDE_DIVIDE))
         {
             if (p1 == 0xA3)
             {
                 comp.ide_hi_byte_w ^= 1;
                 if (comp.ide_hi_byte_w)
                 {
                     comp.ide_write = val;
                     return;
                 }
                 u8 tmp = comp.ide_write;
                 comp.ide_write = val;
                 val = tmp;
             }
             else
             {
                 comp.ide_hi_byte_w = 0;
             }
             port >>= 2;
             goto write_hdd;
         }

         if ((u8)port == 0x1F && conf.sound.ay_scheme == AY_SCHEME_POS)
         {
             comp.active_ay = val & 1;
             return;
         }

         if (!(port & 6) && (conf.ide_scheme == IDE_NEMO || conf.ide_scheme == IDE_NEMO_A8))
         {
             unsigned hi_byte = (conf.ide_scheme == IDE_NEMO)? (port & 1) : (port & 0x100);
             if (hi_byte)
             {
                 comp.ide_write = val;
                 return;
             }
             if ((port & 0x18) == 0x08)
             {
                 if ((port & 0xE0) == 0xC0)
                     hdd.write(8, val);
                 return;
             } // CS1=0,CS0=1,reg=6
             if ((port & 0x18) != 0x10)
                 return; // invalid CS0,CS1
             goto write_hdd_5;
         }

     if ( conf.mem_model == MM_PROFI )
     {
      if ( (port & 0x83) == 0x03 ) // VV55
      {
        if ( conf.sound.covoxProfi_vol > 0 )
        {
          // with covox
          flush_dig_snd();
          if ( (port & 0x60) == 0x40 )
            covProfiL = (val*conf.sound.covoxProfi_vol) >> 8;
          if ( (port & 0x60) == 0x20 )
            covProfiR = (val*conf.sound.covoxProfi_vol) >> 8;
        }
        return;
      }
    }
   }

   if (p1 == 0x00 && conf.mem_model == MM_QUORUM)
   {
       comp.p00 = val;
       set_banks();
       return;
   }

  if ( conf.mem_model == MM_GMX )
  {
    if ((port & 0xFF) == 0x00)
    {
      comp.p00 = val;
      if (comp.p00 & 8)
      {
        comp.gmx_magic_shift = 0x88 | (comp.p00 & 7);

        if (!(comp.p00 & 0x10))
          cpu.reset();
          //reset(RM_NOCHANGE);
      }
      return;
    }

    if ((p1 & 0x23) == (0xFD & 0x23))
    {
      switch ( p2 )
      {
        case 0x1F:
          comp.p1FFD = val;
          set_banks();
          return;
        case 0xDF:
          comp.pDFFD = val;
          set_banks();
          return;
        case 0x7E:
          if (comp.p00 & 0x10)
            comp.p7EFD = (val&0x8F) | (comp.p7EFD&0x70);
          else
            comp.p7EFD = val;
          turbo(comp.p7EFD & 0x80 ? 2 : 1);
          set_banks();
          init_raster();
          return;
        case 0x78:
          comp.p78FD = val;
          set_banks();
          return;
        case 0x7A:
          comp.p7AFD = val;
          return;
        case 0x7C:
          comp.p7CFD = val;
          return;
      }
    }
  }

   #ifdef MOD_VID_VD
   if ((u8)port == 0xDF)
   {
       comp.pVD = val;
       comp.vdbase = (comp.pVD & 4)? vdmem[comp.pVD & 3] : 0;
       return;
   }
   #endif

   // port #FE
   bool pFE;

   // scorp  xx1xxx10 /dos=1 (sc16 green)
   if ((conf.mem_model == MM_SCORP || conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_GMX) && !(comp.flags & CF_DOSPORTS))
       pFE = ((port & 0x23) == (0xFE & 0x23));
   else if (conf.mem_model == MM_QUORUM) // 1xx11xx0
       pFE = ((port & 0x99) == (0xFE & 0x99));
   else // others xxxxxxx0
       pFE = !(port & 1);

   if (pFE)
   {
//[vv]      assert(!(val & 0x08));

#ifdef LOG_FE_OUT
   fprintf(f_log_FE_out, "%d\t%02X\r\n", (u32)(comp.t_states + cpu.t), val);
#endif

      spkr_dig = (val & 0x10) ? conf.sound.beeper_vol : 0;
      mic_dig = (val & 0x08) ? conf.sound.micout_vol : 0;

      // speaker & mic
      if ((comp.pFE ^ val) & 0x18)
      {
//          __debugbreak();
          flush_dig_snd();
      }

      u8 new_border = (val & 7);

    if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_ATM450)
    // BRIGHT for ATM border
          new_border += ((port & 8) ^ 8);

    new_border |= 0xF0;
    if ( comp.ts.border != new_border )
      update_screen();
    comp.ts.border = new_border;

      if (conf.mem_model == MM_ATM450)
    {
          set_atm_aFE((u8)port);
      init_raster();
    }

      if (conf.mem_model == MM_PROFI)
      {
        if (!(port & 0x80) && (comp.pDFFD & 0x80))
        {
      int addr = (comp.pFE ^ 0xF) | 0xF0;
      int val = ~p2;
      comp.cram[addr] =
        ((val & 0x03) << 3) |  // b
        ((val & 0x1C) << 10) |  // r
        ((val & 0xE0) << 2);  // g
      update_clut(addr);
        }
      }

      comp.pFE = val;  // looks obsolete
      // do not return! intro to navy seals (by rst7) uses out #FC for to both FE and 7FFD
   }

   // #xD
   if (!(port & 2))
   {
      // port DD - covox
      if (conf.sound.covoxDD && (u8)port == 0xDD)
      {
//         __debugbreak();
         flush_dig_snd();
         covDD_vol = val*conf.sound.covoxDD_vol/0x100;
         return;
      }

      // zx128 port and related - !!! rewrite it using switch
      if (!(port & 0x8000))
      {
         // 0001xxxxxxxxxx0x (bcig4)
         if ((port & 0xF002) == (0x1FFD & 0xF002) && conf.mem_model == MM_PLUS3)
             goto set1FFD;

         if ((port & 0xC003) == (0x1FFD & 0xC003) && ( (conf.mem_model == MM_KAY) || (conf.mem_model == MM_PHOENIX) ))
             goto set1FFD;

         // 00xxxxxxxx1xxx01 (sc16 green)
         if ( (port & 0xC023) == (0x1FFD & 0xC023) &&
       ((conf.mem_model == MM_SCORP) || (conf.mem_model == MM_PROFSCORP)) )
         {
set1FFD:
            comp.p1FFD = val;
            set_banks();
            return;
         }

         if (conf.mem_model == MM_ATM450 && (port & 0x8202) == (0x7DFD & 0x8202))
         {
             atm_writepal(val);
             return;
         }

         // if (conf.mem_model == MM_ATM710 && (port & 0x8202) != (0x7FFD & 0x8202)) return; // strict 7FFD decoding on ATM-2

         // 01xxxxxxxx1xxx01 (sc16 green)
         if ((port & 0xC023) != (0x7FFD & 0xC023) &&
       (conf.mem_model == MM_SCORP || conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_GMX))
             return;

         // 0xxxxxxxxxx11x0x
         if ((port & 0x801A) != (0x7FFD & 0x801A) && (conf.mem_model == MM_QUORUM))
             return;

         // 7FFD blocker, if 48 lock bit is set
         if (comp.p7FFD & 0x20)
         {
            // #EFF7.2 = 0 disables lock
            if ((((conf.mem_model == MM_PENTAGON && conf.ramsize == 1024) || (conf.mem_model == MM_ATM3))) && !(comp.pEFF7 & EFF7_LOCKMEM))
              goto set_7ffd;
            // Profi with #DFFD.4 = 1 disables lock
            else if (conf.mem_model == MM_PROFI && (comp.pDFFD & 0x10))
              goto set_7ffd;
            // GMX with #7EFD.2 = 1 disables lock
            else if (conf.mem_model == MM_GMX && (comp.p7EFD & 0x04))
              goto set_7ffd;
            // else apply lock
            else
              return;
         }

         // In TS Memory Model the actual value of #7FFD ignored, and page3 is used instead
         if (conf.mem_model == MM_TSL)
         {
            u8 lock128auto = !(!(cpu.opcode & 0x80) ^ !(cpu.opcode & 0x40));    // out(c), R = no lock or out(#FD), a = lock128
            u8 page128  = val & 0x07;
            u8 page512  = ((val & 0xC0) >> 3) | (val & 0x07);
            u8 page1024 = (val & 0x20) | ((val & 0xC0) >> 3) | (val & 0x07);

            switch (comp.ts.lck128)
            {
                // 512kB
                case 0:
                    comp.ts.page[3] = page512;
                break;

                // 128kB
                case 1:
                    comp.ts.page[3] = page128;
                break;

                // 512/128kB auto
                case 2:
                    comp.ts.page[3] = lock128auto ? page128 : page512;
                break;

                // 1024kB
                case 3:
                    comp.ts.page[3] = page1024;
                    val &= ~0x20;
                break;
            }
         }

       set_7ffd:
         comp.p7FFD = val;      // all models apart from TSL will deal with this variable
         comp.ts.vpage = comp.ts.vpage_d = (val & 8) ? 7 : 5;

         set_banks();
         return;
      }

      // xx0xxxxxxxxxxx0x (3.2) [vv]
      if ((port & 0x2002) == (0xDFFD & 0x2002) && conf.mem_model == MM_PROFI)
      {
          comp.pDFFD = val;
          set_banks();
      init_raster();
          return;
      }

      if (conf.mem_model == MM_ATM450 && (port & 0x8202) == (0xFDFD & 0x8202))
      {
          comp.pFDFD = val;
          set_banks();
          return;
      }

      // 1x0xxxxxxxx11x0x
      if ((port & 0xA01A) == (0x80FD & 0xA01A) && conf.mem_model == MM_QUORUM)
      {
          comp.p80FD = val;
          set_banks();
          return;
      }

      // FFFD - PSG address
      if ((port & 0xC0FF) == 0xC0FD)
      {
         // printf("Reg: %d\n", val);

         // switch active PSG via NedoPC scheme
         switch (conf.sound.ay_scheme)
         {
           case AY_SCHEME_CHRV:
             if ((val & 0xF8) == 0xF8)
             {
               if (conf.sound.ay_chip == (SNDCHIP::CHIP_YM2203))
               {
                 fmsoundon0 = val & 4;
                 tfmstatuson0 = val & 2;
               }
               comp.active_ay = val & 1;
             };
           break;

           case AY_SCHEME_AYX32:
             if ((val & 0xF8) == 0xF8)
               comp.active_ay = ~val & 1;
           break;

           case AY_SCHEME_QUADRO:
             comp.active_ay = (port >> 12) & 1;
           break;
         }

         ayx32.reg = val;
         if ((conf.sound.ay_scheme == AY_SCHEME_AYX32) && (val >= 16))
           ayx32.write_addr(val);
         else
           ay[comp.active_ay].select(val);
         return;
      }

      // BFFD - PSG data
      if (((port & 0xC000) == 0x8000) && conf.sound.ay_scheme)
      {
         // printf("Write: %d\n", val);

         u8 n_ay;
         switch (conf.sound.ay_scheme)
         {
           case AY_SCHEME_QUADRO:
             n_ay = (port >> 12) & 1;
           break;

           default:
             n_ay = comp.active_ay;
         }

         if ((conf.sound.ay_scheme == AY_SCHEME_AYX32) && (ayx32.reg >= 16))
           ayx32.write(temp.sndblock ? 0 : cpu.t, val);
         else
           ay[n_ay].write(temp.sndblock ? 0 : cpu.t, val);

         if (conf.input.mouse == 2 && ay[n_ay].get_activereg() == 14)
           input.aymouse_wr(val);
         return;
      }
      return;
   }

   if (conf.sound.sd && (port & 0xAF) == 0x0F)
   { // soundrive
//      __debugbreak();
      if ((u8)port == 0x0F) comp.p0F = val;
      if ((u8)port == 0x1F) comp.p1F = val;
      if ((u8)port == 0x4F) comp.p4F = val;
      if ((u8)port == 0x5F) comp.p5F = val;
      flush_dig_snd();
      sd_l = (conf.sound.sd_vol * (comp.p0F+comp.p1F)) >> 8;
      sd_r = (conf.sound.sd_vol * (comp.p4F+comp.p5F)) >> 8;
      return;
   }
   if (conf.sound.covoxFB && !(port & 4))
   { // port FB - covox
//      __debugbreak();
      flush_dig_snd();
      covFB_vol = val*conf.sound.covoxFB_vol/0x100;
      return;
   }

   if (conf.sound.saa1099 && (p1 == 0xFF)) // saa1099
   {
       if (port & 0x100)
           Saa1099.WrCtl(val);
       else
           Saa1099.WrData(temp.sndblock? 0 : cpu.t, val);
       return;
   }

   if ( (port == 0xEFF7) &&
      ( (conf.mem_model == MM_PENTAGON) || (conf.mem_model == MM_ATM3) ||
      (conf.mem_model == MM_TSL) || (conf.mem_model == MM_PHOENIX) ) ) // lvd added eff7 to atm3
   {
    u8 oldpEFF7 = comp.pEFF7;
      comp.pEFF7 = (comp.pEFF7 & conf.EFF7_mask) | (val & ~conf.EFF7_mask);
      // comp.pEFF7 |= EFF7_GIGASCREEN; // [vv] disable turbo

    if (conf.mem_model==MM_PENTAGON)
      turbo((comp.pEFF7 & EFF7_GIGASCREEN) ? 1 : 2);
    if (conf.mem_model==MM_ATM3)
      set_turbo();

    init_raster();

    /*

      if (!(comp.pEFF7 & EFF7_4BPP))
      {
          temp.offset_vscroll = 0;
          temp.offset_vscroll_prev = 0;
          temp.offset_hscroll = 0;
          temp.offset_hscroll_prev = 0;
      }
*/
      if ((comp.pEFF7 ^ oldpEFF7) & (EFF7_ROCACHE | EFF7_LOCKMEM))
          set_banks();
      return;
   }
   if (conf.cmos && (((comp.pEFF7 & EFF7_CMOS) && conf.mem_model == MM_PENTAGON) ||            // check bit 7 port EFF7 for Pentagon
     (conf.mem_model == MM_TSL && ((comp.pEFF7 & EFF7_CMOS) || (comp.flags & CF_DOSPORTS))) || // check bit 7 port EFF7 or active DOS for TSConf
     (conf.mem_model == MM_ATM3) || (conf.mem_model == MM_PHOENIX) ))
   {
      unsigned mask = ((conf.mem_model == MM_ATM3) && (comp.flags & CF_DOSPORTS)) ? ~0x100 : 0xFFFF;
      mask = (conf.mem_model == MM_TSL) ? 0xFFFF : mask;

      if (port == (0xDFF7 & mask))
      {
          comp.cmos_addr = val;
          return;
      }
      if (port == (0xBFF7 & mask))
      {
         if ((conf.mem_model == MM_ATM3 || conf.mem_model == MM_TSL) && comp.cmos_addr >= 0xF0 && val <= 2)
         {
            if (val < 2)
            {
               input.buffer.Enable(false);

               static unsigned version = 0;
               if (!version)
               {
                  unsigned day, year;
                  char month[8];
                  static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
                  sscanf(__DATE__, "%s %d %d", month, &day, &year);
                  version = day | ((strstr(months, month) - months) / 3 + 1) << 5 | (year - 2000) << 9;
               }

               strcpy((char*)cmos + 0xF0, "UnrealSpeccy");
               *(unsigned*)(cmos + 0xFC) = version;
            }
            else input.buffer.Enable(true);
         }
         else cmos_write(val);
         return;
      }
   }
   if ( (p1 == 0xEF) && (zf232.open_port) )
      zf232.write(p2, val);
}

__inline u8 in1(unsigned port)
{
   port &= 0xFFFF;
   brk_port_in = port;

   u8 p1 = (port & 0xFF);
   u8 p2 = ((port >>8) & 0xFF);

   u8 tmp;

   if (conf.ulaplus)
   {
     if (port == 0xFF3B)
     {
         // ULA+ DATA
      u8 c = (!(comp.ulaplus_reg & 0xC0) && (comp.ulaplus_mode & 1)) ? comp.ulaplus_cram[comp.ulaplus_reg] : 0xFF;
      return c;
     }
   }

/*
   if (p1 == 0xF0)
       __debugbreak();

   if ((comp.flags & CF_DOSPORTS) && port == 0xFADF)
       __debugbreak();
*/

   // В начале дешифрация портов по полным 8бит
   // ngs
   #ifdef MOD_GS
   if ((port & 0xF7) == 0xB3 && conf.gs_type)
       return in_gs(port);
   #endif

   // ZXM-MoonSound
   if ( conf.sound.moonsound &&
      !(comp.flags & CF_DOSPORTS) &&
    (conf.mem_model == MM_PROFI ? !(comp.pDFFD & 0x80) : 1) &&
    (((p1 & 0xFC) == 0xC4) ||
       ((p1 & 0xFE) == 0x7E) ) )
   {
     u8 val = 0xFF;

     if ( zxmmoonsound.read( port, val ) )
       return val;
   }

   // z-controller
   if (conf.zc && (p1 == 0x57) || (p1 == 0x77))
   {
      // no shadow-mode ZXEVO patch here since 0x57 port in read mode is the same
      // as in noshadow-mode, i.e. no A15 is used to decode port.
      return Zc.Rd(port);
   }

   if ((conf.mem_model == MM_TSL) && (p1 == 0xAF))
   {
       // TS-Config extensions ports
     switch (p2) {

      case TSR_STATUS:
      {
        tmp = comp.ts.pwr_up | (comp.ts.vdac2 ? TS_VDAC2 : comp.ts.vdac);
        comp.ts.pwr_up = TS_PWRUP_OFF;
        return tmp;
      }

      case TSR_PAGE2:
        return comp.ts.page[2];

      case TSR_PAGE3:
        return comp.ts.page[3];

      case TSR_DMASTATUS:
        return (comp.ts.dma.state == DMA_ST_NOP) ? 0x00 : 0x80;
     }
  }

  if (conf.mem_model == MM_ATM3)
   {
       // Порт расширений АТМ3
       if (p1 == 0xBF)
           return comp.pBF;

       if (p1 == 0xBE)
       {
           u8 port_hi = (port >> 8) & 0xFF;
           if ((port_hi & ~7) == 0) // Чтение не инвертированного номера страницы
           {
               unsigned PgIdx = port_hi & 7;
               return (comp.pFFF7[PgIdx] & 0xFF) ^ 0xFF;
           }

           switch(port_hi)
           {
           case 0x8: // ram/rom
           {
               u8 RamRomMask = 0;
               for (unsigned i = 0; i < 8; i++)
                   RamRomMask |= ((comp.pFFF7[i] >> 8) & 1) << i;
               return ~RamRomMask;
           }
           case 0x9: //dos7ffd
           {
               u8 RamRomMask = 0;
               for (unsigned i = 0; i < 8; i++)
                   RamRomMask |= ((comp.pFFF7[i] >> 9) & 1) << i;
               return ~RamRomMask;
           }
           case 0xA: return comp.p7FFD;
           case 0xB: return comp.pEFF7; // lvd - added EFF7 reading in pentevo (atm3)

           // lvd: fixed bug with no-anding bits from aFF77, added CF_TRDOS to bit 4
           case 0xC: return (((comp.aFF77 >> 14) << 7) & 0x0080) | (((comp.aFF77 >> 9) << 6) & 0x0040) | (((comp.aFF77 >> 8) << 5) & 0x0020) | ((comp.flags & CF_TRDOS)?0x0010:0) | (comp.pFF77 & 0xF);
           case 0xD: return atm_readpal();
           case 0xE: return zxevo_readfont();
           }
       }
   }

   // divide на nemo портах
   if (conf.ide_scheme == IDE_NEMO_DIVIDE)
   {
       if (((port & 0x1E) == 0x10)) // rrr1000x
       {
           if (p1 == 0x11)
           {
               comp.ide_hi_byte_r = 0;
               return comp.ide_read;
           }

           if ((port & 0xFE) == 0x10)
           {
               comp.ide_hi_byte_r ^= 1;
               if (!comp.ide_hi_byte_r)
               {
                   return comp.ide_read;
               }
           }
           else
           {
               comp.ide_hi_byte_r = 0;
           }
           goto read_hdd_5;
       }
       else if (p1 == 0xC8)
       {
        return hdd.read(8);
       }
   }

   // quorum additional keyboard port
   if ((conf.mem_model == MM_QUORUM) && (p1 == 0x7E))
   {
      u8 val = input.read_quorum(port >> 8);
      return val;
   }

   if (comp.flags & CF_DOSPORTS)
   {
      if (conf.mem_model == MM_ATM3 && (p1 & 0x1F) == 0x0F && !(((p1 >> 5) - 1) & 4))
      {
          // 2F = 001|01111b
          // 4F = 010|01111b
          // 6F = 011|01111b
          // 8F = 100|01111b
          return comp.wd_shadow[(p1 >> 5) - 1];
      }


      if (conf.ide_scheme == IDE_ATM && (port & 0x1F) == 0x0F)
      {
         if (port & 0x100)
             return comp.ide_read;
      read_hdd_5:
         port >>= 5;
      read_hdd:
         port &= 7;
         if (port)
             return hdd.read(port);
         unsigned v = hdd.read_data();
         comp.ide_read = (u8)(v >> 8);
         return (u8)v;
      }

      if ((port & 0x18A3) == (0xFFFE & 0x18A3))
      { // SMUC
         if (conf.smuc)
         {
            if ((port & 0xA044) == (0xDFBA & 0xA044)) return cmos_read(); // clock
            if ((port & 0xA044) == (0xFFBA & 0xA044)) return comp.nvram.out; // SMUC system port
            if ((port & 0xA044) == (0x7FBA & 0xA044)) return comp.p7FBA | 0x3F;
            if ((port & 0xA044) == (0x5FBA & 0xA044)) return 0x3F;
            if ((port & 0xA044) == (0x5FBE & 0xA044)) return 0x57;
            if ((port & 0xA044) == (0x7FBE & 0xA044)) return 0x57;
         }
         if ((port & 0x8044) == (0xFFBE & 0x8044) && conf.ide_scheme == IDE_SMUC)
         { // FFBE, FEBE
            if (comp.pFFBA & 0x80)
            {
                if (!(port & 0x100))
                    return hdd.read(8); // alternate status
                return 0xFF; // obsolete register
            }

            if (!(port & 0x2000))
                return comp.ide_read;
            port >>= 8;
            goto read_hdd;
         }
      }

      u8 p1 = (u8)port;

      if (conf.mem_model == MM_PROFI) // molodcov_alex
      {
          if ((comp.p7FFD & 0x10) && (comp.pDFFD & 0x20))
          { // modified ports
            // BDI ports
            if ((p1 & 0x9F) == 0x83)
                return comp.wd.in((p1 & 0x60) | 0x1F);  // WD93 ports (1F, 3F, 7F)
            if ((p1 & 0xE3) == 0x23)
                return comp.wd.in(0xFF);                // port FF

            // RTC
            if ((port & 0x9F) == 0x9F && conf.cmos)
            {
                if (!(port & 0x20))
                    return cmos_read();
            }

            // IDE
            if ((p1 & 0x9F) == 0x8B && (conf.ide_scheme == IDE_PROFI))
            {
                if (p1 & 0x40) // cs1
                {
                    if (p1 & 0x20)
                        return comp.ide_read;
                    port >>= 8;
                    goto read_hdd;
                }
            }
          }
          else
          {
              // BDI ports
              if ((p1 & 0x83) == 0x03)
                  return comp.wd.in((p1 & 0x60) | 0x1F);  // WD93 ports
              if ((p1 & 0xE3) == ((comp.pDFFD & 0x20) ? 0xA3 : 0xE3))
                  return comp.wd.in(0xFF);                // port FF
          }
      }

      if (conf.mem_model == MM_QUORUM /* && !(comp.p00 & Q_TR_DOS) */) // cpm ports
      {
          if ((p1 & 0xFC) == 0x80) // 80, 81, 82, 83
          {
              p1 = ((p1 & 3) << 5) | 0x1F;
              return comp.wd.in(p1);
          }
      }
          // 1F = 0001|1111b
          // 3F = 0011|1111b
          // 5F = 0101|1111b
          // 7F = 0111|1111b
          // DF = 1101|1111b порт мыши
          // FF = 1111|1111b
      else if ((p1 & 0x9F) == 0x1F || p1 == 0xFF) // 1F, 3F, 5F, 7F, FF
      {
        if (conf.mem_model == MM_TSL)
        {
          if (comp.ts.vdos)
          { // vdos_on
            comp.ts.vdos = 0;
            set_banks();
            return 0xFF;
          }
          else if ((1<<comp.wd.drive) & comp.ts.fddvirt)
          { // vdos_off
            comp.ts.vdos_m1 = 1;
            return 0xFF;
          }
        }
        return comp.wd.in(p1); // physical drive
      }
   }
   else // не dos
   {
       // VG93 free access in TS-Conf (FDDVirt.7 = 1)
       if ((conf.mem_model == MM_TSL) && (comp.ts.fddvirt & 0x80) && ((p1 & 0x1F) == 0x1F)) // 1F, 3F, 5F, 7F, FF
         return comp.wd.in(p1);

       if (((port & 0xA3) == 0xA3) && (conf.ide_scheme == IDE_DIVIDE))
       {
           if (p1 == 0xA3)
           {
               comp.ide_hi_byte_r ^= 1;
               if (!comp.ide_hi_byte_r)
               {
                   return comp.ide_read;
               }
           }
           else
           {
               comp.ide_hi_byte_r = 0;
           }
           port >>= 2;
           goto read_hdd;
       }


       if (!(port & 6) && (conf.ide_scheme == IDE_NEMO || conf.ide_scheme == IDE_NEMO_A8))
       {
          unsigned hi_byte = (conf.ide_scheme == IDE_NEMO)? (port & 1) : (port & 0x100);
          if (hi_byte)
              return comp.ide_read;
          comp.ide_read = 0xFF;
          if ((port & 0x18) == 0x08)
              return ((port & 0xE0) == 0xC0)? hdd.read(8) : 0xFF; // CS1=0,CS0=1,reg=6
          if ((port & 0x18) != 0x10)
              return 0xFF; // invalid CS0,CS1
          goto read_hdd_5;
       }
   }


   if (!(port & 0x20))
   { // kempstons
      port = (port & 0xFFFF) | 0xFA00; // A13,A15 not used in decoding
      if ((port == 0xFADF || port == 0xFBDF || port == 0xFFDF) && conf.input.mouse == 1)
      { // mouse
         input.mouse_joy_led |= 1;
         if (port == 0xFBDF)
             return input.kempston_mx();
         if (port == 0xFFDF)
             return input.kempston_my();
         return input.mbuttons;
      }
      input.mouse_joy_led |= 2;
      u8 res = (conf.input.kjoy)? input.kjoy : 0xFF;
      if (conf.mem_model == MM_SCORP || conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_GMX)
         res = (res & 0x1F) | (comp.wd.in(0xFF) & 0xE0);
      return res;
   }

   if ((conf.mem_model == MM_GMX) && (port & 0x23) == (0xFD & 0x23))
   {
    unsigned char tmp;
    switch ( p2 )
    {
      case 0x7A:
        return (comp.p7FFD & 7)
          | ((comp.p1FFD & 0x10) >> 1)
          | ((comp.pDFFD & 7) << 4)
          | ((comp.pFE & 1) << 7);
      case 0x7E:
        return ((comp.p7FFD & 0x20) >> 5)
          | ((comp.p7FFD & 8) >> 2)
          | ((comp.p7EFD & 0x80) >> 5)
          | (comp.p7EFD & 8)
          | ((comp.p00 & 0x80) >> 3)
          | (comp.p00 & 0x20)
          | ((comp.p1FFD & 1) << 6)
          | ((comp.pFE & 4) << 5);
      case 0x78:
        tmp = (comp.p78FD & 0x7F) | ((comp.pFE & 2) << 6);
        tmp |= (comp.gmx_magic_shift&1);
        comp.gmx_magic_shift >>= 1;
        return tmp;
    }
  }

   if ((port & 0x8023) == (0xFD & 0x8023) && (conf.mem_model == MM_SCORP || conf.mem_model == MM_PROFSCORP))
    turbo(port & 0x4000 ? 2 : 1);

   // port #FE
   bool pFE;

   // scorp  xx1xxx10 (sc16)
   if ((conf.mem_model == MM_SCORP || conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_GMX) && !(comp.flags & CF_DOSPORTS))
       pFE = ((port & 0x23) == (0xFE & 0x23));
   else if (conf.mem_model == MM_QUORUM) // 1xx11xx0
       pFE = ((port & 0x99) == (0xFE & 0x99));
   else // others xxxxxxx0
       pFE = !(port & 1);

   if (pFE)
   {
      if ((cpu.pc & 0xFFFF) == 0x0564 && bankr[0][0x0564]==0x1F && conf.tape_autostart && !comp.tape.play_pointer)
          start_tape();
      u8 val = input.read(port >> 8);
      if (conf.mem_model == MM_ATM450)
          val = (val & 0x7F) | atm450_z(cpu.t);
#ifdef LOG_FE_IN
   fprintf(f_log_FE_in, "%d\t%02X\t%02X\r\n", (u32)(comp.t_states + cpu.t), val, cpu.a);
#endif
      return val;
   }

   // ATM-2 IDE+DAC/ADC
   if ((port & 0x8202) == (0x7FFD & 0x8202) && (conf.mem_model == MM_ATM710 || conf.ide_scheme == IDE_ATM))
   {
      u8 irq = 0x40;
      if (conf.ide_scheme == IDE_ATM) irq = (hdd.read_intrq() & 0x40);
      return irq + 0x3F;
   }

   // xxFD - AY
   if ((p1 == 0xFD) && conf.sound.ay_scheme)
   {
      if ((conf.sound.ay_scheme == AY_SCHEME_CHRV) && (conf.sound.ay_chip == (SNDCHIP::CHIP_YM2203)) && (tfmstatuson0 == 0))
        return 0x7F;  // always ready

      if ((p2 & 0xC0) != 0xC0)
        return 0xFF;

      u8 n_ay;
      if (conf.sound.ay_scheme == AY_SCHEME_QUADRO)
        n_ay = (port >> 12) & 1;
      else
        n_ay = comp.active_ay;

      // read selected AY register
      if (conf.input.mouse == 2 && (ay[n_ay].get_activereg() == 14))
      {
        input.mouse_joy_led |= 1;
        return input.aymouse_rd();
      }

      u8 rc;
      if ((conf.sound.ay_scheme == AY_SCHEME_AYX32) && (ayx32.reg >= 16))
        rc = ayx32.read();
      else
        rc = ay[n_ay].read();

      // printf("Read: %d\n", rc);
      return rc;
   }

//   if ((port & 0x7F) == 0x7B) { // FB/7B
   if ((port & 0x04) == 0x00)
   { // FB/7B (for MODPLAYi)
      if (conf.mem_model == MM_ATM450)
      {
         comp.aFB = (u8)port;
         set_banks();
      }
      else if (conf.cache)
      {
         comp.flags &= ~CF_CACHEON;
         if (port & 0x80) comp.flags |= CF_CACHEON;
         set_banks();
      }
      return 0xFF;
   }

   if (conf.cmos && ((comp.pEFF7 & EFF7_CMOS) || // check 7 bit port EFF7
     conf.mem_model == MM_ATM3 || // ATM3
     (((comp.pEFF7 & EFF7_CMOS) || (comp.flags & CF_DOSPORTS)) && conf.mem_model == MM_TSL))) // check bit 7 port EFF7 or active DOS for TSConf
   {
      unsigned mask = ((conf.mem_model == MM_ATM3) && (comp.flags & CF_DOSPORTS)) ? ~0x100 : 0xFFFF;
    mask = (conf.mem_model == MM_TSL) ? 0xFFFF : mask;

      if (port == (0xBFF7 & mask))
          return cmos_read();
   }

   if ( (p1 == 0xEF) && (zf232.open_port) )
      return zf232.read(p2);

   if (conf.portff && (p1 == 0xFF))
   {
      if (vmode != 2) return 0xFF; // ray is not in paper
      unsigned ula_t = (cpu.t+temp.border_add) & temp.border_and;
      return temp.base[vcurr->atr_offs + (ula_t - vcurr[-1].next_t)/4];
   }
   return 0xFF;
}

u8 in(unsigned port)
{
   brk_port_val = in1(port);
   return brk_port_val;
}

// TS-Config extensions ports
void ts_ext_port_wr(u8 port, u8 val)
{
  switch (port)
  {
      // system
    case TSW_SYSCONF:
      comp.ts.sysconf = val;
      comp.ts.cacheconf = comp.ts.cache ? 0x0F : 0x00;
      set_clk();
    break;

    case TSW_CACHECONF:
      comp.ts.cacheconf = val & 0x0F;
    break;

    case TSW_FDDVIRT:
      comp.ts.fddvirt = val & 0x8F;
    break;

    case TSW_INTMASK:
      comp.ts.intmask = val & 0x07;
      comp.ts.intctrl.pend &= val & 0x07;
    break;

    case TSW_HSINT:
      comp.ts.hsint = val;
      comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line-1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
    break;

    case TSW_VSINTL:
      comp.ts.vsintl = val;
      comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line-1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
    break;

    case TSW_VSINTH:
      comp.ts.vsinth = val;
      comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line-1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
    break;

    // memory
    case TSW_MEMCONF:
      comp.ts.memconf = val;
      comp.p7FFD &= ~0x10;
      comp.p7FFD |= (val & 1) << 4;
      set_banks();
    break;

    case TSW_PAGE0:
      comp.ts.page[0] = val;
      set_banks();
    break;

    case TSW_PAGE1:
      comp.ts.page[1] = val;
      set_banks();
    break;

    case TSW_PAGE2:
      comp.ts.page[2] = val;
      set_banks();
    break;

    case TSW_PAGE3:
      comp.ts.page[3] = val;
      set_banks();
    break;

    case TSW_FMADDR:
      comp.ts.fmaddr = val;
    break;

    // video
    case TSW_VCONF:
      comp.ts.vconf_d = val;
    break;

    case TSW_VPAGE:
      comp.ts.vpage_d = val;
      set_banks();
    break;

    case TSW_TMPAGE:
      comp.ts.tmpage = val;
    break;

    case TSW_T0GPAGE:
      comp.ts.t0gpage[0] = val;
    break;

    case TSW_T1GPAGE:
      comp.ts.t1gpage[0] = val;
    break;

    case TSW_SGPAGE:
      comp.ts.sgpage = val;
    break;

    case TSW_BORDER:
      comp.ts.border = val;
    break;

    case TSW_TSCONF:
      comp.ts.tsconf = val;
    break;

    case TSW_PALSEL:
      comp.ts.palsel_d = val;
    break;

    case TSW_GXOFFSL:
      comp.ts.g_xoffsl_d = val;
    break;

    case TSW_GXOFFSH:
      comp.ts.g_xoffsh_d = val & 1;
    break;

    case TSW_GYOFFSL:
      comp.ts.g_yoffsl = val;
      comp.ts.g_yoffs_updated = 1;
    break;

    case TSW_GYOFFSH:
      comp.ts.g_yoffsh = val & 1;
      comp.ts.g_yoffs_updated = 1;
    break;

    case TSW_T0XOFFSL:
      comp.ts.t0_xoffsl = val;
    break;

    case TSW_T0XOFFSH:
      comp.ts.t0_xoffsh = val & 1;
    break;

    case TSW_T0YOFFSL:
      comp.ts.t0_yoffsl = val;
    break;

    case TSW_T0YOFFSH:
      comp.ts.t0_yoffsh = val & 1;
    break;

    case TSW_T1XOFFSL:
      comp.ts.t1_xoffsl = val;
    break;

    case TSW_T1XOFFSH:
      comp.ts.t1_xoffsh = val & 1;
    break;

    case TSW_T1YOFFSL:
      comp.ts.t1_yoffsl = val;
    break;

    case TSW_T1YOFFSH:
      comp.ts.t1_yoffsh = val & 1;
    break;

  // dma
    case TSW_DMASAL:
      if (assert_dma_active())
        comp.ts.saddrl = val &0xFE;
    break;

    case TSW_DMASAH:
      if (assert_dma_active())
        comp.ts.saddrh = val & 0x3F;
    break;

    case TSW_DMASAX:
      if (assert_dma_active())
        comp.ts.saddrx = val;
    break;

    case TSW_DMADAL:
      if (assert_dma_active())
        comp.ts.daddrl = val &0xFE;
    break;

    case TSW_DMADAH:
      if (assert_dma_active())
        comp.ts.daddrh = val & 0x3F;
    break;

    case TSW_DMADAX:
      if (assert_dma_active())
        comp.ts.daddrx = val;
    break;

    case TSW_DMALEN:
      if (assert_dma_active())
        comp.ts.dmalen = val;
    break;

    case TSW_DMANUM:
      if (assert_dma_active())
        comp.ts.dmanum = val;
    break;

    case TSW_DMACTR:
      if (assert_dma_active())
      {
        comp.ts.dma.ctrl = val;
        comp.ts.dma.state = DMA_ST_INIT;
      }
    break;

    default:
      color(CONSCLR_WARNING); printf("Illegal port! OUT (%02XAF), %02X, PC = %02X:%04X\r\n", port, val, comp.ts.page[cpu.pc >> 14], cpu.pc);
    break;
  }
}

#undef in_trdos
#undef out_trdos
