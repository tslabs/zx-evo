`timescale 1ns/100ps

`ifdef MODEL_TECH
`define SIMULATE
`endif

// IDE modes, select ONE
// `define IDE_HDD     // for IDE HDD
// `define IDE_VDAC    // for VideoDAC instead of IDE
`define IDE_VDAC2      // for VideoDAC2 instead of IDE

`define ESP32_SPI      // ESP32-S3 SPI (VDAC3), requires IDE_VDAC2

// SPI modes, select ONE
// `define SPI_14            // SCK 14 MHz
`define SPI_14_MISO_LATE  // SCK 14 MHz, MISO fetch is 1/2 clk28 later
// `define SPI_28            // SCK 28 MHz

`define KEMPSTON_8BIT  // 8-bit enhanced Kempston Joystick interface
`define XTR_FEAT       // extra features, in only IDEless version
// `define COPPER      // enable Copper
// `define DISABLE_TSU // disable TSU
// `define SD_CARD2    // for second SD Card
// `define AUTO_INT    // auto-incremented Frame Interrpt
// `define FDR         // FDD Ripper version (use with DISABLE_TSU)
// `define PENT_312    // for Pentagon 71680 tacts emulation with 312 video lines
// `define FREE_IORQ   // for non-blocked by internal ports !IORQ
