`timescale 1ns/100ps

`ifdef MODEL_TECH
`define SIMULATE
`endif

`define IDE_VDAC2      // for VideoDAC2 instead of IDE
`define ESP32_SPI      // ESP32-S3 SPI (VDAC3), requires IDE_VDAC2
`define XTR_FEAT       // extra features, in only IDEless version
`define PENT_312       // 312 lines per frame
