#include "defs.h"
#include "tables.h"
#include "op_noprefix.h"
#include "op_dd.h"

/* DD prefix opcodes */

//#ifdef Z80_COMMON
Z80OPCODE opx_09(Z80 *cpu) { // add ix,bc
   cpu->memptr = cpu->ix+1;
   cpu->f = (cpu->f & ~(NF | CF | F5 | F3 | HF));
   cpu->f |= (((cpu->ix & 0x0FFF) + (cpu->bc & 0x0FFF)) >> 8) & 0x10; /* HF */
   cpu->ix = (cpu->ix & 0xFFFF) + (cpu->bc & 0xFFFF);
   if (cpu->ix & 0x10000) cpu->f |= CF;
   cpu->f |= (cpu->xh & (F5 | F3));
   cputact(7);
}
Z80OPCODE opx_19(Z80 *cpu) { // add ix,de
   cpu->memptr = cpu->ix+1;
   cpu->f = (cpu->f & ~(NF | CF | F5 | F3 | HF));
   cpu->f |= (((cpu->ix & 0x0FFF) + (cpu->de & 0x0FFF)) >> 8) & 0x10; /* HF */
   cpu->ix = (cpu->ix & 0xFFFF) + (cpu->de & 0xFFFF);
   if (cpu->ix & 0x10000) cpu->f |= CF;
   cpu->f |= (cpu->xh & (F5 | F3));
   cputact(7);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_21(Z80 *cpu) { // ld ix,nnnn
   cpu->xl = cpu->rd(cpu->pc++);
   cpu->xh = cpu->rd(cpu->pc++);
}
Z80OPCODE opx_22(Z80 *cpu) { // ld (nnnn),ix
   unsigned adr = cpu->rd(cpu->pc++);
   adr += cpu->rd(cpu->pc++)*0x100;
   cpu->memptr = adr+1;
   cpu->wd(adr, cpu->xl);
   cpu->wd(adr+1, cpu->xh);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_23(Z80 *cpu) { // inc ix
   cpu->ix++;
   cputact(2);
}
Z80OPCODE opx_24(Z80 *cpu) { // inc xh
   inc8(cpu, cpu->xh);
}
Z80OPCODE opx_25(Z80 *cpu) { // dec xh
   dec8(cpu, cpu->xh);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_26(Z80 *cpu) { // ld xh,nn
   cpu->xh = cpu->rd(cpu->pc++);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_29(Z80 *cpu) { // add ix,ix
   cpu->memptr = cpu->ix+1;
   cpu->f = (cpu->f & ~(NF | CF | F5 | F3 | HF));
   cpu->f |= ((cpu->ix >> 7) & 0x10); /* HF */
   cpu->ix = (cpu->ix & 0xFFFF)*2;
   if (cpu->ix & 0x10000) cpu->f |= CF;
   cpu->f |= (cpu->xh & (F5 | F3));
   cputact(7);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_2A(Z80 *cpu) { // ld ix,(nnnn)
   unsigned adr = cpu->rd(cpu->pc++);
   adr += cpu->rd(cpu->pc++)*0x100;
   cpu->memptr = adr+1;
   cpu->xl = cpu->rd(adr);
   cpu->xh = cpu->rd(adr+1);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_2B(Z80 *cpu) { // dec ix
   cpu->ix--;
   cputact(2);
}
Z80OPCODE opx_2C(Z80 *cpu) { // inc xl
   inc8(cpu, cpu->xl);
}
Z80OPCODE opx_2D(Z80 *cpu) { // dec xl
   dec8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_2E(Z80 *cpu) { // ld xl,nn
   cpu->xl = cpu->rd(cpu->pc++);
}
Z80OPCODE opx_34(Z80 *cpu) { // inc (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   u8 t = cpu->rd(cpu->ix + ofs);
   inc8(cpu, t);
   cputact(1);
   cpu->wd(cpu->ix + ofs, t);
}
Z80OPCODE opx_35(Z80 *cpu) { // dec (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   u8 t = cpu->rd(cpu->ix + ofs);
   dec8(cpu, t);
   cputact(1);
   cpu->wd(cpu->ix + ofs, t);
}
Z80OPCODE opx_36(Z80 *cpu) { // ld (ix+nn),nn
   char ofs = cpu->rd(cpu->pc++);
   u8 tempbyte = cpu->rd(cpu->pc++);
   cputact(2);
   cpu->wd(cpu->ix + ofs, tempbyte);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_39(Z80 *cpu) { // add ix,sp
   cpu->memptr = cpu->ix+1;
   cpu->f = (cpu->f & ~(NF | CF | F5 | F3 | HF));
   cpu->f |= (((cpu->ix & 0x0FFF) + (cpu->sp & 0x0FFF)) >> 8) & 0x10; /* HF */
   cpu->ix = (cpu->ix & 0xFFFF) + (cpu->sp & 0xFFFF);
   if (cpu->ix & 0x10000) cpu->f |= CF;
   cpu->f |= (cpu->xh & (F5 | F3));
   cputact(7);
}
Z80OPCODE opx_44(Z80 *cpu) { // ld b,xh
   cpu->b = cpu->xh;
}
Z80OPCODE opx_45(Z80 *cpu) { // ld b,xl
   cpu->b = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_46(Z80 *cpu) { // ld b,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->b = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_4C(Z80 *cpu) { // ld c,xh
   cpu->c = cpu->xh;
}
Z80OPCODE opx_4D(Z80 *cpu) { // ld c,xl
   cpu->c = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_4E(Z80 *cpu) { // ld c,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->c = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_54(Z80 *cpu) { // ld d,xh
   cpu->d = cpu->xh;
}
Z80OPCODE opx_55(Z80 *cpu) { // ld d,xl
   cpu->d = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_56(Z80 *cpu) { // ld d,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->d = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_5C(Z80 *cpu) { // ld e,xh
   cpu->e = cpu->xh;
}
Z80OPCODE opx_5D(Z80 *cpu) { // ld e,xl
   cpu->e = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_5E(Z80 *cpu) { // ld e,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->e = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_60(Z80 *cpu) { // ld xh,b
   cpu->xh = cpu->b;
}
Z80OPCODE opx_61(Z80 *cpu) { // ld xh,c
   cpu->xh = cpu->c;
}
Z80OPCODE opx_62(Z80 *cpu) { // ld xh,d
   cpu->xh = cpu->d;
}
Z80OPCODE opx_63(Z80 *cpu) { // ld xh,e
   cpu->xh = cpu->e;
}
Z80OPCODE opx_65(Z80 *cpu) { // ld xh,xl
   cpu->xh = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_66(Z80 *cpu) { // ld h,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->h = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_67(Z80 *cpu) { // ld xh,a
   cpu->xh = cpu->a;
}
Z80OPCODE opx_68(Z80 *cpu) { // ld xl,b
   cpu->xl = cpu->b;
}
Z80OPCODE opx_69(Z80 *cpu) { // ld xl,c
   cpu->xl = cpu->c;
}
Z80OPCODE opx_6A(Z80 *cpu) { // ld xl,d
   cpu->xl = cpu->d;
}
Z80OPCODE opx_6B(Z80 *cpu) { // ld xl,e
   cpu->xl = cpu->e;
}
Z80OPCODE opx_6C(Z80 *cpu) { // ld xl,xh
   cpu->xl = cpu->xh;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_6E(Z80 *cpu) { // ld l,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->l = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_6F(Z80 *cpu) { // ld xl,a
   cpu->xl = cpu->a;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_70(Z80 *cpu) { // ld (ix+nn),b
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->b);
}
Z80OPCODE opx_71(Z80 *cpu) { // ld (ix+nn),c
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->c);
}
Z80OPCODE opx_72(Z80 *cpu) { // ld (ix+nn),d
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->d);
}
Z80OPCODE opx_73(Z80 *cpu) { // ld (ix+nn),e
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->e);
}
Z80OPCODE opx_74(Z80 *cpu) { // ld (ix+nn),h
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->h);
}
Z80OPCODE opx_75(Z80 *cpu) { // ld (ix+nn),l
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->l);
}
Z80OPCODE opx_77(Z80 *cpu) { // ld (ix+nn),a
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->wd(cpu->ix + ofs, cpu->a);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_7C(Z80 *cpu) { // ld a,xh
   cpu->a = cpu->xh;
}
Z80OPCODE opx_7D(Z80 *cpu) { // ld a,xl
   cpu->a = cpu->xl;
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_7E(Z80 *cpu) { // ld a,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cpu->a = cpu->rd(cpu->ix + ofs);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_84(Z80 *cpu) { // add a,xh
   add8(cpu, cpu->xh);
}
Z80OPCODE opx_85(Z80 *cpu) { // add a,xl
   add8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_86(Z80 *cpu) { // add a,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   add8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_8C(Z80 *cpu) { // adc a,xh
   adc8(cpu, cpu->xh);
}
Z80OPCODE opx_8D(Z80 *cpu) { // adc a,xl
   adc8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_8E(Z80 *cpu) { // adc a,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   adc8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_94(Z80 *cpu) { // sub xh
   sub8(cpu, cpu->xh);
}
Z80OPCODE opx_95(Z80 *cpu) { // sub xl
   sub8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_96(Z80 *cpu) { // sub (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   sub8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_9C(Z80 *cpu) { // sbc a,xh
   sbc8(cpu, cpu->xh);
}
Z80OPCODE opx_9D(Z80 *cpu) { // sbc a,xl
   sbc8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_9E(Z80 *cpu) { // sbc a,(ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   sbc8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_A4(Z80 *cpu) { // and xh
   and8(cpu, cpu->xh);
}
Z80OPCODE opx_A5(Z80 *cpu) { // and xl
   and8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_A6(Z80 *cpu) { // and (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   and8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_AC(Z80 *cpu) { // xor xh
   xor8(cpu, cpu->xh);
}
Z80OPCODE opx_AD(Z80 *cpu) { // xor xl
   xor8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_AE(Z80 *cpu) { // xor (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   xor8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_B4(Z80 *cpu) { // or xh
   or8(cpu, cpu->xh);
}
Z80OPCODE opx_B5(Z80 *cpu) { // or xl
   or8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_B6(Z80 *cpu) { // or (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   or8(cpu, cpu->rd(cpu->ix + ofs));
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_BC(Z80 *cpu) { // cp xh
   cp8(cpu, cpu->xh);
}
Z80OPCODE opx_BD(Z80 *cpu) { // cp xl
   cp8(cpu, cpu->xl);
}
//#endif
//#ifndef Z80_COMMON
Z80OPCODE opx_BE(Z80 *cpu) { // cp (ix+nn)
   char ofs = cpu->rd(cpu->pc++);
   cputact(5);
   cp8(cpu, cpu->rd(cpu->ix + ofs));
}
Z80OPCODE opx_E1(Z80 *cpu) { // pop ix
   cpu->xl = cpu->rd(cpu->sp++);
   cpu->xh = cpu->rd(cpu->sp++);
}
Z80OPCODE opx_E3(Z80 *cpu) { // ex (sp),ix
   unsigned tmp = cpu->rd(cpu->sp) + 0x100*cpu->rd(cpu->sp + 1);
   cputact(1);
   cpu->wd(cpu->sp, cpu->xl);
   cpu->wd(cpu->sp+1, cpu->xh);
   cpu->memptr = tmp;
   cpu->ix = tmp;
   cputact(2);
}
Z80OPCODE opx_E5(Z80 *cpu) { // push ix
   cputact(1);
   cpu->wd(--cpu->sp, cpu->xh);
   cpu->wd(--cpu->sp, cpu->xl);
}
//#endif
//#ifdef Z80_COMMON
Z80OPCODE opx_E9(Z80 *cpu) { // jp (ix)
   cpu->last_branch = cpu->pc-2;
   cpu->pc = cpu->ix;
}
Z80OPCODE opx_F9(Z80 *cpu) { // ld sp,ix
   cpu->sp = cpu->ix;
   cputact(2);
}
//#endif
//#ifndef Z80_COMMON

STEPFUNC const ix_opcode[0x100] = {

    op_00,  op_01,  op_02,  op_03,  op_04,  op_05,  op_06,  op_07,
    op_08, opx_09,  op_0A,  op_0B,  op_0C,  op_0D,  op_0E,  op_0F,
    op_10,  op_11,  op_12,  op_13,  op_14,  op_15,  op_16,  op_17,
    op_18, opx_19,  op_1A,  op_1B,  op_1C,  op_1D,  op_1E,  op_1F,
    op_20, opx_21, opx_22, opx_23, opx_24, opx_25, opx_26,  op_27,
    op_28, opx_29, opx_2A, opx_2B, opx_2C, opx_2D, opx_2E,  op_2F,
    op_30,  op_31,  op_32,  op_33, opx_34, opx_35, opx_36,  op_37,
    op_38, opx_39,  op_3A,  op_3B,  op_3C,  op_3D,  op_3E,  op_3F,

    op_40,  op_41,  op_42,  op_43, opx_44, opx_45, opx_46,  op_47,
    op_48,  op_49,  op_4A,  op_4B, opx_4C, opx_4D, opx_4E,  op_4F,
    op_50,  op_51,  op_52,  op_53, opx_54, opx_55, opx_56,  op_57,
    op_58,  op_59,  op_5A,  op_5B, opx_5C, opx_5D, opx_5E,  op_5F,
   opx_60, opx_61, opx_62, opx_63,  op_64, opx_65, opx_66, opx_67,
   opx_68, opx_69, opx_6A, opx_6B, opx_6C,  op_6D, opx_6E, opx_6F,
   opx_70, opx_71, opx_72, opx_73, opx_74, opx_75,  op_76, opx_77,
    op_78,  op_79,  op_7A,  op_7B, opx_7C, opx_7D, opx_7E,  op_7F,

    op_80,  op_81,  op_82,  op_83, opx_84, opx_85, opx_86,  op_87,
    op_88,  op_89,  op_8A,  op_8B, opx_8C, opx_8D, opx_8E,  op_8F,
    op_90,  op_91,  op_92,  op_93, opx_94, opx_95, opx_96,  op_97,
    op_98,  op_99,  op_9A,  op_9B, opx_9C, opx_9D, opx_9E,  op_9F,
    op_A0,  op_A1,  op_A2,  op_A3, opx_A4, opx_A5, opx_A6,  op_A7,
    op_A8,  op_A9,  op_AA,  op_AB, opx_AC, opx_AD, opx_AE,  op_AF,
    op_B0,  op_B1,  op_B2,  op_B3, opx_B4, opx_B5, opx_B6,  op_B7,
    op_B8,  op_B9,  op_BA,  op_BB, opx_BC, opx_BD, opx_BE,  op_BF,

    op_C0,  op_C1,  op_C2,  op_C3,  op_C4,  op_C5,  op_C6,  op_C7,
    op_C8,  op_C9,  op_CA,  op_CB,  op_CC,  op_CD,  op_CE,  op_CF,
    op_D0,  op_D1,  op_D2,  op_D3,  op_D4,  op_D5,  op_D6,  op_D7,
    op_D8,  op_D9,  op_DA,  op_DB,  op_DC,  op_DD,  op_DE,  op_DF,
    op_E0, opx_E1,  op_E2, opx_E3,  op_E4, opx_E5,  op_E6,  op_E7,
    op_E8, opx_E9,  op_EA,  op_EB,  op_EC,  op_ED,  op_EE,  op_EF,
    op_F0,  op_F1,  op_F2,  op_F3,  op_F4,  op_F5,  op_F6,  op_F7,
    op_F8, opx_F9,  op_FA,  op_FB,  op_FC,  op_FD,  op_FE,  op_FF,

};

//#endif
