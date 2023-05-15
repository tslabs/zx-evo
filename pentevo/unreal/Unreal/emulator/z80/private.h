#pragma once
#include "sysdefs.h"
#include "z80.h"

extern const u8 daatab[];

extern const u8 rl0[];
extern const u8 rl1[];

extern const u8 rr0[];
extern const u8 rr1[];

extern u8 rol[];
extern u8 ror[];

extern u8 rlcaf[];
extern u8 rrcaf[];
extern const u8 rlcf[];
extern const u8 rrcf[];

extern const u8 sraf[];
extern u8 log_f[];

extern const u8 incf[];
extern const u8 decf[];
extern u8 adcf[];
extern u8 sbcf[];

extern u8 cpf[];
extern u8 cpf8b[];

extern stepfunc const ix_opcode[];
extern stepfunc const ext_opcode[];
extern stepfunc const iy_opcode[];


#define op_40 op_00     // ld b,b
#define op_49 op_00     // ld c,c
#define op_52 op_00     // ld d,d
#define op_5B op_00     // ld e,e
#define op_64 op_00     // ld h,h
#define op_6D op_00     // ld l,l
#define op_7F op_00     // ld a,a

extern stepfunc const normal_opcode[];

void fastcall op_00(Z80& cpu);
void fastcall op_01(Z80& cpu);
void fastcall op_02(Z80& cpu);
void fastcall op_03(Z80& cpu);
void fastcall op_04(Z80& cpu);
void fastcall op_05(Z80& cpu);
void fastcall op_06(Z80& cpu);
void fastcall op_07(Z80& cpu);
void fastcall op_08(Z80& cpu);
void fastcall op_0A(Z80& cpu);
void fastcall op_0B(Z80& cpu);
void fastcall op_0C(Z80& cpu);
void fastcall op_0D(Z80& cpu);
void fastcall op_0E(Z80& cpu);
void fastcall op_0F(Z80& cpu);
void fastcall op_10(Z80& cpu);
void fastcall op_11(Z80& cpu);
void fastcall op_12(Z80& cpu);
void fastcall op_13(Z80& cpu);
void fastcall op_14(Z80& cpu);
void fastcall op_15(Z80& cpu);
void fastcall op_16(Z80& cpu);
void fastcall op_17(Z80& cpu);
void fastcall op_18(Z80& cpu);
void fastcall op_1A(Z80& cpu);
void fastcall op_1B(Z80& cpu);
void fastcall op_1C(Z80& cpu);
void fastcall op_1D(Z80& cpu);
void fastcall op_1E(Z80& cpu);
void fastcall op_1F(Z80& cpu);
void fastcall op_20(Z80& cpu);
void fastcall op_22(Z80& cpu);
void fastcall op_27(Z80& cpu);
void fastcall op_28(Z80& cpu);
void fastcall op_2A(Z80& cpu);
void fastcall op_2F(Z80& cpu);
void fastcall op_30(Z80& cpu);
void fastcall op_31(Z80& cpu);
void fastcall op_32(Z80& cpu);
void fastcall op_33(Z80& cpu);
void fastcall op_37(Z80& cpu);
void fastcall op_38(Z80& cpu);
void fastcall op_3A(Z80& cpu);
void fastcall op_3B(Z80& cpu);
void fastcall op_3C(Z80& cpu);
void fastcall op_3D(Z80& cpu);
void fastcall op_3E(Z80& cpu);
void fastcall op_3F(Z80& cpu);
void fastcall op_41(Z80& cpu);
void fastcall op_42(Z80& cpu);
void fastcall op_43(Z80& cpu);
void fastcall op_47(Z80& cpu);
void fastcall op_48(Z80& cpu);
void fastcall op_4A(Z80& cpu);
void fastcall op_4B(Z80& cpu);
void fastcall op_4F(Z80& cpu);
void fastcall op_50(Z80& cpu);
void fastcall op_51(Z80& cpu);
void fastcall op_53(Z80& cpu);
void fastcall op_57(Z80& cpu);
void fastcall op_58(Z80& cpu);
void fastcall op_59(Z80& cpu);
void fastcall op_5A(Z80& cpu);
void fastcall op_5F(Z80& cpu);
void fastcall op_76(Z80& cpu);
void fastcall op_78(Z80& cpu);
void fastcall op_79(Z80& cpu);
void fastcall op_7A(Z80& cpu);
void fastcall op_7B(Z80& cpu);
void fastcall op_80(Z80& cpu);
void fastcall op_81(Z80& cpu);
void fastcall op_82(Z80& cpu);
void fastcall op_83(Z80& cpu);
void fastcall op_87(Z80& cpu);
void fastcall op_88(Z80& cpu);
void fastcall op_89(Z80& cpu);
void fastcall op_8A(Z80& cpu);
void fastcall op_8B(Z80& cpu);
void fastcall op_8F(Z80& cpu);
void fastcall op_90(Z80& cpu);
void fastcall op_91(Z80& cpu);
void fastcall op_92(Z80& cpu);
void fastcall op_93(Z80& cpu);
void fastcall op_97(Z80& cpu);
void fastcall op_98(Z80& cpu);
void fastcall op_99(Z80& cpu);
void fastcall op_9A(Z80& cpu);
void fastcall op_9B(Z80& cpu);
void fastcall op_9F(Z80& cpu);
void fastcall op_A0(Z80& cpu);
void fastcall op_A1(Z80& cpu);
void fastcall op_A2(Z80& cpu);
void fastcall op_A3(Z80& cpu);
void fastcall op_A7(Z80& cpu);
void fastcall op_A8(Z80& cpu);
void fastcall op_A9(Z80& cpu);
void fastcall op_AA(Z80& cpu);
void fastcall op_AB(Z80& cpu);
void fastcall op_AF(Z80& cpu);
void fastcall op_B0(Z80& cpu);
void fastcall op_B1(Z80& cpu);
void fastcall op_B2(Z80& cpu);
void fastcall op_B3(Z80& cpu);
void fastcall op_B7(Z80& cpu);
void fastcall op_B8(Z80& cpu);
void fastcall op_B9(Z80& cpu);
void fastcall op_BA(Z80& cpu);
void fastcall op_BB(Z80& cpu);
void fastcall op_BF(Z80& cpu);
void fastcall op_C0(Z80& cpu);
void fastcall op_C1(Z80& cpu);
void fastcall op_C2(Z80& cpu);
void fastcall op_C3(Z80& cpu);
void fastcall op_C4(Z80& cpu);
void fastcall op_C5(Z80& cpu);
void fastcall op_C6(Z80& cpu);
void fastcall op_C7(Z80& cpu);
void fastcall op_C8(Z80& cpu);
void fastcall op_C9(Z80& cpu);
void fastcall op_CA(Z80& cpu);
void fastcall op_CB(Z80& cpu);
void fastcall op_CC(Z80& cpu);
void fastcall op_CD(Z80& cpu);
void fastcall op_CE(Z80& cpu);
void fastcall op_CF(Z80& cpu);
void fastcall op_D0(Z80& cpu);
void fastcall op_D1(Z80& cpu);
void fastcall op_D2(Z80& cpu);
void fastcall op_D3(Z80& cpu);
void fastcall op_D4(Z80& cpu);
void fastcall op_D5(Z80& cpu);
void fastcall op_D6(Z80& cpu);
void fastcall op_D7(Z80& cpu);
void fastcall op_D8(Z80& cpu);
void fastcall op_D9(Z80& cpu);
void fastcall op_DA(Z80& cpu);
void fastcall op_DB(Z80& cpu);
void fastcall op_DC(Z80& cpu);
void fastcall op_DD(Z80& cpu);
void fastcall op_DE(Z80& cpu);
void fastcall op_DF(Z80& cpu);
void fastcall op_E0(Z80& cpu);
void fastcall op_E2(Z80& cpu);
void fastcall op_E4(Z80& cpu);
void fastcall op_E6(Z80& cpu);
void fastcall op_E7(Z80& cpu);
void fastcall op_E8(Z80& cpu);
void fastcall op_EA(Z80& cpu);
void fastcall op_EB(Z80& cpu);
void fastcall op_EC(Z80& cpu);
void fastcall op_ED(Z80& cpu);
void fastcall op_EE(Z80& cpu);
void fastcall op_EF(Z80& cpu);
void fastcall op_F0(Z80& cpu);
void fastcall op_F1(Z80& cpu);
void fastcall op_F2(Z80& cpu);
void fastcall op_F3(Z80& cpu);
void fastcall op_F4(Z80& cpu);
void fastcall op_F5(Z80& cpu);
void fastcall op_F6(Z80& cpu);
void fastcall op_F7(Z80& cpu);
void fastcall op_F8(Z80& cpu);
void fastcall op_FA(Z80& cpu);
void fastcall op_FB(Z80& cpu);
void fastcall op_FC(Z80& cpu);
void fastcall op_FD(Z80& cpu);
void fastcall op_FE(Z80& cpu);
void fastcall op_FF(Z80& cpu);


