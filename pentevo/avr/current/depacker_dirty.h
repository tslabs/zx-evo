#ifndef DEPACKER_DIRTY_H
#define DEPACKER_DIRTY_H

/**
 * @file
 * @brief Depack and load fpga configuration.
 * @author http://www.nedopc.com
 *
 * Depacker use MegaLZ without any checks.
 */

/** Size of output buffer */
#define DBSIZE 2048
/** Mask of output buffer */
#define DBMASK 2047

/** Get next byte. */
#define NEXT_BYTE (pgm_read_byte_far(curFpga++))

/** Actual depacker, 8bit-oriented and without any checks. */
void  depacker_dirty(void);

/** */
UBYTE get_bits_dirty(UBYTE numbits);
WORD get_bigdisp_dirty(void);

/** */
void put_byte(UBYTE);
void repeat(WORD,UBYTE);

#endif

