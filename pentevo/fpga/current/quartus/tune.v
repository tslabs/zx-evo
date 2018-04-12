`timescale 1ns/100ps

`ifdef MODEL_TECH
`define SIMULATE
`endif

//`define DRAMMEM_VERBOSE
//`define FETCH_VERBOSE

// `define FREE_IORQ       // for non-blocked by internal ports !IORQ

`define IDE_HDD         // for IDE HDD
// `define IDE_VDAC        // for VideoDAC instead of IDE
// `define IDE_VDAC2       // for VideoDAC2 instead of IDE

// `define XTR_FEAT        // extra features, in only IDEless version

// `define AUTO_INT     // auto-incremented Frame Interrpt

// `define FDR        // FDD Ripper version

// `define PENT_312    // for Pentagon 71680 tacts emulation with 312 video lines