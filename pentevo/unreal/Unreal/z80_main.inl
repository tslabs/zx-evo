
// Адрес может превышать 0xFFFF
// (чтобы в каждой команде работы с регистрами не делать &= 0xFFFF)
u8 rm(unsigned addr)
{
   addr &= 0xFFFF;

#ifdef Z80_DBG
   u8 *membit = membits + (addr & 0xFFFF);
   *membit |= MEMBITS_R;
   dbgbreak |= (*membit & MEMBITS_BPR);
   cpu.dbgbreak |= (*membit & MEMBITS_BPR);
#endif

#ifdef MOD_GSZ80
   if ((temp.gsdmaon!=0) && ( (conf.mem_model==MM_PENTAGON) || (conf.mem_model==MM_ATM3) ) && ((addr & 0xc000)==0) && ((comp.pEFF7 & EFF7_ROCACHE)==0))
    {
     u8 *tmp;
     tmp = GSRAM_M+((temp.gsdmaaddr-1) & 0x1FFFFF);
     temp.gsdmaaddr = (temp.gsdmaaddr + 1) & 0x1FFFFF;
     z80gs::flush_gs_z80();
     brk_mem_val = *tmp;
	 goto ret;
    }
#endif

  u8 window = (addr >> 14) & 3;

  if (bankm[window] == BANKM_RAM)    // RAM hit
  {
        // TS-conf cache model
    if (conf.mem_model == MM_TSL)
    {
            // pentevo version for 16 bit DRAM/cache
      u32 cached_address = (comp.ts.page[window] << 5) | ((addr >> 9) & 0x1F);  // {page[7:0], addr[13:9]}
      u16 cache_pointer = addr & 0x1FF;  // addr[8:0]
      comp.ts.cache_miss = !(comp.ts.cacheconf & (1 << window)) || (cpu.tscache_addr[cache_pointer] != cached_address);

            if (comp.ts.cache_miss)
      {
                cpu.tscache_data[cache_pointer & ~1] = *am_r(addr & ~1);
        cpu.tscache_data[cache_pointer | 1] = *am_r(addr | 1);
        cpu.tscache_addr[cache_pointer & ~1] = cpu.tscache_addr[cache_pointer | 1] = cached_address;     // set cache tags for two subsequent 8-bit addresses
        vid.memcpucyc[cpu.t / 224]++;
        vid.memcyc_lcmd++;
      }

            brk_mem_val = cpu.tscache_data[cache_pointer];
			goto ret;
    }

    else
      vid.memcpucyc[cpu.t / 224]++;
  }

  else
    comp.ts.cache_miss = false;

   brk_mem_val = *am_r(addr);

ret:
#ifdef Z80_DBG
	brk_mem_rd = addr;
	/* this is sloooow */
	debug_cond_check(&cpu);
#endif

   return brk_mem_val;
}

