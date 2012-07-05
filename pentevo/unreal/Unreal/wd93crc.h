#pragma once

unsigned wd93_crc(unsigned char *ptr, unsigned size);
unsigned short crc16(unsigned char *buf, unsigned size);
void crc32(int &crc, unsigned char *buf, unsigned len);
