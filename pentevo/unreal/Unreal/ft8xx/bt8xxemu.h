/*
BT8XX Emulator Library
Copyright (C) 2013-2016  Future Technology Devices International Ltd
Copyright (C) 2016-2020  Bridgetek Pte Lte
Author: Jan Boon <jan.boon@kaetemi.be>
*/

#ifndef BT8XXEMU_H
#define BT8XXEMU_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26812) // Unscoped enum
#endif

#ifndef EVE_TCHAR_DEFINED
#define EVE_TCHAR_DEFINED
#ifdef _WIN32
typedef wchar_t eve_tchar_t;
#else
typedef char eve_tchar_t;
#endif
#endif

#include "bt8xxemu_inttypes.h"

// API version is increased whenever BT8XXEMU_EmulatorParameters format changes or functions are modified
#define BT8XXEMU_VERSION_API 12

#ifdef BT8XXEMU_REMOTE
#ifndef _WIN32
#undef BT8XXEMU_REMOTE /* Not yet supported */
#else
#define BT8XXEMU_STATIC
#endif
#endif

#ifndef BT8XXEMU_STATIC
#ifdef BT8XXEMU_EXPORT_DYNAMIC
#ifdef _WIN32
#define BT8XXEMU_API __declspec(dllexport)
#else
#define BT8XXEMU_API
#endif
#else
#ifdef _WIN32
#define BT8XXEMU_API __declspec(dllimport)
#else
#define BT8XXEMU_API
#endif
#endif
#else
#define BT8XXEMU_API
#endif

#ifdef __cplusplus

namespace BT8XXEMU {
class Emulator;
class Flash;
}

typedef BT8XXEMU::Emulator BT8XXEMU_Emulator;
typedef BT8XXEMU::Flash BT8XXEMU_Flash;

#elif defined(BT8XXEMU_REMOTE)

typedef struct BT8XXEMUC_Remote BT8XXEMU_Emulator;
typedef struct BT8XXEMUC_Remote BT8XXEMU_Flash;

#else

typedef void BT8XXEMU_Emulator;
typedef void BT8XXEMU_Flash;

#endif /* #ifdef __cplusplus */

typedef enum
{
	BT8XXEMU_LogError = 0,
	BT8XXEMU_LogWarning = 1,
	BT8XXEMU_LogMessage = 2,
} BT8XXEMU_LogType;

typedef enum
{
	BT8XXEMU_EmulatorFT800 = 0x0800,
	BT8XXEMU_EmulatorFT801 = 0x0801,
	BT8XXEMU_EmulatorFT810 = 0x0810,
	BT8XXEMU_EmulatorFT811 = 0x0811,
	BT8XXEMU_EmulatorFT812 = 0x0812,
	BT8XXEMU_EmulatorFT813 = 0x0813,
	BT8XXEMU_EmulatorBT815 = 0x0815,
	BT8XXEMU_EmulatorBT816 = 0x0816,
	BT8XXEMU_EmulatorBT817 = 0x0817,
	BT8XXEMU_EmulatorBT818 = 0x0818,
} BT8XXEMU_EmulatorMode;

typedef enum
{
	// enables the keyboard to be used as input (default: on)
	BT8XXEMU_EmulatorEnableKeyboard = 0x01,
	// enables audio (default: on)
	BT8XXEMU_EmulatorEnableAudio = 0x02,
	// enables coprocessor (default: on)
	BT8XXEMU_EmulatorEnableCoprocessor = 0x04,
	// enables mouse as touch (default: on)
	BT8XXEMU_EmulatorEnableMouse = 0x08,
	// enable debug shortkeys (default: on)
	BT8XXEMU_EmulatorEnableDebugShortkeys = 0x10,
	// enable graphics processor multithreading (default: on)
	BT8XXEMU_EmulatorEnableGraphicsMultithread = 0x20,
	// enable dynamic graphics quality degrading by reducing resolution and dropping frames (default: on)
	BT8XXEMU_EmulatorEnableDynamicDegrade = 0x40,
	// enable usage of REG_ROTATE (default: off)
	// BT8XXEMU_EmulatorEnableRegRotate = 0x80, // Now always on
	// enable emulating REG_PWM_DUTY by fading the rendered display to black (default: off)
	BT8XXEMU_EmulatorEnableRegPwmDutyEmulation = 0x100,
	// enable usage of touch transformation matrix (default: on) (should be disabled in editor)
	BT8XXEMU_EmulatorEnableTouchTransformation = 0x200,
	// enable output to stdout from the emulator (default: off) (note: stdout is is some cases not thread safe)
	BT8XXEMU_EmulatorEnableStdOut = 0x400,
	// enable performance adjustments for running the emulator as a background process without window (default: off)
	BT8XXEMU_EmulatorEnableBackgroundPerformance = 0x800,
	// enable performance adjustments for the main MCU thread (default: on)
	BT8XXEMU_EmulatorEnableMainPerformance = 0x1000,
	// enable HSF preview (default: on for bt817 and up, off otherwise)
	BT8XXEMU_EmulatorEnableHSFPreview = 0x2000,

} BT8XXEMU_EmulatorFlags;

