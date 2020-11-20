#pragma once
#include "sysdefs.h"

struct PC_KEY
{
   u8 vkey;
   u8 normal;
   u8 shifted;
   u8 padd;
};

extern const PC_KEY pc_layout[];
extern const size_t pc_layout_count;
extern const u16 dik_scan[];
