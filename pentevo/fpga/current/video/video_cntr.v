// This module generates video addresses


module video_cntr (

// clocks
	input wire clk,
	input wire c4,
	
// video config
	input wire [7:0] cstart,
    input wire [8:0] rstart,

// video controls
	input wire frame_start,
	input wire line_start,
	input wire vpix,
	
// video counters
	output reg [7:0] cnt_col,
	output reg [8:0] cnt_row,
	output reg cptr,

// DRAM interface
	input  wire video_next
	
);

	
// counters
	always @(posedge clk)
		if (line_start)
		begin
			cnt_col <= cstart;
			cptr <= 1'b0;
		end
		else
		if (video_next)
		begin
			cnt_col <= cnt_col + 1;
			cptr <= ~cptr;
		end

	always @(posedge clk) if (c4)
		if (frame_start)
			cnt_row <=  rstart;
		else
		if (line_start & vpix)
			cnt_row <=  cnt_row + 1;
		


endmodule