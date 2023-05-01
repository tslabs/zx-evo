#pragma once

#include "sysdefs.h"

struct Z80;

constexpr auto z80_fq = 3500000;

#define Z80FAST fastcall

#ifdef _MSC_VER
#define Z80INLINE forceinline // time-critical inlines
#else
#define Z80INLINE inline
#endif

using stepfunc = void(Z80FAST *)(Z80 &);
#define Z80OPCODE void Z80FAST
using logicfunc = u8(Z80FAST *)(Z80 &, u8 byte);
#define Z80LOGIC u8 Z80FAST

typedef union
{
	u32 p;
	struct
	{
		u8 b;
		u8 g;
		u8 r;
		u8 a;
	};
} RGB32;

constexpr auto ts_cache_size = 512;
constexpr auto z80_pc_history_size = 32;

struct pc_history_t
{
	u8 page;
	u16 addr;
};

struct z80_state_t
{
	union
	{
		unsigned tt;
		struct
		{
			unsigned t_l : 8;
			unsigned t : 24;
		};
	};

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

	u8 im;
	bool nmi_in_progress;
	u32 tscache_addr[ts_cache_size];
	u8 tscache_data[ts_cache_size];

	pc_history_t pc_hist[z80_pc_history_size];
	u32 pc_hist_ptr;
};

struct t_mem_if
{
	using t_rm = u8 (*)(u32 addr);
	using t_wm = void (*)(u32 addr, u8 val);
	t_rm rm;
	t_wm wm;
};

constexpr auto max_cbp = 16;

struct Z80 : z80_state_t
{
	using t_bank_names = void(__cdecl *)(int i, char *name);
	using step_fn = void(Z80FAST *)();
	using delta_fn = i64(__cdecl *)();
	using set_lastt_fn = void(__cdecl *)();

	unsigned rate;
	bool vm1{}; // halt handling type
	u8 outc0{};
	u16 last_branch{};
	unsigned trace_curs, trace_top, trace_mode;
	unsigned mem_curs, mem_top, mem_second{};
	unsigned pc_trflags;
	unsigned nextpc;
	unsigned dbg_stophere;
	unsigned dbg_stopsp;
	unsigned dbg_loop_r1;
	unsigned dbg_loop_r2;
	u8 dbgchk;	   // Признак наличия активных брекпоинтов
	bool int_pend; // На входе int есть активное прерывание
	bool int_gate; // Разрешение внешних прерываний (1-разрешены/0 - запрещены)
	unsigned halt_cycle{};

	unsigned cbp[max_cbp][128]{}; // Условия для условных брекпоинтов
	unsigned cbpn{};

	i64 debug_last_t; // used to find time delta
	u32 tpi;		  // Число тактов между прерываниями
	u32 trpc[40]{};

	u32 idx; // Индекс в массиве процессоров
	t_bank_names bank_names;
	step_fn step;
	delta_fn delta;
	set_lastt_fn set_last_t;
	u8 *membits;
	u8 dbgbreak;
	const t_mem_if *fast_mem_if; // Быстрый интерфес памяти
	const t_mem_if *dbg_mem_if;	 // Интерфейс памяти для поддержки отладчика (брекпоинты на доступ к памяти)
	const t_mem_if *mem_if;		 // Текущий активный интерфейс памяти

	void reset()
	{
		int_flags = ir_ = pc = 0;
		im = 0;
		last_branch = 0;
		int_pend = false;
		int_gate = true;
	}

	Z80(u32 Idx, t_bank_names BankNames, step_fn Step, delta_fn Delta, set_lastt_fn SetLastT, u8 *membits, const t_mem_if *FastMemIf, const t_mem_if *DbgMemIf)
		: z80_state_t(),
		idx(Idx),
		bank_names(BankNames),
		step(Step), delta(Delta), set_last_t(SetLastT), membits(membits),
		fast_mem_if(FastMemIf), dbg_mem_if(DbgMemIf)
	{
		mem_if = FastMemIf;
		tpi = 0;
		rate = (1 << 8);
		dbgbreak = 0;
		dbgchk = 0;
		debug_last_t = 0;
		trace_curs = trace_top = (unsigned)-1;
		trace_mode = 0;
		mem_curs = mem_top = 0;
		pc_trflags = nextpc = 0;
		dbg_stophere = dbg_stopsp = (unsigned)-1;
		dbg_loop_r1 = 0;
		dbg_loop_r2 = 0xFFFF;
		int_pend = false;
		int_gate = true;
		nmi_in_progress = false;
	}

	virtual ~Z80() = default;

	u32 get_idx() const
	{
		return idx;
	}

	void set_tpi(u32 Tpi)
	{
		tpi = Tpi;
	}

	void set_fast_mem_if()
	{
		mem_if = fast_mem_if;
	}

	void set_dbg_mem_if()
	{
		mem_if = dbg_mem_if;
	}

	// direct read memory in debuger
	u8 direct_rm(unsigned addr) const
	{
		return *direct_mem(addr);
	}

	// direct write memory in debuger
	void direct_wm(unsigned addr, u8 val)
	{
		*direct_mem(addr) = val;
		const u16 cache_pointer = addr & 0x1FF;
		tscache_addr[cache_pointer] = -1; // write invalidates flag
	}

	virtual u8 *direct_mem(unsigned addr) const = 0; // get direct memory pointer in debuger

	virtual u8 rd(const u32 addr)
	{
		tt += rate * 3;
		return mem_if->rm(addr);
	}

	virtual void wd(const u32 addr, const u8 val)
	{
		tt += rate * 3;
		mem_if->wm(addr, val);
	}

	virtual u8 in(unsigned port) = 0;
	virtual void out(unsigned port, u8 val) = 0;
	virtual u8 m1_cycle() = 0;			 // [vv] Не зависит от процессора (вынести в библиотеку)
	virtual u8 int_vec() = 0;			 // Функция возвращающая значение вектора прерывания для im2
	virtual void check_next_frame() = 0; // Проверка и обновления счетчика кадров и тактов внутри прерывания
	virtual void retn() = 0;			 // Вызывается в конце инструкции retn (должна сбрасывать флаг nmi_in_progress и обновлять раскладку памяти)

private:

};

constexpr auto CF = 0x01;
constexpr auto NF = 0x02;
constexpr auto PV = 0x04;
constexpr auto F3 = 0x08;
constexpr auto HF = 0x10;
constexpr auto F5 = 0x20;
constexpr auto ZF = 0x40;
constexpr auto SF = 0x80;

#define CPUTACT(a) cpu.tt += ((a)*cpu.rate)
#define TURBO(a) cpu.rate = (256 / (a))
