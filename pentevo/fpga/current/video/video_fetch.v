// This module fetches video data from DRAM


module video_fetch (

// clocks
	input wire clk,
	input wire c0,
	
// mode controls
	input wire mode_zx,
	input wire mode_256c,

// video controls
	input wire fetch,
	
// addresses
	input wire [21:0] addr_zx_gfx,
	input wire [21:0] addr_zx_atr,
	input wire [21:0] addr_256c,
	
// video data
	output  reg [31:0] data_out,
	
// DRAM interface
	input  wire        video_strobe,
	output wire [20:0] video_addr,
	input  wire [15:0] video_data,
	output wire [ 1:0] video_bw,
	output wire        video_go
		
);


	assign video_go = fetch;
	assign video_bw = 2'b00;
	assign video_addr = addr_zx;
	
	wire [20:0] addr_zx = addr_zx_gfx[0] ? addr_zx_gfx[21:1] : addr_zx_atr[21:1];
	
// fetching data
	always @(posedge clk)
		if (video_strobe)
			if (addr_zx_gfx[0])
				data_out[31:16] <= video_data;
			else
				data_out[15: 0] <= video_data;
	
	
	
	
endmodule