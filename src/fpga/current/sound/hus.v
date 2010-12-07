`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Hummer Ultra Sound
//
// Written by TS-Labs inc.
// ver. 0.0

module	hus(

	input clk,
		
//dram
	output reg [20:0] hus_addr,
	input [15:0] hus_data,
	output reg hus_req,
	input hus_strobe, hus_next,
	
	
//sfile	
	output wire [8:0] hf_ra,
	input [7:0] hf_rd
	
	);
	

	
	
//State Machine

	reg [4:0] dac_n;
	reg [4:0] mc;
	
	localparam mc_res = 5'd0;
	localparam mc_r1 = 5'd0;
	localparam mc_r2 = 5'd0;
	localparam mc_r3 = 5'd0;
	
	

	
	
endmodule
