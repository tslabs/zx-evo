#pragma once

typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned long u32;
typedef signed long i32;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned char u8;
typedef signed char i8;

#define min(a,b)	(((a) < (b)) ? (a) : (b))
#define sz(a)		(((a) - 1) >> 9)

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

// .spg defaults
#define SPG_MAGIC	SpectrumProg
#define SPG_VER		1
#define SPG_SUBVER	0
#define SPG_PACK	1
#define SPG_RES		0x5B00
#define SPG_SP		0x5C00
#define SPG_PAGE3	0
#define SPG_CLK		0
#define SPG_INT		0
#define SPG_PGR		0

// .ini fields
#define F_DESC 	desc
#define F_START start
#define F_SP	stack
#define F_PAGE3 page3
#define F_CLOCK clock
#define F_INT 	int
#define F_PAGER pager
#define F_RES	resident
#define F_DAY 	day
#define F_MONTH month
#define F_YEAR 	year
#define F_BLK  	block

enum C_MOD {
	M_INFO = 0,
	M_BLD,
	M_CONV,
	M_UNP
};

enum C_PACK {
	PM_AUTO = -1,
	PM_NONE,
	PM_MLZ,
	PM_HST
};

struct CONF {
	C_PACK packer;
	C_MOD mode;
	_TCHAR* in_fname;
	_TCHAR* out_fname;
	int n_blocks;
};

#pragma pack(push)
#pragma pack(1)
struct HDR {
	char desc[32];
	char magic[12];
	struct {
		u8 subver:4;
		u8 ver:4;
	};
	u8 day;
	u8 month;
	u8 year;
	u16 start;
	u16 sp;
	u8 page3;
	struct {
		u8 clk:2;
		u8 ei:1;
	};
	u16 pager;
	u16 resid;
	u16 num_blk;
	u8 second;
	u8 minute;
	u8 hour;
	u8 pad_0[17];
	char creator[32];
	u8 pad_1[144];
	struct {
		struct {
			u8 addr:5;
			u8 pad_1:2;
			u8 last:1;
			u8 size:5;
			u8 pad_2:1;
			u8 comp:2;
		};
		u8 page;
	} blk[256];
};
#pragma pack(pop)

struct BLK {
	char fname[128];
	int offset;
	int size;
	u8 data[16384];
};