typedef enum
{
	// frame render has changes since last render
	BT8XXEMU_FrameBufferChanged = 0x01,
	// frame is completely rendered (without degrade)
	BT8XXEMU_FrameBufferComplete = 0x02,
	// frame has changes since last render
	BT8XXEMU_FrameChanged = 0x04,
	// frame rendered right after display list swap
	BT8XXEMU_FrameSwap = 0x08,

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

} BT8XXEMU_FrameFlags;

typedef struct
{
	// Microcontroller main function. This will be run on a new thread managed by the emulator. When not provided the calling thread is assumed to be the MCU thread
	void (*Main)(BT8XXEMU_Emulator *sender, void *context);
	// See EmulatorFlags.
	int Flags;
	// Emulator mode
	BT8XXEMU_EmulatorMode Mode;

	// The default mouse pressure, default 0 (maximum).
	// See REG_TOUCH_RZTRESH, etc.
	uint32_t MousePressure;
	// External frequency. See CLK, etc.
	uint32_t ExternalFrequency;

	// Reduce graphics processor threads by specified number, default 0
	// Necessary when doing very heavy work on the MCU or Coprocessor
	uint32_t ReduceGraphicsThreads;

	// Sleep function for MCU thread usage throttle. Defaults to generic system sleep
	void (*MCUSleep)(BT8XXEMU_Emulator *sender, void *context, int ms);

	// Replaces the default builtin ROM with a custom ROM from a file.
	// NOTE: String is copied and may be deallocated after call to run(...)
	eve_tchar_t RomFilePath[260];
	// Replaces the default builtin OTP with a custom OTP from a file.
	// NOTE: String is copied and may be deallocated after call to run(...)
	eve_tchar_t OtpFilePath[260];
	// Replaces the builtin coprocessor ROM.
	// NOTE: String is copied and may be deallocated after call to run(...)
	eve_tchar_t CoprocessorRomFilePath[260];

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
	// function returns. Create a copy to use it on another thread.
	// Return false (0) when the application must exit, otherwise return true (1).
	int (*Graphics)(BT8XXEMU_Emulator *sender, void *context, int output, const argb8888 *buffer, uint32_t hsize, uint32_t vsize, BT8XXEMU_FrameFlags flags);

	// Interrupt handler
	// void (*Interrupt)(void *sender, void *context);

	// Log callback
	void (*Log)(BT8XXEMU_Emulator *sender, void *context, BT8XXEMU_LogType type, const char *message);

	// Safe exit. Called when the emulator window is closed
	void (*Close)(BT8XXEMU_Emulator *sender, void *context);

	// User context that will be passed along to callbacks
	void *UserContext;

	// Flash device to connect with, default NULL
	BT8XXEMU_Flash *Flash;

} BT8XXEMU_EmulatorParameters;

typedef struct
{
	// Device type, by library name, default "mx25lemu"
	eve_tchar_t DeviceType[26];

	// Size of the flash memory in bytes, default 8MB
	ptrdiff_t SizeBytes;

	// Data file to load onto the flash, default NULL
	eve_tchar_t DataFilePath[260];

	// Internal flash status file, device specific, default NULL
	eve_tchar_t StatusFilePath[260];

	// Write actions to the flash are persisted to the used file.
	// This is accomplished by memory mapping the file, instead of
	// the file being copied to memory. Default false
	int Persistent;

	// Print log to standard output. Default false
	int StdOut;

	// Data buffer that is written to the flash initially,
	// overriding any existing contents that may have been
	// loaded from a flash file already, default NULL and 0
	void *Data;
	ptrdiff_t DataSizeBytes;

	// Log callback
	void (*Log)(BT8XXEMU_Flash *sender, void *context, BT8XXEMU_LogType type, const char *message);

	// User context that will be passed along to callbacks
	void *UserContext;

} BT8XXEMU_FlashParameters;

