#include "stdafx.h"
#include "defs.h"
#include "func.h"
#include "msg.h"

extern HDR hdr;
extern BLK blk[256];
extern CONF conf;

int parse_arg(int argc, _TCHAR* argv[], _TCHAR* arg, int n)
{
	for (int i=1; i<argc; i++)
		if (!wcscmp(argv[i], arg) && (argc-1) >= (i+n))
			return i+1;
	return 0;
}

void parse_args(int argc, _TCHAR* argv[])
{
	int i;
	conf.packer = PM_AUTO;
	
	if (i = parse_arg(argc, argv, L"-b", 2))
	{
		conf.mode = M_BLD;
		conf.in_fname = argv[i];
		conf.out_fname = argv[i+1];
		if (i = parse_arg(argc, argv, L"-c", 1))
			conf.packer = (C_PACK)_wtoi(argv[i]);
		return;
	}
	
	if (i = parse_arg(argc, argv, L"-u", 1))
	{
		conf.mode = M_UNP;
		conf.in_fname = argv[i];
		return;
	}
	
	print_help();
	error (RC_ARG);
}

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
	int a = 0;
	int b = 0;
	conf.n_blocks = 0;
	
	FILE* f = _wfopen(name, L"rt");
	if (!f)
		error(RC_INI);
		
	while (!feof(f))
	{
		*t = 0;
		fscanf(f, "%256[^ =\n\r\t]%*[ =]%256[^\n\r\t]%*c", t, v);
		if (!*t)
		{
			fscanf(f, "%*c");
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
		
		// block
		if (!strcmp(t, STR(F_BLK)))
		{
			sscanf(v, "%d, %d, %s", &a, &b, v);
			if (a & 0x1FF)
				error(RC_ALGN);
			if ((a < 49152) || (a > 65024))
				error(RC_ADDR);
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

void rand_name(char* name)
{
	for (int i=0; i<11; i++)
	{
		name[i] = 65 + (rand() & 15);
	}
	name[11] ='.', name[12] ='t', name[13] ='m', name[14] ='p', name[15] = 0;
}

void store_block(int i, char* fn, int s)
{
	memset(blk[i].data, 0, sizeof(blk[i].data));
	blk[i].size = s;
	hdr.blk[i].size = sz(blk[i].size);
	FILE* f = fopen(fn, "rb");
	fread(blk[i].data, 1, s, f);
	fclose(f);
}

void pack_blocks()
{
	struct stat st;
	int s1, s2, sz1, sz2;
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
		sz1 = sz2 = 31;
		
		if (conf.packer == PM_AUTO || conf.packer == PM_MLZ)
		{
			if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-mlz", f1n, f2n, NULL) < 0)
				error(RC_MHMT);
			stat(f2n, &st); s1 = st.st_size; sz1 = sz(st.st_size);
		}
		
		if (conf.packer == PM_AUTO || conf.packer == PM_HST)
		{
			if (_spawnlp(_P_WAIT, "mhmt.exe", "dummy", "-hst", f1n, f3n, NULL) < 0)
				error(RC_MHMT);
			stat(f3n, &st); s2 = st.st_size; sz2 = sz(st.st_size);
		}
		
		remove(f1n);
		
		if (sz(blk[i].size) <= min(sz1, sz2))
			p = 0;
		else
			p = (sz1 < sz2) ? PM_MLZ : PM_HST;		// HRUST priority (preferably)
			// p = (sz1 <= sz2) ? PM_MLZ : PM_HST;	// MLZ priority
		
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

void load_spg(_TCHAR* name)
{
}

void save_spg(_TCHAR* name)
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

void load_files()
{
	struct stat st;
	
	for (int i=0; i<conf.n_blocks; i++)
	{
		stat(blk[i].fname, &st);
		
		if (st.st_size < 0)
		{
			printf("%s: ", blk[i].fname);
			error(RC_FILE);
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
		hdr.blk[i].size = sz(blk[i].size);
		fread(blk[i].data, 1, blk[i].size, f);
		fclose(f);
	}
}

void save_files()
{
}

void save_ini(_TCHAR *name)
{
}
