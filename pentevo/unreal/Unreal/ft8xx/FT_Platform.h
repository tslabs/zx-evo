/*

Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Abstract:

This file contains is functions for all UI fields.

Author : FTDI 

Revision History: 
0.1 - date 2013.04.24 - initial version
0.2 - date 2014.04.28 - Split in individual files according to platform
0.3 - date 2015.09.25 - Added VM810C50 platform macro
*/

#ifndef _FT_PLATFORM_H_
#define _FT_PLATFORM_H_

/* platform specific macros */
#define MSVC_FT800EMU 							(1)// enable by default for emulator platform

#define VM800B43_50								(1)
//#define VM800B35								(1)
//#define VM801B43_50								(1)
//#define VM810C50								(1)

#ifdef VM800B43_50

/* Define all the macros specific to VM800B43_50 module */
#define FT_800_ENABLE							(1)
#define DISPLAY_RESOLUTION_WQVGA				(1)
#define ENABLE_SPI_SINGLE						(1)
#define RESISTANCE_THRESHOLD					(1200)
#endif /* VM800B43_50 */

#ifdef VM800B35

#define FT_800_ENABLE							(1)
#define DISPLAY_RESOLUTION_QVGA					(1)
#define ENABLE_SPI_SINGLE						(1)
#define RESISTANCE_THRESHOLD					(1200)
#endif /* VM800B35 */

#ifdef VM801B43_50

#define FT_801_ENABLE							(1)
#define DISPLAY_RESOLUTION_WQVGA				(1)
#define ENABLE_SPI_SINGLE						(1)

#endif

#ifdef VM810C50

#define FT_810_ENABLE							(1)
#define DISPLAY_RESOLUTION_WVGA					(1)
#define RESISTANCE_THRESHOLD					(1200)

#endif /* VM810C50 */

/* Custom configuration */
#if (!defined(VM800B43_50) && !defined(VM800B35) && !defined(VM801B43_50) && !defined(VM810C50))

/* Display configuration specific macros */
#define DISPLAY_RESOLUTION_QVGA					(1)
#define DISPLAY_RESOLUTION_WQVGA				(1)
#define DISPLAY_RESOLUTION_WVGA					(1)
#define DISPLAY_RESOLUTION_HVGA_PORTRAIT		(1)

/* Chip configuration specific macros */
#define FT_800_ENABLE							(1)
#define FT_801_ENABLE							(1)
#define FT_810_ENABLE							(1)
#define FT_811_ENABLE							(1)
#define FT_812_ENABLE							(1)
#define FT_813_ENABLE							(1)

/* SPI specific macros - compile time switches for SPI single, dial and quad use cases */
#define ENABLE_SPI_SINGLE						(1)
#define ENABLE_SPI_DUAL							(1)
#define ENABLE_SPI_QUAD							(1)

#define RESISTANCE_THRESHOLD					(1200)

#endif
#if defined(FT_800_ENABLE) || defined(FT_801_ENABLE)
#define FT_80X_ENABLE							(1)
#endif

#if (defined(FT_810_ENABLE) || defined(FT_811_ENABLE) || defined(FT_812_ENABLE) || defined(FT_813_ENABLE))
#define FT_81X_ENABLE							(1)
#endif

/* Standard C libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include <direct.h>
#include <time.h>
#include <io.h>
/* HAL inclusions */
#include "FT_DataTypes.h"

#include "FT_EmulatorMain.h"
#include "FT_Gpu_Hal.h"
#include "FT_Gpu.h"
#include "FT_CoPro_Cmds.h"
#include "FT_Hal_Utils.h"
#define BUFFER_OPTIMIZATION


#define FT800_SEL_PIN   0
#define FT800_PD_N      7


#endif /*_FT_PLATFORM_H_*/
/* Nothing beyond this*/




