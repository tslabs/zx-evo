// This module generates video addresses


module video_cntr (

// clocks
	input wire clk,
	input wire c4,
	
// video controls
	input wire frame_start,
	input wire line_start,
	input wire vpix,
	
// video counters
	output reg [8:0] cnt_col,
	output reg [8:0] cnt_row,

// DRAM interface
	input  wire video_next
	
);

	
	wire [8:0] cstart = 0;
	wire [8:0] rstart = 0;


// counters
	always @(posedge clk)
		if (line_start)
			cnt_col <= cstart;
		else
		if (video_next)
			cnt_col <= cnt_col + 1;

	always @(posedge clk) if (c4)
		if (frame_start)
			cnt_row <=  rstart;
		else
		if (line_start & vpix)
			cnt_row <=  cnt_row + 1;
		


endmodule