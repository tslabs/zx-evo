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

U8 levels[25];
U8 temp[8];

U8 conv_levels(U8 lvl_in)
{
    int i;

    for (i = 24; i >= 0; i--)
	{
        if (lvl_in >= levels[i])
            break;
	}

	return i;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TGA_HEADER header;
	FILE *f_tga;
	FILE *f_map;
	FILE *f_pal;
	FILE *f_btm;

// loading levels map
	if (!(f_map = _wfopen(L"levels.map", L"r")))
		goto fatal;

    for (int i = 0; i < 25; i++)
    {
        if (fscanf(f_map, "%uhh\n", temp) != 1)
            goto fatal;

		levels[i] = temp[0];
    }

	fclose(f_map);

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

// convert color map to 0RRrrrGG gggBBbbb format
	static _TCHAR fpal[256];
	wcscpy(fpal, argv[1]);
	wcscat(fpal, L".pal");

	if (!(f_pal = _wfopen(fpal, L"wb")))
		goto fatal;

    for (int i = header.color_map_first_index; i < header.color_map_length; i++)
    {
        static U16 cmap_ts;

        if (fread(temp, 1, header.color_map_bpp / 8, f_tga) != (header.color_map_bpp / 8))
            goto fatal;

        cmap_ts = conv_levels(temp[0]);
        cmap_ts |= conv_levels(temp[1]) << 5;
        cmap_ts |= conv_levels(temp[2]) << 10;

        fwrite(&cmap_ts, 1, sizeof(cmap_ts), f_pal);
    }

    fclose(f_pal);

// extract bitmap
	static _TCHAR fbtm[256];
    static U8 buf[65536];

	wcscpy(fbtm, argv[1]);
	wcscat(fbtm, L".pix");

	if (!(f_btm = _wfopen(fbtm, L"wb")))
		goto fatal;

// to do: add checker for image origin top/bottom

    for (int i = 0; i < header.image_height; i++)
    {
        if (fread(buf, 1, header.image_width, f_tga) != header.image_width)
            goto fatal;

        if (fwrite(buf, 1, header.image_width, f_btm) != header.image_width)
            goto fatal;
    }

    fclose(f_btm);

	return 0;

fatal:
	printf("File error!\n");

fatal2:
	if (f_tga)
		fclose(f_tga);

	return 1;
}

