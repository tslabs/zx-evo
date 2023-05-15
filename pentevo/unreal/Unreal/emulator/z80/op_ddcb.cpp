#include "std.h"
#include "private.h"


/* DDCB/FDCB opcodes */
/* note: cpu.t and destination updated in step(), here process 'byte' */

//#ifdef Z80_COMMON
u8 fastcall oplx_00(Z80& cpu, u8 byte) { // rlc (ix+nn)
   cpu.f = rlcf[byte]; return rol[byte];
}
u8 fastcall oplx_08(Z80& cpu, u8 byte) { // rrc (ix+nn)
   cpu.f = rrcf[byte]; return ror[byte];
}
u8 fastcall oplx_10(Z80& cpu, u8 byte) { // rl (ix+nn)
   if (cpu.f & CF) {
      cpu.f = rl1[byte]; return (byte << 1) + 1;
   } else {
      cpu.f = rl0[byte]; return (byte << 1);
   }
}
u8 fastcall oplx_18(Z80& cpu, u8 byte) { // rr (ix+nn)
   if (cpu.f & CF) {
      cpu.f = rr1[byte]; return (byte >> 1) + 0x80;
   } else {
      cpu.f = rr0[byte]; return (byte >> 1);
   }
}
u8 fastcall oplx_20(Z80& cpu, u8 byte) { // sla (ix+nn)
   cpu.f = rl0[byte]; return (byte << 1);
}
u8 fastcall oplx_28(Z80& cpu, u8 byte) { // sra (ix+nn)
   cpu.f = sraf[byte]; return (byte >> 1) + (byte & 0x80);
}
u8 fastcall oplx_30(Z80& cpu, u8 byte) { // sli (ix+nn)
   cpu.f = rl1[byte]; return (byte << 1) + 1;
}
u8 fastcall oplx_38(Z80& cpu, u8 byte) { // srl (ix+nn)
   cpu.f = rr0[byte]; return (byte >> 1);
}
u8 fastcall oplx_40(Z80& cpu, u8 byte) { // bit 0,(ix+nn)
   bitmem(cpu, byte, 0); return byte;
}
u8 fastcall oplx_48(Z80& cpu, u8 byte) { // bit 1,(ix+nn)
   bitmem(cpu, byte, 1); return byte;
}
u8 fastcall oplx_50(Z80& cpu, u8 byte) { // bit 2,(ix+nn)
   bitmem(cpu, byte, 2); return byte;
}
u8 fastcall oplx_58(Z80& cpu, u8 byte) { // bit 3,(ix+nn)
   bitmem(cpu, byte, 3); return byte;
}
u8 fastcall oplx_60(Z80& cpu, u8 byte) { // bit 4,(ix+nn)
   bitmem(cpu, byte, 4); return byte;
}
u8 fastcall oplx_68(Z80& cpu, u8 byte) { // bit 5,(ix+nn)
   bitmem(cpu, byte, 5); return byte;
}
u8 fastcall oplx_70(Z80& cpu, u8 byte) { // bit 6,(ix+nn)
   bitmem(cpu, byte, 6); return byte;
}
u8 fastcall oplx_78(Z80& cpu, u8 byte) { // bit 7,(ix+nn)
   bitmem(cpu, byte, 7); return byte;
}
u8 fastcall oplx_80(Z80& cpu, u8 byte) { // res 0,(ix+nn)
   return resbyte(byte, 0);
}
u8 fastcall oplx_88(Z80& cpu, u8 byte) { // res 1,(ix+nn)
   return resbyte(byte, 1);
}
u8 fastcall oplx_90(Z80& cpu, u8 byte) { // res 2,(ix+nn)
   return resbyte(byte, 2);
}
u8 fastcall oplx_98(Z80& cpu, u8 byte) { // res 3,(ix+nn)
   return resbyte(byte, 3);
}
u8 fastcall oplx_A0(Z80& cpu, u8 byte) { // res 4,(ix+nn)
   return resbyte(byte, 4);
}
u8 fastcall oplx_A8(Z80& cpu, u8 byte) { // res 5,(ix+nn)
   return resbyte(byte, 5);
}
u8 fastcall oplx_B0(Z80& cpu, u8 byte) { // res 6,(ix+nn)
   return resbyte(byte, 6);
}
u8 fastcall oplx_B8(Z80& cpu, u8 byte) { // res 7,(ix+nn)
   return resbyte(byte, 7);
}
u8 fastcall oplx_C0(Z80& cpu, u8 byte) { // set 0,(ix+nn)
   return setbyte(byte, 0);
}
u8 fastcall oplx_C8(Z80& cpu, u8 byte) { // set 1,(ix+nn)
   return setbyte(byte, 1);
}
u8 fastcall oplx_D0(Z80& cpu, u8 byte) { // set 2,(ix+nn)
   return setbyte(byte, 2);
}
u8 fastcall oplx_D8(Z80& cpu, u8 byte) { // set 3,(ix+nn)
   return setbyte(byte, 3);
}
u8 fastcall oplx_E0(Z80& cpu, u8 byte) { // set 4,(ix+nn)
   return setbyte(byte, 4);
}
u8 fastcall oplx_E8(Z80& cpu, u8 byte) { // set 5,(ix+nn)
   return setbyte(byte, 5);
}
u8 fastcall oplx_F0(Z80& cpu, u8 byte) { // set 6,(ix+nn)
   return setbyte(byte, 6);
}
u8 fastcall oplx_F8(Z80& cpu, u8 byte) { // set 7,(ix+nn)
   return setbyte(byte, 7);
}

