#pragma once

#include "sysdefs.h"

struct Z80;

#define Z80FAST fastcall

// #define LOG_TAPE_IN
// #define LOG_FE_IN
// #define LOG_FE_OUT

#ifdef _MSC_VER
#define Z80INLINE forceinline // time-critical inlines
#else
#define Z80INLINE inline
#endif

typedef void (Z80FAST *STEPFUNC)(Z80*);
#define Z80OPCODE void Z80FAST
typedef u8 (Z80FAST *LOGICFUNC)(Z80*, u8 byte);
#define Z80LOGIC u8 Z80FAST

typedef union {
	u32 p;
	struct {
		u8 b;
		u8 g;
		u8 r;
		u8 a;
	};
} RGB32;

#define TS_CACHE_SIZE	512
struct TZ80State
{
   union
   {
		unsigned tt;
		struct
		{
			unsigned t_l:8;
			unsigned t:24;
		};
   };
    /*------------------------------*/
    union
    {
        unsigned pc;
        struct
        {
            u8 pcl;
            u8 pch;
        };
    };
    union
    {
        unsigned sp;
        struct
        {
            u8 spl;
            u8 sph;
        };
    };
    union
    {
        unsigned ir_;
        struct
        {
            u8 r_low;
            u8 i;
        };
    };
    union
    {
        unsigned int_flags;
        struct
        {
            u8 r_hi;
            u8 iff1;
            u8 iff2;
            u8 halted;
        };
    };
    /*------------------------------*/
    union
    {
        unsigned bc;
        u16 bc16;
        struct
        {
            u8 c;
            u8 b;
        };
    };
    union
    {
        unsigned de;
        struct
        {
            u8 e;
            u8 d;
        };
    };
    union
    {
        unsigned hl;
        struct
        {
            u8 l;
            u8 h;
        };
    };
    union
    {
        unsigned af;
        struct
        {
            u8 f;
            u8 a;
        };
    };
    /*------------------------------*/
    union
    {
        unsigned ix;
        struct
        {
            u8 xl;
            u8 xh;
        };
    };
    union
    {
        unsigned iy;
        struct
        {
            u8 yl;
            u8 yh;
        };
    };
    /*------------------------------*/
    struct
    {
        union
        {
            unsigned bc;
            struct
            {
                u8 c;
                u8 b;
            };
        };
        union
        {
            unsigned de;
            struct
            {
                u8 e;
                u8 d;
            };
        };
        union
        {
            unsigned hl;
            struct
            {
                u8 l;
                u8 h;
            };
        };
        union
        {
            unsigned af;
            struct
            {
                u8 f;
                u8 a;
            };
        };
    } alt;
    union
    {
        unsigned memptr; // undocumented register
        struct
        {
            u8 meml;
            u8 memh;
        };
    };
	u8 opcode;
    unsigned eipos, haltpos;
    /*------------------------------*/
    u8 im;
    bool nmi_in_progress;
	u32 tscache_addr[TS_CACHE_SIZE];
	u8  tscache_data[TS_CACHE_SIZE];
};

typedef u8 (* TRm)(u32 addr);
typedef void (* TWm)(u32 addr, u8 val);

struct TMemIf
{
    TRm rm;
    TWm wm;
};

struct Z80 : public TZ80State
{
   u8 tmp0, tmp1, tmp3;
   unsigned rate;
   u8 outc0;
   u16 last_branch;
   unsigned trace_curs, trace_top, trace_mode;
   unsigned mem_curs, mem_top, mem_second;
   unsigned pc_trflags;
   unsigned nextpc;
   unsigned dbg_stophere;
   unsigned dbg_stopsp;
   unsigned dbg_loop_r1;
   unsigned dbg_loop_r2;
   u8 dbgchk; // Признак наличия активных брекпоинтов
   bool int_pend; // На входе int есть активное прерывание
   bool int_gate; // Разрешение внешних прерываний (1-разрешены/0 - запрещены)

   #define MAX_CBP 16
   unsigned cbp[MAX_CBP][128]; // Условия для условных брекпоинтов
   unsigned cbpn;

