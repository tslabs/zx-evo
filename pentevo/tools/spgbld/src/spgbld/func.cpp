#include "stdafx.h"
#include "defs.h"
#include "func.h"
#include "msg.h"

extern HDR hdr;
extern BLK blk[256];
extern CONF conf;

void init_hdr()
{
	memset(&hdr, 0, sizeof(hdr));
	strncpy(hdr.magic, STR(SPG_MAGIC), sizeof(hdr.magic));
	strncpy(hdr.creator, STR(SB_CRT), sizeof(hdr.creator));
	hdr.ver = SPG_VER;
	hdr.subver = SPG_SUBVER;
	hdr.sp = SPG_SP;
	hdr.page3 = SPG_PAGE3;
	hdr.clk = SPG_CLK;
	hdr.ei = SPG_INT;
	hdr.pager = SPG_PGR;
	hdr.resid = SPG_RES;
	time_t t1 = time(NULL);
	tm* t2 = localtime(&t1);
	hdr.day = t2->tm_mday;
	hdr.month = t2->tm_mon + 1;
	hdr.year = t2->tm_year - 100;
	hdr.second = t2->tm_sec;
	hdr.minute = t2->tm_min;
	hdr.hour = t2->tm_hour;
}

void load_ini(_TCHAR *name)
{
	char t[256];
	char v[256];
	char e[16];
	int a = 0;
	int b = 0;
	conf.n_blocks = 0;
	
	FILE* f = _wfopen(name, L"rt");
	if (!f)
		error(RC_NOINI);
		
	while (!feof(f))
	{
		*t = 0;
		fscanf(f, "%256[^ =\n\r\t]%[ =]%256[^\n\r\t]%c", t, e, v, e);
		if (!*t)
		{
			fscanf(f, "%c", e);
			continue;
		}

		strlwr(t);
		
		// description string
		if (!strcmp(t, STR(F_DESC)))
		{
			strncpy(hdr.desc, v, 32);
			continue;
		}
		
		// stack address
		if (!strcmp(t, STR(F_SP)))
		{
			sscanf(v, "%d", &a);
			hdr.sp = a;
			continue;
		}
		
		// resident address
		if (!strcmp(t, STR(F_RES)))
		{
			sscanf(v, "%d", &a);
			hdr.resid = a;
			continue;
		}
		
		// pager address
		if (!strcmp(t, STR(F_PAGER)))
		{
			sscanf(v, "%d", &a);
			hdr.pager = a;
			continue;
		}
		
		// start address
		if (!strcmp(t, STR(F_START)))
		{
			sscanf(v, "%d", &a);
			hdr.start = a;
			continue;
		}
		
		// page3
		if (!strcmp(t, STR(F_PAGE3)))
		{
			sscanf(v, "%d", &a);
			hdr.page3 = a;
			continue;
		}
		
		// clock
		if (!strcmp(t, STR(F_CLOCK)))
		{
			sscanf(v, "%d", &a);
			hdr.clk = a;
			continue;
		}
		
		// INT
		if (!strcmp(t, STR(F_INT)))
		{
			sscanf(v, "%d", &a);
			hdr.ei = a;
			continue;
		}
		
		// packer
		if (!strcmp(t, STR(F_COMP)))
		{
			sscanf(v, "%d", &a);
			if (a < -1 || a > 2)
				error(RC_PACK);
			conf.packer = a;
			continue;
		}
		
		// block
		if (!strcmp(t, STR(F_BLK)))
		{
			sscanf(v, "%d, %d, %s", &a, &b, v);
			if (a & 0x1FF)
				error(RC_ALGN);
			hdr.blk[conf.n_blocks].addr = (a - 49152) >> 9;
			hdr.blk[conf.n_blocks].page = b;
			strncpy(blk[conf.n_blocks].fname, v, sizeof(blk[conf.n_blocks].fname));
			conf.n_blocks++;
			
			if (conf.n_blocks == 256)
				break;
			continue;
		}
		
		// unknown tag
		printf("%s: ", t);
		error(RC_UNK);
	}
	
	if (!conf.n_blocks)
		error(RC_0BLK);
	
	hdr.num_blk = conf.n_blocks;
	hdr.blk[conf.n_blocks - 1].last = 1;

	fclose(f);
}

void load_files()
{
	struct stat st;
	
	for (int i=0; i<conf.n_blocks; i++)
	{
		stat(blk[i].fname, &st);
		
		if (st.st_size < 0)
		{
			printf("%s: ", blk[i].fname);
			error(RC_NOFILE);
		}

		if (st.st_size > 16384)
		{
			printf("%s: ", blk[i].fname);
			error(RC_BIG);
		}
		
		if (st.st_size == 0)
		{
			printf("%s: ", blk[i].fname);
			error(RC_ZERO);
		}
		
		FILE* f = fopen(blk[i].fname, "rb");
		blk[i].size = st.st_size;
		hdr.blk[i].size = (blk[i].size - 1) >> 9;
		fread(blk[i].data, 1, blk[i].size, f);
		fclose(f);
	}
}

void store_block(int i, char* fn, int s)
{
	memset(blk[i].data, 0, sizeof(blk[i].data));
	blk[i].size = s;
	hdr.blk[i].size = (blk[i].size - 1) >> 9;
	FILE* f = fopen(fn, "rb");
	fread(blk[i].data, 1, s, f);
	fclose(f);
}

void pack_blocks()
{
	struct stat st;
	int s1, s2;
	int p;
	
	for (int i=0; i<conf.n_blocks; i++)
	{
		char f1n[16], f2n[16], f3n[16];
		rand_name(f1n); rand_name(f2n); rand_name(f3n);
		
		// add errors check here!!!
		FILE* f1 = fopen(f1n, "wb");
		fwrite(blk[i].data, 1, blk[i].size, f1);
		fclose(f1);
		
		s1 = s2 = 16384;
		
		if (conf.packer == -1 || conf.packer == 1)
		{
			_spawnl(_P_WAIT, "mhmt.exe", "_", "-mlz", f1n, f2n, NULL);
			stat(f2n, &st); s1 = st.st_size;
		}
		
		if (conf.packer == -1 || conf.packer == 2)
		{
			_spawnl(_P_WAIT, "mhmt.exe", "_", "-hst", f1n, f3n, NULL);
			stat(f3n, &st); s2 = st.st_size;
		}
		
		remove(f1n);
		
		if (blk[i].size <= min(s1, s2))
			p = 0;
		else p = (s1 < s2) ? 1 : 2;
		
		hdr.blk[i].comp = p;

		switch (p)
		{
			case 1:
			{
				store_block(i, f2n, s1);
				break;
			}
			case 2:
			{
				store_block(i, f3n, s2);
				break;
			}
		}
		
		remove(f2n); remove(f3n);
	}
}

void save_out(_TCHAR* name)
{
	FILE* f = _wfopen(name, L"wb");
	
	// save header
	fwrite(&hdr, 1, sizeof(hdr), f);
	
	// save blocks
	for (int i=0; i<conf.n_blocks; i++)
	{
		fwrite(&blk[i].data, 1, (hdr.blk[i].size + 1) << 9, f);
	}
	
	fclose(f);
}

void rand_name(char* name)
{
	for (int i=0; i<11; i++)
	{
		name[i] = 65 + (rand() & 15);
	}
	name[11] ='.', name[12] ='t', name[13] ='m', name[14] ='p', name[15] = 0;
}
