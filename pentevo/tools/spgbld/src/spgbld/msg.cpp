
#include "stdafx.h"
#include "defs.h"
#include "msg.h"

void print_help()
{
	printf(STR(SB_VER));
	printf("\n\n");
	printf("Build:	  spgbld.exe -b <input.ini> <output.spg> [-c <pack method>]\n");
	//printf("Info:	  spgbld.exe -i <input.spg>\n");
	printf("Unpack:	  spgbld.exe -u <input.spg>\n");
	//printf("Re-pack:  spgbld.exe -r <input.spg> <output.spg> [-c <pack method>]\n");
	printf("\n");
}

#define	ERR(a, b)	case (a): \
			printf(STR(b)); \
			printf("\n\n"); \
			exit(i);

void error(int i)
{
	switch(i)
	{
		ERR (RC_OK, ER_OK)
		ERR (RC_ARG, ER_ARG)
		ERR (RC_INI, ER_INI)
		ERR (RC_UNK, ER_UNK)
		ERR (RC_ALGN, ER_ALGN)
		ERR (RC_FILE, ER_FILE)
		ERR (RC_PACK, ER_PACK)
		ERR (RC_0BLK, ER_0BLK)
		ERR (RC_BIG, ER_BIG)
		ERR (RC_ZERO, ER_ZERO)
		ERR (RC_MHMT, ER_MHMT)
		ERR (RC_VER, ER_VER)
	}
}