   i64 debug_last_t; // used to find time delta
   u32 tpi; // Число тактов между прерываниями
   u32 trpc[40];
//   typedef u8 (* TRmDbg)(u32 addr);
//   typedef u8 *(* TMemDbg)(u32 addr);
//   typedef void (* TWmDbg)(u32 addr, u8 val);
   typedef void (__cdecl *TBankNames)(int i, char *Name);
   typedef void (Z80FAST * TStep)();
   typedef i64 (__cdecl * TDelta)();
   typedef void (__cdecl * TSetLastT)();
//   TRmDbg DirectRm; // direct read memory in debuger
//   TWmDbg DirectWm; // direct write memory in debuger
//   TMemDbg DirectMem; // get direct memory pointer in debuger
   u32 Idx; // Индекс в массиве процессоров
   TBankNames BankNames;
   TStep Step;
   TDelta Delta;
   TSetLastT SetLastT;
   u8 *membits;
   u8 dbgbreak;
   const TMemIf *FastMemIf; // Быстрый интерфес памяти
   const TMemIf *DbgMemIf; // Интерфейс памяти для поддержки отладчика (брекпоинты на доступ к памяти)
   const TMemIf *MemIf; // Текущий активный интерфейс памяти

   void reset() { int_flags = ir_ = pc = 0; im = 0; last_branch = 0; int_pend = false; int_gate = true; }
   Z80(u32 Idx, TBankNames BankNames, TStep Step, TDelta Delta,
       TSetLastT SetLastT, u8 *membits, const TMemIf *FastMemIf, const TMemIf *DbgMemIf) :
       Idx(Idx), 
       BankNames(BankNames),
       Step(Step), Delta(Delta), SetLastT(SetLastT), membits(membits),
       FastMemIf(FastMemIf), DbgMemIf(DbgMemIf)
   {
       MemIf = FastMemIf;
       tpi = 0;
	   rate = (1 << 8);
       dbgbreak = 0;
       dbgchk = 0;
       debug_last_t = 0;
       trace_curs = trace_top = (unsigned)-1; trace_mode = 0;
       mem_curs = mem_top = 0;
       pc_trflags = nextpc = 0;
       dbg_stophere = dbg_stopsp = (unsigned)-1;
       dbg_loop_r1 = 0;
       dbg_loop_r2 = 0xFFFF;
       int_pend = false;
       int_gate = true;
       nmi_in_progress = false;
   }
   virtual ~Z80() { }
   u32 GetIdx() const { return Idx; }
   void SetTpi(u32 Tpi) { tpi = Tpi; }

   void SetFastMemIf() { MemIf = FastMemIf; }
   void SetDbgMemIf() { MemIf = DbgMemIf; }

   u8 DirectRm(unsigned addr) const { return *DirectMem(addr); } // direct read memory in debuger
   void DirectWm(unsigned addr, u8 val) 
   { // direct write memory in debuger
     *DirectMem(addr) = val;
     u16 cache_pointer = addr & 0x1FE;
     tscache_addr[cache_pointer] = tscache_addr[cache_pointer + 1] = -1;	// write invalidates 16 bit
   }
/*
   virtual u8 rm(unsigned addr) = 0;
   virtual void wm(unsigned addr, u8 val) = 0;
   */
   virtual u8 *DirectMem(unsigned addr) const = 0; // get direct memory pointer in debuger

   virtual u8 rd(u32 addr) {
     tt += rate * 3;
     return MemIf->rm(addr);
   }

   virtual void wd(u32 addr, u8 val) {
      tt += rate * 3;
      MemIf->wm(addr, val);
   }

   virtual u8 in(unsigned port) = 0;
   virtual void out(unsigned port, u8 val) = 0;
   virtual u8 m1_cycle() = 0; // [vv] Не зависит от процессора (вынести в библиотеку)
   virtual u8 IntVec() = 0; // Функция возвращающая значение вектора прерывания для im2
   virtual void CheckNextFrame() = 0; // Проверка и обновления счетчика кадров и тактов внутри прерывания
   virtual void retn() = 0; // Вызывается в конце инструкции retn (должна сбрасывать флаг nmi_in_progress и обновлять раскладку памяти)
};

#define CF 0x01
#define NF 0x02
#define PV 0x04
#define F3 0x08
#define HF 0x10
#define F5 0x20
#define ZF 0x40
#define SF 0x80

#define cputact(a)	cpu->tt += ((a) * cpu->rate)
// #define cputact(a) cpu->t += (a)

#define turbo(a)	cpu.rate = (256/(a))
