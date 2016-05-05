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
0.1 - date 2013.04.24  - initial version
0.2 - date 2014.04.28 - Split in individual files according to platform

*/

/*
 * Copyright (C) 2013  Future Technology Devices International Ltd
 */

#ifndef _FT_EMULATOR_H
#define _FT_EMULATOR_H
// #include <...>


// Project includes (include standard stuff for user)
#include "FT_Platform.h"

// API version is increased whenever FT8XXEMU_EmulatorParameters format changes or functions are modified
#define FT8XXEMU_VERSION_API 9

#ifndef FT8XXEMU_STATIC
#	ifdef FT8XXEMU_EXPORT_DYNAMIC
#		ifdef WIN32
#			define FT8XXEMU_API __declspec(dllexport)
#		else
#			define FT8XXEMU_API
#		endif
#	else
#		ifdef WIN32
#			define FT8XXEMU_API __declspec(dllimport)
#		else
#			define FT8XXEMU_API
#		endif
#	endif
#else
#	define FT8XXEMU_API
#endif

typedef enum
{
	FT8XXEMU_EmulatorFT800 = 0x0800,
	FT8XXEMU_EmulatorFT801 = 0x0801,
	FT8XXEMU_EmulatorFT810 = 0x0810,
	FT8XXEMU_EmulatorFT811 = 0x0811,
	FT8XXEMU_EmulatorFT812 = 0x0812,
	FT8XXEMU_EmulatorFT813 = 0x0813,
} FT8XXEMU_EmulatorMode;

typedef enum
{
	// enables the keyboard to be used as input (default: on)
	FT8XXEMU_EmulatorEnableKeyboard = 0x01,
	// enables audio (default: on)
	FT8XXEMU_EmulatorEnableAudio = 0x02,
	// enables coprocessor (default: on)
	FT8XXEMU_EmulatorEnableCoprocessor = 0x04,
	// enables mouse as touch (default: on)
	FT8XXEMU_EmulatorEnableMouse = 0x08,
	// enable debug shortkeys (default: on)
	FT8XXEMU_EmulatorEnableDebugShortkeys = 0x10,
	// enable graphics processor multithreading (default: on)
	FT8XXEMU_EmulatorEnableGraphicsMultithread = 0x20,
	// enable dynamic graphics quality degrading by interlacing and dropping frames (default: on)
	FT8XXEMU_EmulatorEnableDynamicDegrade = 0x40,
	// enable usage of REG_ROTATE (default: off)
	// FT8XXEMU_EmulatorEnableRegRotate = 0x80, // Now always on
	// enable emulating REG_PWM_DUTY by fading the rendered display to black (default: off)
	FT8XXEMU_EmulatorEnableRegPwmDutyEmulation = 0x100,
	// enable usage of touch transformation matrix (default: on) (should be disabled in editor)
	FT8XXEMU_EmulatorEnableTouchTransformation = 0x200,
} FT8XXEMU_EmulatorFlags;

typedef enum
{
	// frame render has changes since last render
	FT8XXEMU_FrameBufferChanged = 0x01,
	// frame is completely rendered (without degrade)
	FT8XXEMU_FrameBufferComplete = 0x02,
	// frame has changes since last render
	FT8XXEMU_FrameChanged = 0x04,
	// frame rendered right after display list swap
	FT8XXEMU_FrameSwap = 0x08,

	// NOTE: Difference between FrameChanged and FrameBufferChanged is that
	// FrameChanged will only be true if the content of the frame changed,
	// whereas FrameBufferChanged will be true if the rendered buffer changed.
	// For example, when the emulator renders a frame incompletely due to
	// CPU overload, it will then finish the frame in the next callback,
	// and when this is the same frame, FrameChanged will be false,
	// but FrameBufferChanged will be true as the buffer has changed.

	// NOTE: Frames can change even though no frame was swapped, due to
	// several parameters such as REG_MACRO or REG_ROTATE.

	// NOTE: If you only want completely rendered frames, turn OFF
	// the EmulatorEnableDynamicDegrade feature.

	// NOTE: To get the accurate frame after a frame swap, wait for FrameSwap
	// to be set, and get the first frame which has FrameBufferComplete set.

	// NOTE: To get the accurate frame after any frame change, wait for
	// FrameChanged, and get the first frame which has FrameBufferComplete set.
} FT8XXEMU_FrameFlags;

