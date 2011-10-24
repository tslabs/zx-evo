// This module generates video addresses


module video_adr (

// clocks
	input wire clk,
	input wire c4,
	
// video controls
	input wire frame_start,
	input wire line_start,

// mode controls
	input wire [7:0] scr_page,
	input wire		 mode_zx,
	input wire		 mode_256c,
	input wire		 vpix,
	
// addresses
	output wire [21:0] addr_zx_gfx,
	output wire [21:0] addr_zx_atr,
	output wire [21:0] addr_256c,

// DRAM interface
	input  wire        video_next
	
);

	
	assign ccc = col; //!!!

	wire [8:0] cstart = 0;
	wire [8:0] rstart = 0;


// counters
	reg [8:0] col;
	reg [8:0] row;
	
	always @(posedge clk) if (c4)
		if (frame_start)
			row <=  rstart;
		else
		if (line_start & vpix)
			row <=  row + 1;
		
	always @(posedge clk) if (c4)
		if (line_start)
			col <= cstart;
		else
		if (video_next)
			col <= col + 1;

			
// address calculations
	assign addr_zx_gfx = {scr_page, 1'b0, row[7:6], row[2:0], row[5:3], col[4:0]};
	assign addr_zx_atr = {scr_page, 4'b0110, row[7:3], col[4:0]};
	assign addr_256c = {scr_page[7:4], row, col};


	
endmodule