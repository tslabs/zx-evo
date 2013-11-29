
#include "stdafx.h"
#include "defs.h"
#include "msg.h"

void print_help()
{
	printf(STR(SB_VER));
	printf("\n\n");
	printf("Build:	  spgbld.exe -b <input.ini> <output.spg> [-c <pack method>]\n");
	//printf("Info:	  spgbld.exe -i <input.spg>\n");
	//printf("Unpack:	  spgbld.exe -u <input.spg>\n");
	//printf("Re-pack:  spgbld.exe -r <input.spg> <output.spg> [-c <pack method>]\n");
	printf("\n");
}

#define	ERR(a)	case (RC_##a): \
			printf(STR(ER_##a)); \
			printf("\n\n"); \
			exit(i);

void error(int i)
{
	switch(i)
	{
		ERR (OK)
		ERR (ARG)
		ERR (INI)
		ERR (UNK)
		ERR (ALGN)
		ERR (FILE)
		ERR (PACK)
		ERR (0BLK)
		ERR (BIG)
		ERR (ZERO)
		ERR (MHMT)
		ERR (VER)
		ERR (ADDR)
	}
}