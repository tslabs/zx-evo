#pragma once
void set_mode(ROM_MODE mode);
void set_banks();
void set_scorp_profrom(unsigned read_address);
void cmos_write(unsigned char val);
unsigned char cmos_read();

Z80INLINE u8 *am_r(unsigned addr)
{
#ifdef MOD_VID_VD
   if (comp.vdbase && (unsigned)((addr & 0xFFFF) - 0x4000) < 0x1800) return comp.vdbase + (addr & 0x1FFF);
#endif
   return bankr[(addr >> 14) & 3] + (addr & (PAGE-1));
}
//u8 __fastcall rmdbg(u32 addr);
//void __fastcall wmdbg(u32 addr, u8 val);
//u8 *__fastcall MemDbg(u32 addr);
