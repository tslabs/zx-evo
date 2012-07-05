#pragma once

struct PC_KEY
{
   unsigned char vkey;
   unsigned char normal;
   unsigned char shifted;
   unsigned char padd;
};

extern const PC_KEY pc_layout[];
extern const size_t pc_layout_count;
extern const unsigned short dik_scan[];