typedef struct
{
	// Microcontroller function called before loop.
	void(*Setup)();
	// Microcontroller continuous loop.
	void(*Loop)();
	// See EmulatorFlags.
	int Flags;
	// Emulator mode
	FT8XXEMU_EmulatorMode Mode;

	// Called after keyboard update.
	// Supplied function can use Keyboard.isKeyDown(FT8XXEMU_KEY_F3)
	// or FT8XXEMU_isKeyDown(FT8XXEMU_KEY_F3) functions.
	void(*Keyboard)();
	// The default mouse pressure, default 0 (maximum).
	// See REG_TOUCH_RZTRESH, etc.
	uint32_t MousePressure;
	// External frequency. See CLK, etc.
	uint32_t ExternalFrequency;

	// Reduce graphics processor threads by specified number, default 0
	// Necessary when doing very heavy work on the MCU or Coprocessor
	uint32_t ReduceGraphicsThreads;

	// Sleep function for MCU thread usage throttle. Defaults to generic system sleep
	void(*MCUSleep)(int ms);

	// Replaces the default builtin ROM with a custom ROM from a file.
	// NOTE: String is copied and may be deallocated after call to run(...)
	char *RomFilePath;
	// Replaces the default builtin OTP with a custom OTP from a file.
	// NOTE: String is copied and may be deallocated after call to run(...)
	char *OtpFilePath;
	// Replaces the builtin coprocessor ROM.
	// NOTE: String is copied and may be deallocated after call to run(...)
	char *CoprocessorRomFilePath;

	// Graphics driverless mode
	// Setting this callback means no window will be created, and all
	// rendered graphics will be automatically sent to this function.
	// For enabling touch functionality, the functions
	// Memory.setTouchScreenXY and Memory.resetTouchScreenXY must be
	// called manually from the host application.
	// Builtin keyboard functionality is not supported and must be
	// implemented manually when using this mode.
	// The output parameter is false (0) when the display is turned off.
	// The contents of the buffer pointer are undefined after this
	// function returns.
	// Return false (0) when the application must exit, otherwise return true (1).
	int(*Graphics)(int output, const argb8888 *buffer, uint32_t hsize, uint32_t vsize, FT8XXEMU_FrameFlags flags);

	// Interrupt handler
	// void (*Interrupt)();

	// Exception callback
	void(*Exception)(const char *message);

	// Safe exit
	void(*Close)();

} FT8XXEMU_EmulatorParameters;

#ifdef __cplusplus 
extern "C" {
#endif

//////////
// INIT //
//////////

// Return version information
FT8XXEMU_API const char *FT8XXEMU_version();

// Initialize the default emulator parameters
FT8XXEMU_API void FT8XXEMU_defaults(uint32_t versionApi, FT8XXEMU_EmulatorParameters *params, FT8XXEMU_EmulatorMode mode);

// Run the emulator on the current thread. Returns when the emulator is fully stopped. Parameter versionApi must be set to FT8XXEMU_VERSION_API
FT8XXEMU_API void FT8XXEMU_run(uint32_t versionApi, const FT8XXEMU_EmulatorParameters *params);

// Stop the emulator. Can be called from any thread. Returns when the emulator has fully stopped
FT8XXEMU_API extern void(*FT8XXEMU_stop)();

/////////////
// RUNTIME //
/////////////

// Transfer data over the imaginary SPI bus. Call from the MCU thread (from the setup/loop callbacks). See FT8XX documentation for SPI transfer protocol
FT8XXEMU_API extern uint8_t (*FT8XXEMU_transfer)(uint8_t data);

// Set cable select. Must be set to 1 to start data transfer, 0 to end. See FT8XX documentation for CS_N
FT8XXEMU_API extern void (*FT8XXEMU_cs)(int cs);

// Returns 1 if there is an interrupt flag set. Depends on mask. See FT8XX documentation for INT_N
FT8XXEMU_API extern int (*FT8XXEMU_int)();

//////////////
// ADVANCED //
//////////////

// Set touch XY. Param idx 0..4. Call on every frame during mouse down or touch when using custom graphics output
FT8XXEMU_API void FT8XXEMU_touchSetXY(int idx, int x, int y, int pressure);

// Reset touch XY. Call once no longer touching when using custom graphics output
FT8XXEMU_API void FT8XXEMU_touchResetXY(int idx);

#ifdef __cplusplus 
} /* extern "C" */
#endif

#endif /* #ifndef FT800EMU_EMULATOR_H */

/* end of file */
