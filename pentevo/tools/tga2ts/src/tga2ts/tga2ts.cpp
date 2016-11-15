// tga2ts.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma pack (push, 1)

typedef struct
{
    U8 id_length;
    U8 color_map_type;
    U8 image_type;

  /* Color Map Specification.  */
    U16 color_map_first_index;
    U16 color_map_length;
    U8 color_map_bpp;

  /* Image Specification.  */
    U16 image_x_origin;
    U16 image_y_origin;
    U16 image_width;
    U16 image_height;
    U8 image_bpp;
    U8 image_descriptor;
} TGA_HEADER;

#pragma pack (pop)

enum
{
    TGA_IMAGE_TYPE_NONE = 0,
    TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR = 1,
    TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR = 2,
    TGA_IMAGE_TYPE_UNCOMPRESSED_BLACK_AND_WHITE = 3,
    TGA_IMAGE_TYPE_RLE_INDEXCOLOR = 9,
    TGA_IMAGE_TYPE_RLE_TRUECOLOR = 10,
    TGA_IMAGE_TYPE_RLE_BLACK_AND_WHITE = 11,
};

enum
{
    TGA_COLOR_MAP_TYPE_NONE = 0,
    TGA_COLOR_MAP_TYPE_INCLUDED = 1
};

enum
{
    TGA_IMAGE_ORIGIN_RIGHT = 0x10,
    TGA_IMAGE_ORIGIN_TOP   = 0x20
};

// default levels.map
U8 levels[25] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140,
                 150, 160, 170, 180, 190, 200, 210, 220, 230, 240};
U8 temp[8];

BOOL shift16c = FALSE;
BOOL vdac_en = FALSE;
BOOL flip    = TRUE;
BOOL align   = TRUE;

U32 first_index = 0;

