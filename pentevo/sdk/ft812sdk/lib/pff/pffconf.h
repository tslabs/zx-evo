/*---------------------------------------------------------------------------/
/  Petit FatFs - Configuration file  R0.03 (C)ChaN, 2014
/---------------------------------------------------------------------------*/

#ifndef _PFFCONF
#define _PFFCONF 4004	/* Revision ID */

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define _FS_FAT12	0	/* Enable FAT12 */
#define _FS_FAT16	0	/* Enable FAT16 */
#define _FS_FAT32	1	/* Enable FAT32 */

/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define _WORD_ACCESS	1
/* The _WORD_ACCESS option is an only platform dependent option. It defines
/  which access method is used to the word data on the FAT volume.
/
/   0: Byte-by-byte access. Always compatible with all platforms.
/   1: Word access. Do not choose this unless under both the following conditions.
/
/  * Address misaligned memory access is always allowed for ALL instructions.
/  * Byte order on the memory is little-endian.
/
/  If it is the case, _WORD_ACCESS can also be set to 1 to improve performance and
/  reduce code size. Following table shows an example of some processor types.
/
/   ARM7TDMI    0           ColdFire    0           V850E       0
/   Cortex-M3   0           Z80         0/1         V850ES      0/1
/   Cortex-M0   0           RX600(LE)   0/1         TLCS-870    0/1
/   AVR         0/1         RX600(BE)   0           TLCS-900    0/1
/   AVR32       0           RL78        0           R32C        0
/   PIC18       0/1         SH-2        0           M16C        0/1
/   PIC24       0           H8S         0           MSP430      0
/   PIC32       0           H8/300H     0           x86         0/1
*/

#endif /* _PFFCONF */
