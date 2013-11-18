#ifndef __VERSION_H__
#define __VERSION_H__

/**
 * @file
 * @brief Base configuration and bootloader version out support.
 * @author http://www.nedopc.com
 *
 * Getting base configuration and bootloader version.
 *
 * Version data is 16 bytes [indexes 0x00..0x0F]. Data format:
 * - 0x00..0x0B [12 bytes]: version name string, may be zero ended if name shoter then 12 bytes;
 * - 0x0C..0x0D [2 bytes]: revision date and officiality bit;
 * - 0x0E..0x0F [2 bytes]: CRC value of bootloader or base configuration code.
 *
 * Revision date and officiality bit format:
 * - 7 bit of 0x0D: officiality bit (0 - test version, 1 - official release);
 * - 6..1 bits of 0x0D: year (value 0..63 means 2000..2063 year);
 * - 0 bit of 0x0D and 7..5 bits of 0x0C: month (value 1..12);
 * - 4..0 bits of 0x0C: day (value 1..31).
 *
 * Example:
 * 50 65 6E 74 31 6D 00 00 00 00 00 00 7B 14 3C B1
 * 50 65 6E 74 31 6D 00 00 00 00 00 00 = name string: "Pent1m";
 * 7B 14 = non official, 10 year, 03 month, 27 day: release date "27.03.2010";
 * 3C B1 = CRC.
 *
 * Recommend type next strings on display:
 * "Pent1m 27.03.2010 beta" - name and release date (officiality bit is not set);
 * "Pent1m 27.03.2010" - name and release date (officiality bit is set).
 */

/**
 * Get version byte.
 * @return version byte
 * @param index [in] - index of byte (0x00..0x0F)
 */
UBYTE GetVersionByte(UBYTE index);

/**
 * Set version type.
 * @param type [in] - 0: base configuration, 1: bootloader
 */
void SetVersionType(UBYTE type);

#endif //__VERSION_H__
