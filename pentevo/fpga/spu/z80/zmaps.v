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
	
// SPRAM/SFILE
	output [8:0] spsf_wa,
	output spsf_we

);

	parameter spsf	= 3'b101;
	
	assign fhit = (cpu_addr[21:14] == fp) && (cpu_addr[13:12] == fa);
	assign cpu_w = (cpu_req && ~cpu_rnw && ~fwd);

	assign spsf_we = (cpu_w && (cpu_addr[11:9] == spsf) && fhit);
	assign spsf_wa = cpu_addr[8:0];


endmodule
