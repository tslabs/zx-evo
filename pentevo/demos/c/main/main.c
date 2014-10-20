
#define TSC      // comment this to compile 6912 screen version
#define VPAGE   16

#include <pff.h>
#include <tjpgd.h>
#include <defs.h>

DIR dir;
FILINFO fil;
FATFS fs;
UINT num_read;
UINT num_write;

U8 buf[512];

#ifdef TSC
#include <tsconf.h>
#define border(a)   outp(TS_BORDER, (a))

#else
#define border(a)   output8(0xFE, (a))
#define page128(a)  output(0x7FFD, (a))

#endif

JDEC jdec;              // decompression object
BYTE jdec_buf[3100];

U8 anykey(void)
{
    U8 key, stat;
    static U8 old = 0;
    
    key = ~input(0x00FE) & 0x1F;
    stat = key && !old;
    old = key;
    return stat;
}

UINT in_func (JDEC* jd, BYTE* buff, UINT nbyte)
{
    // read data
    if (buff)
    {
        pf_read(buff, nbyte, &num_read);
        return num_read;
    }

    // skip data
    else
    {
		pf_lseek(fs.fptr + nbyte);
        return nbyte;
    }
}

#ifdef TSC
UINT out_func (JDEC *jd, void *bitmap, JRECT* rect)
{
    U8 x, y, l;
    U8 page;
    U8 *va;
    U8 *b;
    U8 p;
    
    if ((rect->right > 319) || (rect->bottom > 239))
        goto exit;
    
    page = (rect->top >> 5) | VPAGE;
    outp(TS_PAGE0, page);
    
    b = bitmap;
    l = rect->top & 0x1F;
    
    for (y = l; y < (l + 16); y++)
    {
        va = (U8*)0;
        va += y << 9;
        va += rect->left & 0x1FF;
        
        for (x = 0; x < 16; x++)
        {
            p = *b++ & 0xE0;
            p |= (*b++ & 0xE0) >> 3;
            p |= (*b++ & 0xC0) >> 6;
            *va++ = p;
        }
    }
    
    outp(TS_PAGE3, 0);

exit:
	return !anykey();    // continue to decompress or break if anykey pressed
}

void cls(void)
{
    U8 p;
    for (p = VPAGE; p < (VPAGE + 9); p++)
    {
        outp(TS_PAGE0, p);
        memset((void*)0, 0x00, 16384);
    }
}

// This func prepares palette for 256c mode, RGB = 3:3:2
ts_prepare_332(void *buf)
{
    U16 *p = buf;
    U8 r, g, b;
    U8 vd_en = inp(TS_STATUS) != 0;
    U16 pal;

    for (r = 0; r < 8; r++)
        for (g = 0; g < 8; g++)
            for (b = 0; b < 4; b++)
            {
                pal = vd_en ? ((r << 2) | (r >> 1)) : ((r & 6) << 2);
                pal <<= 5;
                pal |= vd_en ? ((g << 2) | (g >> 1)) : ((g & 6) << 2);
                pal <<= 5;
                pal |= (b << 3) | (b << 1) | (b >> 1);
                if (vd_en) pal |= PAL_VDAC_EN;
                *p++ = pal;
            }
}

#else
UINT out_func (JDEC *jd, void *bitmap, JRECT* rect)
{
	U8 y;
    U8 c;
    U8 *va;
    U8 *b;

    // check for screen limits
    if ((rect->right > 255) || (rect->bottom > 191))
        goto exit;

    b = bitmap;
    b++;
    // b++;

    #define dith(a) (*b & 0x80) ? (a) : 0; b += 3

    for (y = rect->top; y < (rect->top + 16); y++)
    {
        va = (U8*)0x4000;
        va += (y & 0xC0) << 5;
        va += (y & 0x38) << 2;
        va += (y & 7) << 8;
        va += rect->left >> 3;

        c  = dith(0x80);
        c |= dith(0x40);
        c |= dith(0x20);
        c |= dith(0x10);
        c |= dith(0x08);
        c |= dith(0x04);
        c |= dith(0x02);
        c |= dith(0x01);
        *va++ = c;

        c  = dith(0x80);
        c |= dith(0x40);
        c |= dith(0x20);
        c |= dith(0x10);
        c |= dith(0x08);
        c |= dith(0x04);
        c |= dith(0x02);
        c |= dith(0x01);
        *va = c;
    }

exit:
	return !anykey();    // continue to decompress or break if anykey pressed
}

void cls(void)
{
    memset((void*)0xC000, 0x00, 6144);
    memset((void*)0xD800, 0x47, 768);
}

#endif

int strsfx(char const *str, char const* sfx)
{
    U8 lstr, lsfx;
    lstr = strlen(str);
    lsfx = strlen(sfx);
    return strcmp(str + lstr - lsfx, sfx) == 0;
}

// -- Main --
void main (void)
{
    U8 screen;
    U16 i;

    disable_interrupt();
    border(0);
    
    #ifdef TSC
        ts_prepare_332(buf);
        ts_load_pal(buf, 512);
        outp(TS_VCONFIG, VID_320X240 | VID_256C);
        outp(TS_VPAGE, VPAGE);
        outp(TS_MEMCONFIG, MEM_W0WE | MEM_W0MAP_N | MEM_W0RAM);
    #endif

    disk_initialize();
	pf_mount(&fs);
    
restart:
    pf_opendir(&dir, "");

    while (1)
    {
        if ((pf_readdir(&dir, &fil) != FR_OK) || (fil.fname[0] == 0)) goto restart;        // all again
        if (fil.fattrib & AM_DIR) continue;         // not a file
        if (!strsfx(fil.fname, ".JPG")) continue;   // not a JPEG
    
    #ifdef TSC
        cls();

    #else
        page128(0x15); cls();
        page128(0x17); cls();

    #endif
        
        pf_open(fil.fname);
        jd_prepare(&jdec, in_func, jdec_buf, sizeof(jdec_buf), 0);
        jd_decomp(&jdec, out_func, 0);
        
        while (!anykey());
    }
}
