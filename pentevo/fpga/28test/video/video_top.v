// This module is a video top-level

`include "../include/tune.v"


module video_top (

// clocks
	input wire clk,
	input wire f0, f1,
	input wire c0, c1, c2, c3,

// video DAC
	output wire	[1:0] vred,
	output wire	[1:0] vgrn,
	output wire	[1:0] vblu,

// video syncs
	output wire	hsync,
	output wire	vsync,
	output wire	csync,

// Z80 controls
	input wire [ 7:0] d,

// port write strobes
    input wire zborder_wr,

// video controls
	input wire vga_on

);


// video config
	wire tv_blank;

// counters
    wire [7:0] cnt_col;
    wire [8:0] cnt_row;
    wire [8:0] cnt_tp_row;
	wire cptr;
    wire [3:0] scnt;
	wire [8:0] lcount;

// synchro
	wire frame_start;
	wire line_start;
	wire pix_start;
	wire tv_pix_start;
    wire vga_pix_start;
	wire tspix_start;
	wire vga_blank;
	wire vga_line;

// video data
 	wire [7:0] border;
	wire [7:0] vplex;
	wire [7:0] vgaplex;

// VGA-line
	wire [9:0] vga_cnt_in;
	wire [9:0] vga_cnt_out;


	video_ports video_ports (
		.clk		  	(clk),
        .d              (d),
        .zborder_wr     (zborder_wr),
        .border         (border)
);

	video_sync video_sync (
		.clk			(clk),
		.f1				(f1),
		.c3				(c3),
		.hsync			(hsync),
		.vsync			(vsync),
		.csync			(csync),
		.tv_blank		(tv_blank),
		.vga_blank		(vga_blank),
		.vga_cnt_in		(vga_cnt_in),
		.vga_cnt_out	(vga_cnt_out),
		.lcount			(lcount),
		.vga_line		(vga_line),
		.frame_start	(frame_start),
		.line_start		(line_start),
		.int_start		(int_start)
);


	video_out video_out (
		.vga_on			(vga_on),
		.tv_blank 		(tv_blank),
		.vga_blank		(vga_blank),
	    .vplex_in		(border),
	    .vgaplex		(vgaplex),
		.vred			(vred),
	    .vgrn			(vgrn),
	    .vblu			(vblu)
);


// 2 lines * 512 pix * 8 bit (10x8) - used for VGA doubler
// (1 altdpram)
	video_vmem video_vmem(
		.clock		(clk),
		.wraddress	(vga_cnt_in),
		.data		(border),
		.wren		(c3),
	    .rdaddress	(vga_cnt_out),
	    .q			(vgaplex)
);


endmodule