// Адрес может превышать 0xFFFF
// (чтобы в каждой команде работы с регистрами не делать &= 0xFFFF)
void wm(unsigned addr, u8 val)
{
   addr &= 0xFFFF;

#ifdef Z80_DBG
   u8 *membit = membits + (addr & 0xFFFF);
   *membit |= MEMBITS_W;
   dbgbreak |= (*membit & MEMBITS_BPW);
   cpu.dbgbreak |= (*membit & MEMBITS_BPW);

	brk_mem_wr = addr;
    brk_mem_val = val;
	/* this is sloooow */
	debug_cond_check(&cpu);
#endif

#ifdef MOD_GSZ80
   if ((temp.gsdmaon!=0) && ( (conf.mem_model==MM_PENTAGON) || (conf.mem_model==MM_ATM3) ) && ((addr & 0xc000)==0))
    {
     u8 *tmp;
     tmp = GSRAM_M+temp.gsdmaaddr;
     *tmp = val;
     temp.gsdmaaddr++;
     z80gs::flush_gs_z80();
    }
#endif

#ifdef MOD_VID_VD
   if (comp.vdbase && (unsigned)((addr & 0xFFFF) - 0x4000) < 0x1800)
   {
       comp.vdbase[addr & 0x1FFF] = val;
       return;
   }
#endif

  // write to FPGA mapped area
  if ((conf.mem_model == MM_TSL) && (comp.ts.fm_en) && (((addr >> 12) & 0x0F) == comp.ts.fm_addr))
  {
    // 256 byte arrays
    if (((addr >> 8) & 0x0F) == TSF_REGS)
      ts_ext_port_wr(addr & 0xFF, val);
    
    // 512 byte arrays
    else
    {
      if (addr & 1)
      {
        switch ((addr >> 9) & 0x07)
        {
          case TSF_CRAM:
          {
            comp.cram[(addr >> 1) & 0xFF] = ((val << 8) | temp.fm_tmp);
            update_clut((addr >> 1) & 0xFF);
            break;
          }
          
          // 
          case TSF_SFILE:
          {
            comp.sfile[(addr >> 1) & 0xFF] = (val << 8) | temp.fm_tmp;
            break;
          }
        }
      }
      else
      // remember temp value
        temp.fm_tmp = val;
    }
  }

  // TS-conf cache model
  if (conf.mem_model == MM_TSL)
  {
    // pentevo version for 16 bit DRAM/cache
        u16 cache_pointer = addr & 0x1FE;
    cpu.tscache_addr[cache_pointer] = cpu.tscache_addr[cache_pointer + 1] = -1;    // invalidate two 8-bit addresses
    vid.memcpucyc[cpu.t / 224]++;
  }

   if ((conf.mem_model == MM_ATM3) && (comp.pBF & 4) /*&& ((addr & 0xF800) == 0)*/ ) // Разрешена загрузка шрифта для ATM3 // lvd: any addr is possible in ZXEVO
   {
       unsigned idx = ((addr&0x07F8) >> 3) | ((addr & 7) << 8);
       fontatm2[idx] = val;
       return;
   }

   u8 *a = bankw[(addr >> 14) & 3];
#ifndef TRASH_PAGE
   if (!a)
       return;
#endif
   a += (addr & (PAGE-1));

   *a = val;
}

Z80INLINE u8 m1_cycle(Z80 *cpu)
{
   if ((conf.mem_model == MM_PENTAGON) &&
       ((comp.pEFF7 & (EFF7_CMOS | EFF7_4BPP)) == (EFF7_CMOS | EFF7_4BPP)))
       temp.offset_vscroll++;
   if ((conf.mem_model == MM_PENTAGON) &&
      ((comp.pEFF7 & (EFF7_384 | EFF7_4BPP)) == (EFF7_384 | EFF7_4BPP)))
       temp.offset_hscroll++;
   if (conf.mem_model == MM_TSL && comp.ts.vdos_m1) {
      comp.ts.vdos    = 1;
      comp.ts.vdos_m1 = 0;
      set_banks();
   }
   return cpu->m1_cycle();
}

void Z80FAST step()
{
   if (comp.flags & CF_SETDOSROM)
   {
      if (cpu.pch == 0x3D)
      {
           comp.flags |= CF_TRDOS;  // !!! add here TS memconf behaviour !!!
           set_banks();
      }
   }
   else if (comp.flags & CF_LEAVEDOSADR)
   {
      if (cpu.pch & 0xC0) // PC > 3FFF closes TR-DOS
      {
         close_dos: comp.flags &= ~CF_TRDOS;
         set_banks();
      }
      if (conf.trdos_traps)
          comp.wd.trdos_traps();
   }
   else if (comp.flags & CF_LEAVEDOSRAM)
   {
      // executing RAM closes TR-DOS
      if (bankr[(cpu.pc >> 14) & 3] < RAM_BASE_M+PAGE*MAX_RAM_PAGES)
          goto close_dos;
      if (conf.trdos_traps)
          comp.wd.trdos_traps();
   }

   if (comp.tape.play_pointer && conf.tape_traps && (cpu.pc & 0xFFFF) == 0x056B)
       tape_traps();

   if (comp.tape.play_pointer && !conf.sound.enabled)
       fast_tape();

//~todo
//[vv]   unsigned oldt=cpu.t; //0.37
	if ( cpu.vm1 && cpu.halted )
	{
		cpu.tt += cpu.rate * 1;
		if ( ++ cpu.halt_cycle == 4 )
		{
			cpu.r_low += 1;
			cpu.halt_cycle = 0;
		}
	}
	else
	{
		if (cpu.pch & temp.evenM1_C0)
			cpu.tt += (cpu.tt & cpu.rate);

		u8 opcode = m1_cycle(&cpu);
		(normal_opcode[opcode])(&cpu);
	}

/* [vv]
//todo if (comp.turbo)cpu.t-=tbias[cpu.t-oldt]
   if ( ((conf.mem_model == MM_PENTAGON) && ((comp.pEFF7 & EFF7_GIGASCREEN)==0)) ||
       ((conf.mem_model == MM_ATM710) && (comp.pFF77 & 8)))
       cpu.t -= (cpu.t-oldt) >> 1; //0.37
//~todo
*/
#ifdef Z80_DBG
   if ((comp.flags & CF_PROFROM) && ((membits[0x100] | membits[0x104] | membits[0x108] | membits[0x10C]) & MEMBITS_R))
   {
      if (membits[0x100] & MEMBITS_R)
          set_scorp_profrom(0);
      if (membits[0x104] & MEMBITS_R)
          set_scorp_profrom(1);
      if (membits[0x108] & MEMBITS_R)
          set_scorp_profrom(2);
      if (membits[0x10C] & MEMBITS_R)
          set_scorp_profrom(3);
   }
#endif
}

