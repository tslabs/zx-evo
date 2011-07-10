`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// APU ALU
//
// Written by TS-Labs inc.
//


module	apu_alu(

	input wire	[31:0]	arg0,
	input wire	[31:0]	arg1,
	input wire			c,
	input wire	[3:0]	func,
	input wire	[1:0]	sz,

	output reg	[31:0]	res,
	output reg			fc,
	output reg			fz,
	output wire			fs,
	output wire			fv
	
		);

	wire [31:0] src = arg0;		// just aliases
	wire [31:0] arg = arg1;

	
	// Flag C
	always @*
	casex (func)
4'b00XX:	fc = 1'b0;						// LOAD, AND, OR, XOR
4'b01XX:    fc = cx[sz];    				// ADD, ADC, SUB, SBC
4'b10XX:    fc = cr;	    				// RR, RRC, SRA, SRZ
4'b110X:    fc = cl[sz];    				// RL, RLC
4'b111X:    fc = fs;	    				// MUL, MULS
	endcase

	
	// Flag Z
	always @*
	case (sz)
2'b00:	fz = res[ 7:0] == 8'b0;
2'b01:	fz = res[15:0] == 16'b0;
2'b10:	fz = res[23:0] == 24'b0;
2'b11:	fz = res[31:0] == 32'b0;
	endcase


	// Result
	always @*
	casex (func)
4'b0000:	res = arg;						// LOAD
4'b0001:	res = src & arg;				// AND
4'b0010:    res = src | arg;				// OR
4'b0011:    res = src ^ arg;				// XOR
4'b010X:    res = r_add;					// ADD, ADC
4'b011X:    res = r_sub;					// SUB, SBC
4'b10XX:    res = r_rr;						// RR, RRC, SRA, SRZ
4'b110X:    res = r_rl;						// RL, RLC
4'b111X:    res = r_mul;					// MUL, MULS
// 4'b11XX:    res = r_rl;						// RL, RLC			// BLOCKER for muls!!!
	endcase
	
	
	// Adder, Subtractor
	wire [31:0] r_add;
	wire [31:0] r_sub;

	wire ca, cs;
	
	wire cx[0:3];
	assign cx[2'b00] = res[ 8];
	assign cx[2'b01] = res[16];
	assign cx[2'b10] = res[24];	
	assign cx[2'b11] = !func[1] ? ca : cs;

	wire [31:0] src_sz = {	sz > 2 ? src[31:24] : 8'b0,
							sz > 1 ? src[23:16] : 8'b0,
							sz > 0 ? src[15:8] : 8'b0,
							src[7:0] };

	wire [31:0] arg_sz = {	sz > 2 ? arg[31:24] : 8'b0,
							sz > 1 ? arg[23:16] : 8'b0,
							sz > 0 ? arg[15:8] : 8'b0,
							arg[7:0] };

	wire sc = c && func[0];			// ADD/ADC and SUB/SBC select
	// wire sc = 1'b0;				// BLOCKER for ADC, SBC

	
//  ADD, ADC
	assign {ca, r_add} = src_sz + arg_sz + sc;

	
//  SUB, SBC
	assign {cs, r_sub} = src_sz - arg_sz - sc;

	
	// Rotators
	wire [31:0] r_rl;
	wire [31:0] r_rr;
	
	wire cl[0:3];
	assign cl[2'b00] = arg[ 7];
	assign cl[2'b01] = arg[15];
	assign cl[2'b10] = arg[23];
	assign cl[2'b11] = arg[31];

	wire cr = arg[0];
	wire lc = !func[0] ?	cl[sz] : c;					// RL/RLC select
	wire rc = !func[1] ?	(!func[0] ? cr : c) :		// RR/RRC select
							(!func[0] ? cl[sz] : 1'b0); // SRA/SRZ select

	wire rc0 = (sz == 2'b00) ? rc : arg[ 8];
	wire rc1 = (sz == 2'b01) ? rc : arg[16];
	wire rc2 = (sz == 2'b10) ? rc : arg[24];
	
	
//  RL, RLC
	assign r_rl = {arg[30:0], lc};
	

// RR, RRC, SRA, SRZ
	assign r_rr = {rc, arg[31:25], rc2, arg[23:17], rc1, arg[15:9], rc0, arg[7:1]};
	

	// Multiplier
	wire [31:0] r_mul;
	wire [31:0] r_muls;

	wire [15:0] srcm[0:3];
	wire [15:0] argm[0:3];
	
	wire sg = func[0];
	// wire sg = 1'b0;			// BLOCKER for signed mul!!!
	
	wire sgs8  = src[ 7] && sg;
	wire sga8  = arg[ 7] && sg;
	wire sgs16 = src[15] && sg;
	wire sga16 = arg[15] && sg;
	
	wire [ 7:0] src8  = sgs8  ? 16'h0 - src[ 7:0] : src[ 7:0];
	wire [ 7:0] arg8  = sga8  ? 16'h0 - arg[ 7:0] : arg[ 7:0];
	wire [15:0] src16 = sgs16 ? 16'h0 - src[15:0] : src[15:0];
	wire [15:0] arg16 = sgs16 ? 16'h0 - arg[15:0] : arg[15:0];

	wire [3:0] sgr = (sz > 1 ? sgs16 : sgs8) ^ (sz > 2 ? sga16 : sga8);
	
	assign srcm = sz > 1 ? {8'b0, src8} : src16;
	assign argm = sz > 2 ? {8'b0, arg8} : arg16;


// MUL, MULS
	assign r_muls = srcm * argm;
	assign r_mul  = sgr ? 32'h0 - r_muls : r_muls;
	
	
endmodule
