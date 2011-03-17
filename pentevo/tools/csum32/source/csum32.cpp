#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
typedef unsigned int u32;
typedef unsigned char u8;

static u8 buf[0x1000];

int main(int argc, char *argv[])
{
    u32 cs32 = 0;

    if(argc < 2)
    {
        puts("usage: csum32 input.bin");
        return -1;
    }

    int fi = open(argv[1], O_RDONLY | O_BINARY);
    int len = filelength(fi);
    int n = len / sizeof(buf);
    int m = len % sizeof(buf);
    for(int i = 0; i < n; i++)
    {
        if(read(fi, buf, sizeof(buf)) < 0)
        {
            puts("error reading input file");
            close(fi);
            return -1;
        }
        for(int i = 0; i < sizeof(buf); i++)
            cs32 += buf[i];
    }

    if(m)
    {
        if(read(fi, buf, m) < 0)
        {
            puts("error reading input file");
            close(fi);
            return -1;
        }
        for(int i = 0; i < m; i++)
            cs32 += buf[i];
    }
    close(fi);

    int fo = open("csum32.bin", O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
    if(write(fo, &cs32, 4) < 0)
        puts("error writing output file");
    close(fo);
    return 0;
}
