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
*/

#include "EVE_Platform.h"

/*
On Windows platform, filenames are assumed to be in the local character set.
The unicode variants of the functions can be used for unicode paths.
On Linux platform, filenames are assumed to be in UTF-8.
On embedded platforms, filename character set depends on the filesystem library.
*/

/* Load SD card */
EVE_HAL_EXPORT bool EVE_Util_loadSdCard(EVE_HalContext *phost);
EVE_HAL_EXPORT bool EVE_Util_sdCardReady(EVE_HalContext *phost);

EVE_HAL_EXPORT bool EVE_Util_loadRawFile(EVE_HalContext *phost, uint32_t address, const char *filename);
EVE_HAL_EXPORT bool EVE_Util_loadInflateFile(EVE_HalContext *phost, uint32_t address, const char *filename);

/* Load a file using CMD_LOADIMAGE.
The image format is provided as output to the optional format argument */
EVE_HAL_EXPORT bool EVE_Util_loadImageFile(EVE_HalContext *phost, uint32_t address, const char *filename, uint32_t *format);

/* Load a file into the coprocessor FIFO */
EVE_HAL_EXPORT bool EVE_Util_loadCmdFile(EVE_HalContext *phost, const char *filename, uint32_t *transfered);

#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
/* Load a file into the media FIFO.
If transfered is set, the file may be streamed partially,
and will be kept open until EVE_Util_closeFile is called, 
and stop once the coprocessor has processed it.
Filename may be omitted in subsequent calls */
EVE_HAL_EXPORT bool EVE_Util_loadMediaFile(EVE_HalContext *phost, const char *filename, uint32_t *transfered);

EVE_HAL_EXPORT void EVE_Util_closeFile(EVE_HalContext *phost);
#endif

#ifdef _WIN32

EVE_HAL_EXPORT bool EVE_Util_loadRawFileW(EVE_HalContext *phost, uint32_t address, const wchar_t *filename);
EVE_HAL_EXPORT bool EVE_Util_loadInflateFileW(EVE_HalContext *phost, uint32_t address, const wchar_t *filename);

/* Load a file using CMD_LOADIMAGE.
The image format is provided as output to the optional format argument */
EVE_HAL_EXPORT bool EVE_Util_loadImageFileW(EVE_HalContext *phost, uint32_t address, const wchar_t *filename, uint32_t *format);

/* Load a file into the coprocessor FIFO */
EVE_HAL_EXPORT bool EVE_Util_loadCmdFileW(EVE_HalContext *phost, const wchar_t *filename, uint32_t *transfered);

#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
/* Load a file into the media FIFO.
If transfered is set, the file may be streamed partially,
and will be kept open until EVE_Util_closeFile is called, 
and stop once the coprocessor has processed it.
Filename may be omitted in subsequent calls  */
EVE_HAL_EXPORT bool EVE_Util_loadMediaFileW(EVE_HalContext *phost, const wchar_t *filename, uint32_t *transfered);
#endif

#endif

/* end of file */