#ifdef __cplusplus
extern "C" {
#endif

//////////
// INIT //
//////////

// Return version information
BT8XXEMU_API extern const char *BT8XXEMU_version();

// Initialize the default emulator parameters
BT8XXEMU_API extern void BT8XXEMU_defaults(uint32_t versionApi, BT8XXEMU_EmulatorParameters *params, BT8XXEMU_EmulatorMode mode);

// Run the emulator on the current thread. Returns when the emulator is fully stopped when a Main function is supplied, returns when the emulator is fully started otherwise. Parameter versionApi must be set to BT8XXEMU_VERSION_API
BT8XXEMU_API extern void BT8XXEMU_run(uint32_t versionApi, BT8XXEMU_Emulator **emulator, const BT8XXEMU_EmulatorParameters *params);

// Stop the emulator. Can be called from any thread. Returns when the emulator has fully stopped. Safe to call multiple times.
BT8XXEMU_API extern void BT8XXEMU_stop(BT8XXEMU_Emulator *emulator);

// Destroy the emulator. Calls BT8XXEMU_stop implicitly. Emulator must be destroyed before process exits.
BT8XXEMU_API extern void BT8XXEMU_destroy(BT8XXEMU_Emulator *emulator);

// Poll if the emulator is still running. Returns 0 when the output window has been closed, or when the emulator has been stopped.
BT8XXEMU_API extern int BT8XXEMU_isRunning(BT8XXEMU_Emulator *emulator);

/////////////
// RUNTIME //
/////////////

// Transfer data over the imaginary SPI bus. Call from the MCU thread (from the setup/loop callbacks). See FT8XX documentation for SPI transfer protocol
BT8XXEMU_API extern uint8_t BT8XXEMU_transfer(BT8XXEMU_Emulator *emulator, uint8_t data);

// Set chip select. Must be set to 1 to start data transfer, 0 to end. See FT8XX documentation for CS_N
BT8XXEMU_API extern void BT8XXEMU_chipSelect(BT8XXEMU_Emulator *, int cs);

// Returns 1 if there is an interrupt flag set. Depends on mask. See FT8XX documentation for INT_N
BT8XXEMU_API extern int BT8XXEMU_hasInterrupt(BT8XXEMU_Emulator *emulator);

//////////////
// ADVANCED //
//////////////

// Set touch XY. Param idx 0..4. Call on every frame during mouse down or touch when using custom graphics output
BT8XXEMU_API extern void BT8XXEMU_touchSetXY(BT8XXEMU_Emulator *emulator, int idx, int x, int y, int pressure);

// Reset touch XY. Call once no longer touching when using custom graphics output
BT8XXEMU_API extern void BT8XXEMU_touchResetXY(BT8XXEMU_Emulator *emulator, int idx);

// Set a single emulation flag on or off. Only PWM and HSF options can be changed at runtime. Returns the value of the flag after the operation
BT8XXEMU_API extern int BT8XXEMU_setFlag(BT8XXEMU_Emulator *emulator, BT8XXEMU_EmulatorFlags flag, int value);

///////////
// FLASH //
///////////

#ifndef BT8XXEMU_FLASH_LIBRARY

// Initialize the default flash emulator parameters
BT8XXEMU_API extern void BT8XXEMU_Flash_defaults(uint32_t versionApi, BT8XXEMU_FlashParameters *params);

// Create flash emulator instance
BT8XXEMU_API extern BT8XXEMU_Flash *BT8XXEMU_Flash_create(uint32_t versionApi, const BT8XXEMU_FlashParameters *params);

// Destroy flash emulator instance
BT8XXEMU_API extern void BT8XXEMU_Flash_destroy(BT8XXEMU_Flash *flash);

// Transfer data using SPI or Quad SPI protocol. Bit 0:3 are data, bit 4 is cable select (0 active), SCK is clock. In single mode bit 0 is MOSI and bit 1 is MISO
BT8XXEMU_API extern uint8_t BT8XXEMU_Flash_transferSpi4(BT8XXEMU_Flash *flash, uint8_t signal);

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* #ifndef BT8XXEMU_H */

/* end of file */
