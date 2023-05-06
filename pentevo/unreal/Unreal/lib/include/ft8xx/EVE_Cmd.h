/**
* This source code ("the Software") is provided by Bridgetek Pte Ltd
* ("Bridgetek") subject to the licence terms set out
*   http://brtchip.com/BRTSourceCodeLicenseAgreement/ ("the Licence Terms").
* You must read the Licence Terms before downloading or using the Software.
* By installing or using the Software you agree to the Licence Terms. If you
* do not agree to the Licence Terms then do not download or use the Software.
*
* Without prejudice to the Licence Terms, here is a summary of some of the key
* terms of the Licence Terms (and in the event of any conflict between this
* summary and the Licence Terms then the text of the Licence Terms will
* prevail).
*
* The Software is provided "as is".
* There are no warranties (or similar) in relation to the quality of the
* Software. You use it at your own risk.
* The Software should not be used in, or for, any medical device, system or
* appliance. There are exclusions of Bridgetek liability for certain types of loss
* such as: special loss or damage; incidental loss or damage; indirect or
* consequential loss or damage; loss of income; loss of business; loss of
* profits; loss of revenue; loss of contracts; business interruption; loss of
* the use of money or anticipated savings; loss of information; loss of
* opportunity; loss of goodwill or reputation; and/or loss of, damage to or
* corruption of data.
* There is a monetary cap on Bridgetek's liability.
* The Software may have subsequently been amended by another user and then
* distributed by that other user ("Adapted Software").  If so that user may
* have additional licence terms that apply to those amendments. However, Bridgetek
* has no liability in relation to those amendments.
*
* File Description:
*    This file defines the generic APIs of phost access layer for the FT800 or EVE compatible silicon.
*    Application shall access FT800 or EVE resources over these APIs,regardless of I2C or SPI protocol.
*    In addition, there are some helper functions defined for FT800 coprocessor engine as well as phost commands.
*
*/

#ifndef EVE_CMD__H
#define EVE_CMD__H
#include "EVE_HalDefs.h"

/********************
** COPROCESSOR CMD **
********************/

#define EVE_CMD_STRING_MAX 511

/* Get the current read pointer.
Safe to use during ongoing command transaction */
EVE_HAL_EXPORT uint16_t EVE_Cmd_rp(EVE_HalContext *phost);

/* Get the current write pointer.
Updates cached write pointer when CMDB is not supported.
Safe to use during ongoing command transaction */
EVE_HAL_EXPORT uint16_t EVE_Cmd_wp(EVE_HalContext *phost);

/* Get the currently available space.
Updates cached available space.
Safe to use during ongoing command transaction */
EVE_HAL_EXPORT uint16_t EVE_Cmd_space(EVE_HalContext *phost);

/* Begin writing a function, keeps the transfer open.
While a command transaction is ongoing,
HAL functions outside of EVE_Cmd_* must not be used. */
EVE_HAL_EXPORT void EVE_Cmd_startFunc(EVE_HalContext *phost);

/* End writing a function, closes the transfer */
EVE_HAL_EXPORT void EVE_Cmd_endFunc(EVE_HalContext *phost);

/* Write a buffer to the command buffer. 
Waits if there is not enough space in the command buffer. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_wrMem(EVE_HalContext *phost, const uint8_t *buffer, uint32_t size);

/* Write a progmem buffer to the command buffer. 
Waits if there is not enough space in the command buffer. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_wrProgMem(EVE_HalContext *phost, eve_progmem_const uint8_t *buffer, uint32_t size);

/* Write a string to the command buffer, padded to 4 bytes. 
Waits if there is not enough space in the command buffer. 
Parameter `maxLength` can be set up to `EVE_CMD_STRING_MAX`.
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT uint32_t EVE_Cmd_wrString(EVE_HalContext *phost, const char *str, uint32_t maxLength);

/* Write a 8-bit value to the command buffer. 
Uses a cache to write 4 bytes at once. 
Waits if there is not enough space in the command buffer. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_wr8(EVE_HalContext *phost, uint8_t value);

/* Write a 16-bit value to the command buffer. 
Uses a cache to write 4 bytes at once. 
Wire endianness is handled by the transfer. 
Waits if there is not enough space in the command buffer. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_wr16(EVE_HalContext *phost, uint16_t value);

/* Write a value to the command buffer. 
Wire endianness is handled by the transfer. 
Waits if there is not enough space in the command buffer. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_wr32(EVE_HalContext *phost, uint32_t value);

/* Move the write pointer forward by the specified number of bytes. 
Returns the previous write pointer */
EVE_HAL_EXPORT uint16_t EVE_Cmd_moveWp(EVE_HalContext *phost, uint16_t bytes);

/* Wait for the command buffer to fully empty. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_Cmd_waitFlush(EVE_HalContext *phost);

/* Wait for the command buffer to have at least the requested amount of free space.
Returns 0 in case a coprocessor fault occurred */
EVE_HAL_EXPORT uint32_t EVE_Cmd_waitSpace(EVE_HalContext *phost, uint32_t size);

/* Wait for logo to finish displaying. 
(Waits for both the read and write pointer to go to 0) */
EVE_HAL_EXPORT bool EVE_Cmd_waitLogo(EVE_HalContext *phost);

/* Restore the internal state of EVE_Cmd.
Call this after manually writing the the coprocessor buffer */
EVE_HAL_EXPORT void EVE_Cmd_restore(EVE_HalContext *phost);

#endif /* #ifndef EVE_HAL_INCL__H */

/* end of file */