logicfunc const logic_ix_opcode[0x100] = {

   oplx_00, oplx_00, oplx_00, oplx_00, oplx_00, oplx_00, oplx_00, oplx_00,
   oplx_08, oplx_08, oplx_08, oplx_08, oplx_08, oplx_08, oplx_08, oplx_08,
   oplx_10, oplx_10, oplx_10, oplx_10, oplx_10, oplx_10, oplx_10, oplx_10,
   oplx_18, oplx_18, oplx_18, oplx_18, oplx_18, oplx_18, oplx_18, oplx_18,
   oplx_20, oplx_20, oplx_20, oplx_20, oplx_20, oplx_20, oplx_20, oplx_20,
   oplx_28, oplx_28, oplx_28, oplx_28, oplx_28, oplx_28, oplx_28, oplx_28,
   oplx_30, oplx_30, oplx_30, oplx_30, oplx_30, oplx_30, oplx_30, oplx_30,
   oplx_38, oplx_38, oplx_38, oplx_38, oplx_38, oplx_38, oplx_38, oplx_38,

   oplx_40, oplx_40, oplx_40, oplx_40, oplx_40, oplx_40, oplx_40, oplx_40,
   oplx_48, oplx_48, oplx_48, oplx_48, oplx_48, oplx_48, oplx_48, oplx_48,
   oplx_50, oplx_50, oplx_50, oplx_50, oplx_50, oplx_50, oplx_50, oplx_50,
   oplx_58, oplx_58, oplx_58, oplx_58, oplx_58, oplx_58, oplx_58, oplx_58,
   oplx_60, oplx_60, oplx_60, oplx_60, oplx_60, oplx_60, oplx_60, oplx_60,
   oplx_68, oplx_68, oplx_68, oplx_68, oplx_68, oplx_68, oplx_68, oplx_68,
   oplx_70, oplx_70, oplx_70, oplx_70, oplx_70, oplx_70, oplx_70, oplx_70,
   oplx_78, oplx_78, oplx_78, oplx_78, oplx_78, oplx_78, oplx_78, oplx_78,

   oplx_80, oplx_80, oplx_80, oplx_80, oplx_80, oplx_80, oplx_80, oplx_80,
   oplx_88, oplx_88, oplx_88, oplx_88, oplx_88, oplx_88, oplx_88, oplx_88,
   oplx_90, oplx_90, oplx_90, oplx_90, oplx_90, oplx_90, oplx_90, oplx_90,
   oplx_98, oplx_98, oplx_98, oplx_98, oplx_98, oplx_98, oplx_98, oplx_98,
   oplx_A0, oplx_A0, oplx_A0, oplx_A0, oplx_A0, oplx_A0, oplx_A0, oplx_A0,
   oplx_A8, oplx_A8, oplx_A8, oplx_A8, oplx_A8, oplx_A8, oplx_A8, oplx_A8,
   oplx_B0, oplx_B0, oplx_B0, oplx_B0, oplx_B0, oplx_B0, oplx_B0, oplx_B0,
   oplx_B8, oplx_B8, oplx_B8, oplx_B8, oplx_B8, oplx_B8, oplx_B8, oplx_B8,

   oplx_C0, oplx_C0, oplx_C0, oplx_C0, oplx_C0, oplx_C0, oplx_C0, oplx_C0,
   oplx_C8, oplx_C8, oplx_C8, oplx_C8, oplx_C8, oplx_C8, oplx_C8, oplx_C8,
   oplx_D0, oplx_D0, oplx_D0, oplx_D0, oplx_D0, oplx_D0, oplx_D0, oplx_D0,
   oplx_D8, oplx_D8, oplx_D8, oplx_D8, oplx_D8, oplx_D8, oplx_D8, oplx_D8,
   oplx_E0, oplx_E0, oplx_E0, oplx_E0, oplx_E0, oplx_E0, oplx_E0, oplx_E0,
   oplx_E8, oplx_E8, oplx_E8, oplx_E8, oplx_E8, oplx_E8, oplx_E8, oplx_E8,
   oplx_F0, oplx_F0, oplx_F0, oplx_F0, oplx_F0, oplx_F0, oplx_F0, oplx_F0,
   oplx_F8, oplx_F8, oplx_F8, oplx_F8, oplx_F8, oplx_F8, oplx_F8, oplx_F8,

};

