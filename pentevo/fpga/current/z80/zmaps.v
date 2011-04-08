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
	
//VSTP
	output [8:0] yt_wa,
	output yt_we,
	
//SPRAM
	output [8:0] sp_wa,
	output sp_we,

//SFILE
	output [8:0] sf_wa,
	output sf_we


	
);

	parameter hscrl0 = 3'b00x;		//not
	parameter hscrl1 = 3'b01x;		//used
	parameter vstp = 3'b100;
	parameter spram = 3'b101;
	parameter sfile = 3'b110;	
	
	assign fmhit = (cpu_addr[21:14] == fp) && (cpu_addr[13:12] == fa);
	assign cpu_w = (cpu_req && ~cpu_rnw && ~fwd);

	assign yt_we = (cpu_w && (cpu_addr[11:9] == vstp) && fmhit);
	assign sp_we = (cpu_w && (cpu_addr[11:9] == spram) && fmhit);
	assign sf_we = (cpu_w && (cpu_addr[11:9] == sfile) && fmhit);

	assign yt_wa = cpu_addr[8:0];
	assign sp_wa = cpu_addr[8:0];
	assign sf_wa = cpu_addr[8:0];


endmodule
