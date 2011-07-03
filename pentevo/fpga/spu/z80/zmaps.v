`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// Z80 RAM to Altera Mappers
//
// (c)2011 TS-Labs
//

module zmaps(

	input cpu_req,
	input cpu_rnw,
	input [21:0] cpu_addr,
	
	input [7:0] fp,
	input [1:0] fa,
	input fwd,
	
//SPRAM
	output [8:0] sp_wa,
	output sp_we,

//SFile	
	output [8:0] sf_wa,
	output sf_we

//HFile	
	// output [8:0] hf_wa,
	// output hf_we,

//HVol
	// output [5:0] hv_wa,
	// output hv_we,
	//	output reg hus_en, li_en,
);

	parameter sfile = 3'b000;
	parameter spram = 3'b001;
	parameter hscrl0 = 3'b010;
	parameter hscrl1 = 3'b011;
	
	assign fhit = (cpu_addr[21:14] == fp) && (cpu_addr[13:12] == fa);
	assign cpu_w = (cpu_req && ~cpu_rnw && ~fwd);

	assign sf_we = (cpu_w && (cpu_addr[11:9] == sfile) && fhit);
	assign sf_wa = cpu_addr[8:0];

	assign sp_we = (cpu_w && (cpu_addr[11:9] == spram) && fhit);
	assign sp_wa = cpu_addr[8:0];


endmodule
