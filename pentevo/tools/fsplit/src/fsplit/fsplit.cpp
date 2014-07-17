// fsplit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define MAX_SPLIT 100

int _tmain(int argc, _TCHAR* argv[])
{
	if ((argc < 3) || (argc > (MAX_SPLIT + 2)))
	{
		printf("File splitter by TS-Labs\n");
		printf("Usage: fsplit.exe <input> <piece 1 size> .. <piece N size>\n");
		return 1;
	}

	FILE* f_in = _wfopen (argv[1], L"rb");
	if (!f_in)
	{
		printf ("Input file error!");
		return 1;
	}

    struct _stat st;
    _wstat(argv[1], &st);
    int sz = st.st_size;
    void *buf = malloc(sz);
    if (!buf)
    {
        printf ("Memory alloc error!");
        return 3;
    }

    int ssz[MAX_SPLIT];
    int n = (argc - 2);
    for (int i = 0; i < n; i++)
    {
        ssz[i] = min(sz, _wtoi(argv[i + 2]));
        sz -= ssz[i];
        if (!sz)
            break;
    }
        
    if (sz)
        ssz[n++] = sz;

    for (int i = 0; i < n; i++)
	{
        _TCHAR fout[256];
        _TCHAR t[256];
        wcscpy(fout, argv[1]);
        wcscat(fout, L".");
        wcscat(fout, _itow(i, t, 10));

        FILE *f_out = _wfopen(fout, L"wb");
        if (!f_out)
        {
            printf ("Output file error!");
            return 2;
        }

        if (fread(buf, 1, ssz[i], f_in) != ssz[i])
        {
            printf ("Read file error!");
            return 4;
        }

        if (fwrite(buf, 1, ssz[i], f_out) != ssz[i])
        {
            printf ("Write file error!");
            return 5;
        }

        fclose(f_out);
    }

    fclose(f_in);

    printf("DONE!\n");

	return 0;
}