U8 conv_levels(U8 lvl_in)
{
    int i;

    if (vdac_en)
        i = lvl_in >> 3;
    
    else
    {
        for (i = 24; i >= 0; i--)
        {
            if (lvl_in >= levels[i])
                break;
        }
    }

	return i;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TGA_HEADER header;
	FILE *f_tga = NULL;
	FILE *f_map = NULL;
	FILE *f_pal = NULL;
    FILE *f_pal4 = NULL;
	FILE *f_btm = NULL;
	FILE *f_btm4 = NULL;

    printf("TGA to TS converter by TS-Labs\n");
    printf("remixed by wbcbz7 zz.oq.zolb\n");
    printf("------------\n");

    if (argc < 2)
    {
        printf("Usage: tga2ts.exe <input>.tga [-v] [-a]\n");
        printf("-v    - enable VDAC palette\n");
        printf("-a    - do not align picture to 16bit boundary\n");
        printf("-s    - shift color indices in bitmap for 16c if palette starts from >0 offset\n"); 
        printf("-i[x] - start palette from [x] index for 16c (includes -s)\n"); 
        return 1;
    }

    for (int i = 2; i < argc; i++)
    {
        static _TCHAR arg[256];
        wcscpy(arg, argv[i]);
        _wcsupr(arg);

        if ((wcsstr(arg, L"V"))) 
        {
            vdac_en = TRUE; continue; 
        }

        if ((wcsstr(arg, L"A"))) 
        {
            align   = FALSE; continue;
        }

        if ((wcsstr(arg, L"S"))) 
        {
            shift16c  = TRUE; continue;
        }

        if (wcsstr(arg, L"I"))
        {
            if (!swscanf(wcsstr(arg, L"I") + wcslen(L"I"), L"%d", &first_index)) first_index = 0;
            shift16c  = TRUE; continue;
        }
    }

    if (vdac_en) goto no_levels_map;

// loading levels map
	if (!(f_map = _wfopen(L"levels.map", L"r")))
		goto no_levels_map;

    for (int i = 0; i < 25; i++)
    {
        if (fscanf(f_map, "%uhh\n", temp) != 1)
            goto fatal;

		levels[i] = temp[0];
    }

	fclose(f_map);

no_levels_map:

// parse TGA file header
	if (!(f_tga = _wfopen(argv[1], L"rb")))
		goto fatal;

	if (fread(&header, 1, sizeof (header), f_tga) != sizeof (header))
		goto fatal;

    if (fseek (f_tga, header.id_length, SEEK_CUR))
		goto fatal;

    if (header.image_type != TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR)
    {
        printf("Image is of a wrong type! Only uncompressed indexed supported.\n");
        goto fatal2;
    }

    if (header.image_bpp != 8)
    {
        printf("Image is of a wrong color resolution! Only 8 bit supported.\n");
        goto fatal2;
    }

    if (header.color_map_type != TGA_COLOR_MAP_TYPE_INCLUDED)
    {
        printf("Image has no color map!\n");
        goto fatal2;
    }

    if (header.color_map_bpp != 24)
    {
        printf("Image is of a wrong color map resolution! Only 24 bit supported.\n");
        goto fatal2;
    }
    
    if (header.image_descriptor & 0x20)
    {
        flip = FALSE;
    }
    
    printf("Source picture -> %dx%d, %d color %s\n", header.image_width, header.image_height, header.color_map_length, (vdac_en ? "(VDAC)" : "")); 

    /*
    if (first_index != 0) {
        header.color_map_first_index = (U16)first_index;
        fseek(f_tga, header.color_map_first_index * (header.color_map_bpp / 8), SEEK_CUR);
    }
    */

// convert color map to 0RRrrrGG gggBBbbb format
	static _TCHAR fpal[256];
	wcscpy(fpal, argv[1]);
	wcscat(fpal, L".pal");

    static _TCHAR fpal4[256];
	wcscpy(fpal4, argv[1]);
	wcscat(fpal4, L".pal4");

	if (!(f_pal = _wfopen(fpal, L"wb")))
		goto fatal;

    if (!(f_pal4 = _wfopen(fpal4, L"wb")))
		goto fatal;

    U32 j = 0; 
    for (int i = header.color_map_first_index; i < header.color_map_length; i++, j++)
    {
        static U16 cmap_ts;

        if (fread(temp, 1, header.color_map_bpp / 8, f_tga) != (header.color_map_bpp / 8))
            goto fatal;

        cmap_ts = conv_levels(temp[0]);
        cmap_ts |= conv_levels(temp[1]) << 5;
        cmap_ts |= conv_levels(temp[2]) << 10;
        if (vdac_en) cmap_ts |= 0x8000; // wbcbz7 note: сам не пофиксишь - никто не пофиксит :)

        fwrite(&cmap_ts, 1, sizeof(cmap_ts), f_pal);
        if ((j >= first_index) && (j < (first_index + 16))) fwrite(&cmap_ts, 1, sizeof(cmap_ts), f_pal4);
    }

    fclose(f_pal);
    fclose(f_pal4);

// extract bitmap
	static _TCHAR fbtm[256];
	static _TCHAR fbtm4[256];
    static U8 buf[65536];
    static U8 buf4[65536];
    U32 indexshift = (shift16c ? header.color_map_first_index : 0); // color map index shift for 16c images
    
    memset(buf,  0, 65536);
    memset(buf4, 0, 65536);

	wcscpy(fbtm, argv[1]);
	wcscat(fbtm, L".pix");
	wcscpy(fbtm4, argv[1]);
	wcscat(fbtm4, L".pix4");

	if (!(f_btm = _wfopen(fbtm, L"wb")))
		goto fatal;

	if (!(f_btm4 = _wfopen(fbtm4, L"wb")))
		goto fatal;

    int pitch_256c = (align ?  (header.image_width + 1) & ~1      : header.image_width); // da fix!
    int pitch_16c  = (align ? ((header.image_width + 3) & ~3) / 2 : header.image_width / 2); // da fix!

    if (flip) fseek(f_tga, (header.image_height-1)*header.image_width, SEEK_CUR);

    for (int i = 0; i < header.image_height; i++)
    {
        int p = 0;

        if (fread(buf, 1, header.image_width, f_tga) != header.image_width)
            goto fatal;

        if (flip) fseek(f_tga, -2*header.image_width, SEEK_CUR);

        if (fwrite(buf, 1, pitch_256c, f_btm) != pitch_256c)
            goto fatal;

        for (int j = 0; j < header.image_width;)
        {
            U8 c = ((buf[j++] - indexshift) & 15) << 4;
            c |= (buf[j++] - indexshift) & 15;
            buf4[p++] = c;
        }

        if (fwrite(buf4, 1, pitch_16c, f_btm4) != pitch_16c)
            goto fatal;
    }

    fclose(f_btm);

    printf("Output picture -> %dx%d, 256c_pitch = %d, 16c_pitch = %d\n", header.image_width, header.image_height, pitch_256c, pitch_16c);

	return 0;

fatal:
	printf("File error!\n");

fatal2:
	if (f_tga)
		fclose(f_tga);

	return 1;
}