forceinline void and8(Z80& cpu, u8 src)
{
	cpu.a &= src;
	cpu.f = log_f[cpu.a] | HF;
}

forceinline void or8(Z80& cpu, u8 src)
{
	cpu.a |= src;
	cpu.f = log_f[cpu.a];
}

forceinline void xor8(Z80& cpu, u8 src)
{
	cpu.a ^= src;
	cpu.f = log_f[cpu.a];
}

forceinline void bitmem(Z80& cpu, u8 src, u8 bit)
{
	cpu.f = log_f[src & (1 << bit)] | HF | (cpu.f & CF);
	cpu.f = (cpu.f & ~(F3 | F5)) | (cpu.memh & (F3 | F5));
}

forceinline void set(u8& src, u8 bit)
{
	src |= (1 << bit);
}

forceinline void res(u8& src, u8 bit)
{
	src &= ~(1 << bit);
}

forceinline void bit(Z80& cpu, u8 src, u8 bit)
{
	cpu.f = log_f[src & (1 << bit)] | HF | (cpu.f & CF) | (src & (F3 | F5));
}

forceinline u8 resbyte(u8 src, u8 bit)
{
	return src & ~(1 << bit);
}

forceinline u8 setbyte(u8 src, u8 bit)
{
	return src | (1 << bit);
}

forceinline void inc8(Z80& cpu, u8& x)
{
	cpu.f = incf[x] | (cpu.f & CF);
	x++;
}

forceinline void dec8(Z80& cpu, u8& x)
{
	cpu.f = decf[x] | (cpu.f & CF);
	x--;
}

forceinline void add8(Z80& cpu, u8 src)
{
	cpu.f = adcf[cpu.a + src * 0x100];
	cpu.a += src;
}

forceinline void sub8(Z80& cpu, u8 src)
{
	cpu.f = sbcf[cpu.a * 0x100 + src];
	cpu.a -= src;
}

forceinline void adc8(Z80& cpu, u8 src)
{
	u8 carry = ((cpu.f) & CF);
	cpu.f = adcf[cpu.a + src * 0x100 + 0x10000 * carry];
	cpu.a += src + carry;
}

forceinline void sbc8(Z80& cpu, u8 src)
{
	u8 carry = ((cpu.f) & CF);
	cpu.f = sbcf[cpu.a * 0x100 + src + 0x10000 * carry];
	cpu.a -= src + carry;
}

forceinline void cp8(Z80& cpu, u8 src)
{
	cpu.f = cpf[cpu.a * 0x100 + src];
}
