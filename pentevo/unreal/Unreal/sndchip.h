#pragma once
/*
   YM-2149F emulator for Unreal Speccy project
   created under public domain license by SMT, jan.2006
*/

#include "sysdefs.h"
#include "sndrender.h"

const unsigned SNDR_DEFAULT_AY_RATE = 1774400; // original ZX-Spectrum soundchip clock fq

struct AYOUT;
struct SNDCHIP_VOLTAB;
struct SNDCHIP_PANTAB;

extern const char * const ay_chips[];

#ifdef __GNUC__
#pragma pack(push,1)
#else
#pragma pack(push)
#pragma pack(1) // envelope period (envT, R13-R14) has invalid word alignment
#endif
struct AYREGS
{
   u16 fA, fB, fC;
   u8 noise, mix;
   u8 vA, vB, vC;
   u16 envT;
   u8 env;
   u8 portA, portB;
};
#pragma pack(pop)

class SNDCHIP : public SNDRENDER
{

 public:

   YM2203 * Chip2203; //registers //Dexus
   //FMSAMPLE FMbuf; //1 sample //Dexus
#define FMBUFSIZE (1) //Alone Coder
   FMSAMPLE FMbufs[FMBUFSIZE]; //Alone Coder
   u16 FMbufN; //Alone Coder
   int FMbufOUT; //1 sample for add to AY output //Alone Coder
   u16 FMbufMUL; //conf.sound.ay/8192*0.7f //Alone Coder
   float nextfmtickfloat,ayticks_per_fmtick; //Alone Coder
   u32 nextfmtick; //Alone Coder

   enum CHIP_TYPE { CHIP_AY, CHIP_YM, CHIP_YM2203, CHIP_MAX }; //Dexus
   static const char *get_chipname(CHIP_TYPE i) { return ay_chips[i]; }

   void set_chip(CHIP_TYPE type) { chiptype = type; }
   void set_timings(unsigned system_clock_rate, unsigned chip_clock_rate, unsigned sample_rate);
   void set_volumes(unsigned global_vol, const SNDCHIP_VOLTAB *voltab, const SNDCHIP_PANTAB *stereo);

   void reset(unsigned timestamp = 0); // call with default parameter, when context outside start_frame/end_frame block

   // 'render' is a function that converts array of register writes into PCM-buffer
   unsigned render(AYOUT *src, unsigned srclen, unsigned clk_ticks, bufptr_t dst);

   // set of functions that fills buffer in emulation progress
   void start_frame(bufptr_t dst);
   void select(u8 nreg);
   void write(unsigned timestamp, u8 val);
   u8 read();
   unsigned end_frame(unsigned clk_ticks);

   SNDCHIP();

   // new start position as previous end position
   // (continue to fill buffer)
   void start_frame() { start_frame(dstpos); }

   // for monitoring, chip can't report this values
   u8 get_activereg() { return activereg; }
   u8 get_r13_reloaded() { return r13_reloaded; }
   u8 get_reg(unsigned nreg) { return reg[nreg]; }
   unsigned get_env() { return env; }

 private:

   unsigned t, ta, tb, tc, tn, te, env;
   int denv;
   unsigned bitA, bitB, bitC, bitN, ns;
   unsigned bit0, bit1, bit2, bit3, bit4, bit5;
   unsigned ea, eb, ec, va, vb, vc;
   unsigned fa, fb, fc, fn, fe;
   unsigned mult_const;

   u8 activereg, r13_reloaded;

   unsigned vols[6][32];
   CHIP_TYPE chiptype;

   union {
      u8 reg[16];
      struct AYREGS r;
   };

   unsigned chip_clock_rate, system_clock_rate;
   u64 passed_chip_ticks, passed_clk_ticks;
   void flush(unsigned chiptick);
   void apply_regs(unsigned timestamp = 0);
};

struct AYOUT
{
   unsigned timestamp; // in system ticks
   u8 reg_num;
   u8 reg_value;
   u8 res1, res2; // padding
};

// output volumes (#0000-#FFFF) for given envelope state or R8-R10 value
// AY chip has only 16 different volume values, so v[0]=v[1], v[2]=v[3], ...
struct SNDCHIP_VOLTAB
{
   unsigned v[32];
};

// generator's channel panning, % (0-100)
struct SNDCHIP_PANTAB
{
   unsigned raw[6];
   // structured as 'struct { unsigned left, right; } chan[3]';
};

extern const SNDCHIP_VOLTAB SNDR_VOL_AY_S;
extern const SNDCHIP_VOLTAB SNDR_VOL_YM_S;
extern const SNDCHIP_PANTAB SNDR_PAN_MONO_S;
extern const SNDCHIP_PANTAB SNDR_PAN_ABC_S;
extern const SNDCHIP_PANTAB SNDR_PAN_ACB_S;
extern const SNDCHIP_PANTAB SNDR_PAN_BAC_S;
extern const SNDCHIP_PANTAB SNDR_PAN_BCA_S;
extern const SNDCHIP_PANTAB SNDR_PAN_CAB_S;
extern const SNDCHIP_PANTAB SNDR_PAN_CBA_S;

// used as parameters to SNDCHIP::set_volumes(),
// if application don't want to override defaults
const SNDCHIP_VOLTAB * const SNDR_VOL_AY = &SNDR_VOL_AY_S;
const SNDCHIP_VOLTAB * const SNDR_VOL_YM = &SNDR_VOL_YM_S;
const SNDCHIP_PANTAB * const SNDR_PAN_MONO = &SNDR_PAN_MONO_S;
const SNDCHIP_PANTAB * const SNDR_PAN_ABC = &SNDR_PAN_ABC_S;
const SNDCHIP_PANTAB * const SNDR_PAN_ACB = &SNDR_PAN_ACB_S;
const SNDCHIP_PANTAB * const SNDR_PAN_BAC = &SNDR_PAN_BAC_S;
const SNDCHIP_PANTAB * const SNDR_PAN_BCA = &SNDR_PAN_BCA_S;
const SNDCHIP_PANTAB * const SNDR_PAN_CAB = &SNDR_PAN_CAB_S;
const SNDCHIP_PANTAB * const SNDR_PAN_CBA = &SNDR_PAN_CBA_S;