void step1()
{
#ifdef Z80_DBG
  debug_events(&cpu);
#endif
  step();
}

void z80loop_TSL()
{
  cpu.haltpos = 0;
  comp.ts.intctrl.line_t = comp.ts.intline ? 0 : VID_TACTS;

  while (cpu.t < conf.frame)
  {
    bool vdos = comp.ts.vdos || comp.ts.vdos_m1;

    TSFrameINT(vdos);

    // Line INT
    if (cpu.t >= comp.ts.intctrl.line_t)
    {
      comp.ts.intctrl.line_t += VID_TACTS;
      if (!vdos) comp.ts.intctrl.line_pend = comp.ts.intline;
    }

    // DMA INT
    if (comp.ts.intctrl.new_dma)
    {
      comp.ts.intctrl.new_dma = false;
      comp.ts.intctrl.dma_pend = comp.ts.intdma;
    }

    vid.memcyc_lcmd = 0; // new command, start accumulate number of busy memcycles

    if (comp.ts.intctrl.pend && cpu.iff1 && cpu.t != cpu.eipos && !vdos) // int disabled in vdos after r/w vg ports
    {
      handle_int(&cpu, cpu.IntVec());
    }
    step1();
    update_screen(); // update screen, TSU, DMA
  }
}

void z80loop_other()
{
	bool int_occurred = false;
	unsigned int_start = conf.intstart;
	unsigned int_end = conf.intstart + conf.intlen;

  cpu.haltpos = 0;

	if ( int_end >= conf.frame )
	{
		int_end -= conf.frame;
		cpu.int_pend = true;
		int_occurred = true;
	}

  while (cpu.t < conf.frame)
  {
    // Baseconf NMI trap
    if (conf.mem_model == MM_ATM3 && (comp.pBF & 0x10) && (cpu.pc == comp.pBD))
      nmi_pending = 1;

    // NMI processing
    if (nmi_pending)
    {
      if (conf.mem_model == MM_ATM3)
      {
        nmi_pending = 0;
        cpu.nmi_in_progress = true;
        set_banks();
        m_nmi(RM_NOCHANGE);
        continue;
      }
      else if (conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_SCORP)
      {
        nmi_pending--;
        if (cpu.pc > 0x4000)
        {
          m_nmi(RM_DOS);
          nmi_pending = 0;
        }
      }
      else
        nmi_pending = 0;
    } // end if (nmi_pending)

    // Baseconf NMI
    if (comp.pBE)
    {
      if (conf.mem_model == MM_ATM3 && comp.pBE == 1)
      {
        cpu.nmi_in_progress = false;
        set_banks();
      }
      comp.pBE--;
    }

    // Reset INT
	if ( !int_occurred && cpu.t >= int_start) 
	{
		int_occurred = true;
		cpu.int_pend = true;
	}

	if ( cpu.int_pend && (cpu.t >= int_end) )
		cpu.int_pend = false;

    vid.memcyc_lcmd = 0; // new command, start accumulate number of busy memcycles

    // INT
    if (cpu.int_pend && cpu.iff1 && // INT enabled in CPU
      cpu.t != cpu.eipos &&         // INT disabled after EI
      cpu.int_gate)                 // INT enabled by ATM hardware (no INT disabling in PentEvo)
    {
      handle_int(&cpu, cpu.IntVec());
    }

    step1();
    update_screen(); // update screen, TSU, DMA
  } // end while (cpu.t < conf.intlen)
}


void z80loop()
{
  if (conf.mem_model == MM_TSL)
    z80loop_TSL();
  else
    z80loop_other();
}
