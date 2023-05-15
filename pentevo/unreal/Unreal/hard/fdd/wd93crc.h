#pragma once
#include "sysdefs.h"

unsigned wd93_crc(u8 *ptr, unsigned size);
u16 crc16(u8 *buf, unsigned size);
void crc32(int &crc, u8 *buf, unsigned len);
