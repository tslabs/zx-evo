#pragma once
#include "sysdefs.h"
#include "defs.h"

Z80INLINE void handle_int(Z80 *cpu, u8 vector)
{
   unsigned intad;
   if (cpu->im < 2) {
      intad = 0x38;
   } else { // im2
      unsigned vec = vector + cpu->i*0x100;
      intad = cpu->MemIf->rm(vec) + 0x100*cpu->MemIf->rm(vec+1);
   }

   if (cpu->DirectRm(cpu->pc) == 0x76) // int on halt command
       cpu->pc++;

   cputact(((cpu->im < 2) ? 13 : 19) - 3);
   cpu->MemIf->wm(--cpu->sp, cpu->pch);
   cpu->MemIf->wm(--cpu->sp, cpu->pcl);
   cpu->pc = intad;
   cpu->memptr = intad;
   cpu->halted = 0;
   cpu->iff1 = cpu->iff2 = 0;
   cpu->int_pend = false;
   if (conf.mem_model == MM_TSL)
   {
     if (comp.ts.intctrl.frame_pend) comp.ts.intctrl.frame_pend = 0;
     else
     if (comp.ts.intctrl.line_pend)  comp.ts.intctrl.line_pend  = 0;
     else
     if (comp.ts.intctrl.dma_pend)   comp.ts.intctrl.dma_pend   = 0;
   }
}