// offsets to b,c,d,e,h,l,<unused>,a  from cpu.c
unsigned reg_offset[] = { 1,0, 5,4, 9,8, 2,13 };
//#endif

//#ifndef Z80_COMMON
forceinline void ddfd(Z80& cpu, u8 opcode)
{
   u8 op1; // last DD/FD prefix
   do {
      op1 = opcode;
      opcode = cpu.m1_cycle();
   } while ((opcode | 0x20) == 0xFD); // opcode == DD/FD

   if (opcode == 0xCB) {

      unsigned ptr; // pointer to DDCB operand
      ptr = ((op1 == 0xDD) ? cpu.ix:cpu.iy) + (char)cpu.rd(cpu.pc++);
      cpu.memptr = ptr;
      // DDCBnnXX,FDCBnnXX increment R by 2, not 3!
      opcode = cpu.m1_cycle(); cpu.r_low--;
	  CPUTACT(1);

      u8 byte = (logic_ix_opcode[opcode])(cpu, cpu.rd(ptr));     
	  CPUTACT(1);
      if ((opcode & 0xC0) == 0x40) return; // bit n,rm

      // select destination register for shift/res/set
      *(&cpu.c + reg_offset[opcode & 7]) = byte;
      cpu.wd(ptr, byte);
      return;
   }

   if (opcode == 0xED) {
      opcode = cpu.m1_cycle();
      (ext_opcode[opcode])(cpu);
      return;
   }

   // one prefix: DD/FD
   ((op1 == 0xDD) ? ix_opcode[opcode] : iy_opcode[opcode])(cpu);
}

void fastcall op_DD(Z80& cpu)
{
   ddfd(cpu, 0xDD);
}

void fastcall op_FD(Z80& cpu)
{
   ddfd(cpu, 0xFD);
}
//#endif
