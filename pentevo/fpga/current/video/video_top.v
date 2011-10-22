// This module generates all video part


module video_top (

// clocks
	input wire clk,
	input wire q0,
	input wire c0,
	input wire c1,
	input wire c2,
	input wire c3,
	input wire c15,

// video DAC
	output wire	[1:0] vred,
	output wire	[1:0] vgrn,
	output wire	[1:0] vblu,

// video syncs
	output wire	hsync,
	output wire	vsync,
	output wire	csync,

// ZX ports
	input wire [7:0] border,
	input wire [7:0] scr_page,
	
// ZX controls
	output wire int_start,

// DRAM interface
	input  wire        video_strobe,
	input  wire        video_next,
	output wire [20:0] video_addr,
	input  wire [15:0] video_data,
	output wire [ 1:0] video_bw,
	output wire        video_go, 
	
// controls
	input wire vga_on
	
);


// !!! under construction drafts !!!
	assign video_bw = 2'b00;
	assign video_go = 0;
	assign vred = 0;
	assign vgrn = 0;
	assign vblu = 0;

	
// instantiations

	video_sync video_sync (
		.clk		(clk		),
		.q0			(q0			),
		.c15		(c15		),
		.hsync		(hsync		),
		.vsync		(vsync		),
		.csync		(csync		),
		.int_start	(int_start	)
	);
	
	
endmodule
