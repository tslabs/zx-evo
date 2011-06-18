`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// Z80 RAM to Altera Mappers
//
// (c)2011 TS-Labs
//

module zmaps(

	input 			cpu_req,
	input 			cpu_rnw,
	input [21:0]	cpu_addr,
	
	input [7:0]		zmaps_page,
	input [1:0]		zmaps_addr,
	input 			zmaps_wr_en,
	input			mem_wr_fclk,
	
	output			ys_tp_we,
	output			sf_sp_we,
	output			apu_code_we
	
);

	assign zmaps_hit = (cpu_addr[21:14] == zmaps_page) && (cpu_addr[13:12] == zmaps_addr);
	assign cpu_w = (cpu_req && ~cpu_rnw && zmaps_wr_en);

	assign ys_tp_we = (cpu_w && (cpu_addr[11:9] == 3'b100) && zmaps_hit);		// YScrolls + Tile Palette
	assign sf_sp_we = (cpu_w && (cpu_addr[11:9] == 3'b101) && zmaps_hit);		// SPU File + Palette
	assign apu_code_we = (cpu_w && (cpu_addr[11:9] == 3'b110) && zmaps_hit);	// APU Code

	
endmodule
