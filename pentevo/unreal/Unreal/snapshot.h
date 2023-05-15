#pragma once
#include "sysdefs.h"

#pragma pack(1)

struct blkSPG1_0{
	u8 addr;
	u8 size;
	u8 page;
};

struct hdrSPG1_0 {
	u8 author[32];
	u8 sign[12];	// These are common for versions 1.0 and 0.2
	u8 ver;			//
	u8 day;
	u8 month;
	u8 year;
	u16 pc;
	u16 sp;
	u8 win3_pg;
	u8 clk;
	u16 pgmgr_addr;
	u16 rsd_addr;
	u16 n_blk;
	u8 second;
	u8 minute;
	u8 hour;
	u8 res1[17];
	u8 creator[32];
	u8 res2[144];
	blkSPG1_0 blocks[256];
	u8 data;
};

struct blkSPG0_2{
	u16 addr;
	u8 size;
	u8 page;
};

struct hdrSPG0_2 {
	u8 res1[32];
	u8 sign[12];	// These are common for versions 1.0 and 0.2
	u8 ver;			//
	u16 crc;
	u16 crc_inv;
	u16 port_xi_addr;
	u8 port_xi_val;
	u8 res2[64-52];
	u16 pc;
	u8 win3_pg;
	u8 flag;
	u16 pgmgr_addr;
	u8 day;
	u8 month;
	u8 year;
	u8 assy_ver;
	u16 sp;
	u16 vars_addr;
	u16 vars_len;
	u8 res3[128-80];
	blkSPG0_2 blocks[16];
	u8 vars[320];
	u8 data;
};

struct hdrSNA128 {
   u8 i;
   u16 althl, altde, altbc, altaf;
   u16 hl, de, bc, iy, ix;
   u8 iff1; /* 00 - reset, FF - set */
   u8 r;
   u16 af, sp;
   u8 im,pFE;
   u8 page5[PAGE]; // 4000-7FFF
   u8 page2[PAGE]; // 8000-BFFF
   u8 active_page[PAGE]; // C000-FFFF
   /* 128k extension */
   u16 pc;
   u8 p7FFD;
   u8 trdos;
//   u8 pages[PAGE]; // all pages, except already saved
                                // (m.b. 5 or 6 pages)
};

struct hdrZ80
{
   u8 a,f;
   u16 bc,hl,pc,sp;
   u8 i,r,flags;
   u16 de,bc1,de1,hl1;
   u8 a1,f1;
   u16 iy,ix;
   u8 iff1, iff2, im;
   /* 2.01 extension */
   u16 len, newpc;
   u8 model, p7FFD;
   u8 r1,r2, p7FFD_1;
   u8 AY[16];
   /* 3.0 extension */
   u16 LowT;
   u8 HighT;
   u8 ReservedFlag;
   u8 MgtRom;
   u8 MultifaceRom;
   u8 RamRom0; // 0000-1FFF ram/rom
   u8 RamRom1; // 2000-3FFF ram/rom
   u8 KbMap1[10];
   u8 KbMap2[10];
   u8 MgtType;
   u8 Disciple1;
   u8 Disciple2;
   u8 p1FFD;
};

struct hdrSP
{
   u16 sig;
   u16 len;
   u16 start;
   u16 bc,de,hl,af;
   u16 ix,iy;
   u16 altbc,altde,althl,altaf;
   u8 r, i;
   u16 sp, pc;
   u16 reserved;
   u8 pFE;
   u8 reserved2;
   u16 flags;
};

#pragma pack()

typedef void (*TVideoSaver)();

extern TVideoSaver VideoSaver;

int loadsnap(char *filename);
int write_sna(FILE *ff);
void opensnap(int index);
void savesnap(int diskindex);
void main_scrshot();
void main_scrshot_clipboard();
//void main_savevideo();

bool gdiplus_startup();
void gdiplus_shutdown();
