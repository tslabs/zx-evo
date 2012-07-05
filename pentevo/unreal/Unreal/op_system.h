#pragma once

Z80INLINE void handle_int(Z80 *cpu, unsigned char vector)
{
   unsigned intad;
   if (cpu->im < 2) {
      intad = 0x38;
   } else { // im2
      unsigned vec = vector + cpu->i*0x100;
      intad = cpu->MemIf->rm(vec) + 0x100*cpu->MemIf->rm(vec+1);
   }

   if (cpu->MemIf->rm(cpu->pc) == 0x76) // int on halt command
       cpu->pc++;

   cpu->t += (cpu->im < 2) ? 13 : 19;
   cpu->MemIf->wm(--cpu->sp, cpu->pch);
   cpu->MemIf->wm(--cpu->sp, cpu->pcl);
   cpu->pc = intad;
   cpu->memptr = intad;
   cpu->halted = 0;
   cpu->iff1 = cpu->iff2 = 0;
   cpu->int_pend = false;
}
