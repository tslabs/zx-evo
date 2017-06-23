
#include "stdafx.h"

void print_help()
{
	printf(STR(SB_VER));
	printf("\n\n");
	printf("Build:	  spgbld.exe -b <input.ini> <output.spg> [-c <pack method>] [-v]\n");
	printf("Unpack:	  spgbld.exe -u <input.spg>\n");
	//printf("Info:	  spgbld.exe -i <input.spg>\n");
	//printf("Re-build:  spgbld.exe -r <input.spg> <output.spg> [-c <pack method>]\n");
	printf("\n");
}
