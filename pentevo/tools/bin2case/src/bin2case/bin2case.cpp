// bin2case.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{

	if (argc != 3)
	{
		printf("Binary to Verilog CASE ROM converter by TS-Labs\n");
		printf("Usage: bin2case.exe <input> <output>\n");
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

    fprintf(f_out,"module rom\n");
    fprintf(f_out,"(\n");
    fprintf(f_out,"    input wire [15:0] addr,\n");
    fprintf(f_out,"    output logic [7:0] data\n");
    fprintf(f_out,");   \n\n");
    fprintf(f_out,"    always_comb\n");
    fprintf(f_out,"        case (addr)\n");

    static unsigned char c;
    int i = 0;

	while (fread(&c, 1, 1, f_in))
	{

        fprintf(f_out,"            16'h%04X:   data = 8'h%02X;\n", i, c);
        i++;
	}

    fprintf(f_out,"            default:    data = 8'hFF;\n");
    fprintf(f_out,"        endcase\n\n");
    fprintf(f_out,"endmodule\n\n");

    fclose(f_in);
    fclose(f_out);

    printf("DONE!\n");

	return 0;
}

