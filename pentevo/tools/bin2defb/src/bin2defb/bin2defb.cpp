// bin2defb.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	unsigned char c;

	if (argc != 3)
	{
		printf("Binary to DEFB converter by TS-Labs\n");
		printf("Usage: bin2defb.exe <input binary> <output assembly>\n");
		return 1;
	}

	FILE* f_in = _wfopen (argv[1], L"rb");
	if (!f_in)
	{
		printf ("Input filename error!");
		return 1;
	}

	FILE* f_out = _wfopen (argv[2], L"wt");
	if (!f_out)
	{
		printf ("Output filename error!");
		return 1;
	}

	wprintf(L"Converting %s to %s ... ", argv[1], argv[2]);

	int i = 0;
	
	while (!feof(f_in), fread(&c, 1, 1, f_in))
	{
		if (!(i % 16))
			fprintf(f_out,"\tdefb 0x%02x", c);
		else if ((i % 16) != 15)
			fprintf(f_out,", 0x%02x", c);
		else
			fprintf(f_out,", 0x%02x\n", c);
		i++;
	}

	printf("DONE!\n");

	return 0;
}

