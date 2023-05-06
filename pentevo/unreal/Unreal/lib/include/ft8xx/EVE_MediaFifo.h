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

#ifndef EVE_MEDIAFIFO__H
#define EVE_MEDIAFIFO__H
#include "EVE_HalDefs.h"

/**************************
** COPROCESSOR MEDIAFIFO **
***************************/

#ifdef EVE_SUPPORT_MEDIAFIFO

/* Set the media FIFO. 
Returns false in case a coprocessor fault occurred */
EVE_HAL_EXPORT bool EVE_MediaFifo_set(EVE_HalContext *phost, uint32_t address, uint32_t size);

/* Closes the current media FIFO.
Indication for HAL only */
EVE_HAL_EXPORT void EVE_MediaFifo_close(EVE_HalContext *phost);

/* Get the current read pointer. */
EVE_HAL_EXPORT uint32_t EVE_MediaFifo_rp(EVE_HalContext *phost);

/* Get the current write pointer. */
EVE_HAL_EXPORT uint32_t EVE_MediaFifo_wp(EVE_HalContext *phost);

/* Get the currently available space. */
EVE_HAL_EXPORT uint32_t EVE_MediaFifo_space(EVE_HalContext *phost);

/* Write a buffer to the media FIFO. 
Waits if there is not enough space in the media FIFO. 
Returns false in case a coprocessor fault occurred.
If the transfered pointer is set, the write may exit early 
if the coprocessor function has finished, and the
transfered amount will be set. */
EVE_HAL_EXPORT bool EVE_MediaFifo_wrMem(EVE_HalContext *phost, const uint8_t *buffer, uint32_t size, uint32_t *transfered);

/* Wait for the media FIFO to fully empty. 
When checking if a file is fully processed, EVE_Cmd_waitFlush must be called.
Returns false in case a coprocessor fault occurred, or in case the coprocessor is done processing */
EVE_HAL_EXPORT bool EVE_MediaFifo_waitFlush(EVE_HalContext *phost, bool orCmdFlush);

/* Wait for the media FIFO to have at least the requested amount of free space.
Returns 0 in case a coprocessor fault occurred, or in case the coprocessor is done processing */
EVE_HAL_EXPORT uint32_t EVE_MediaFifo_waitSpace(EVE_HalContext *phost, uint32_t size, bool orCmdFlush);

#else

#define EVE_MediaFifo_set(phost, address, size) (false)
#define EVE_MediaFifo_rp(phost) (0)
#define EVE_MediaFifo_wp(phost) (0)
#define EVE_MediaFifo_space(phost) (~0)
#define EVE_MediaFifo_wrMem(phost, buffer, size, transfered) (false)
#define EVE_MediaFifo_waitFlush(phost) (false)
#define EVE_MediaFifo_waitSpace(phost, size) (false)

#endif

#endif /* #ifndef EVE_HAL_INCL__H */

/* end of file */