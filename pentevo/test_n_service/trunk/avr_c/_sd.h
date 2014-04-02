#ifndef _SD_H
#define _SD_H 1

#define SDV1FLAG 0b00000001
#define SDV2FLAG 0b00000010
#define SDHCFLAG 0b00000100
#define MMCFLAG  0b00010000
#define BIT_SDHCFLAG 2
// SDSC=SDV2FLAG
// SDHC=SDV2FLAG|SDHCFLAG

#ifndef __ASSEMBLER__

#include "_types.h"

extern volatile u8 sd_cardtype;
extern const u8 cmd08[] PROGMEM;

u8 sd_receive(void);
u8 sd_exchange(u8 data);
void sd_rd_dummy(u8 count);
u8 sd_cmd_without_arg(u8 cmd);
u8 sd_cmd_with_1arg(u8 cmd, u16 h_arg);
u8 sd_cmd_with_arg(u8 cmd, u32 arg);
u8 sd_wait_notff(void);
u8 sd_read_sector(u8 *buff, u32 sectnumb);
u8 sd_init(void);

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef _SD_H


