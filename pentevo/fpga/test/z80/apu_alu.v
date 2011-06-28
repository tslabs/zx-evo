`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// APU ALU
//
// Written by TS-Labs inc.
//


module	apu_alu(

	input wire	[31:0]	src,
	input wire	[31:0]	arg,
	input wire			c,
	input wire	[3:0]	func,
	input wire	[1:0]	sz,

	output reg	[31:0]	res,
	output wire			fz,
	output wire			fs,
	output reg			fc,
	output wire			fv
	
		);


	wire		zf[0:3];
	wire		sf[0:3];

	assign fz = zf[sz];
	assign fs = sf[sz];
	assign fv = 1'b0;

	wire [31:0] r_ld ;
	wire [31:0] r_and;
	wire [31:0] r_or ;
	wire [31:0] r_xor;
	wire [31:0] r_add[0:3];
	wire [31:0] r_sub[0:3];
	wire [31:0] r_rl [0:3];
	wire [31:0] r_rr [0:3];
	wire [31:0] r_mul[0:3];
	
	wire ca[0:3];
	wire cs[0:3];
	wire cl[0:3];
	wire cm[0:3];

	assign zf [2'b00] = res[ 7:0] == 8'b0;
	assign zf [2'b01] = res[15:0] == 16'b0;
	assign zf [2'b10] = res[23:0] == 24'b0;
	assign zf [2'b11] = res[31:0] == 32'b0;

	assign sf [2'b00] = res[ 7];
	assign sf [2'b01] = res[15];
	assign sf [2'b10] = res[23];
	assign sf [2'b11] = res[31];

	
	// Adder, Subtractor
	wire sc = c && func[0];		// ADD/ADC and SUB/SBC select

	// Rotators
	wire cr = src[0];
	assign cl  [2'b00] = src[ 7];
	assign cl  [2'b01] = src[15];
	assign cl  [2'b10] = src[23];
	assign cl  [2'b11] = src[31];

	wire lc =	!func[0] ?	cl[sz] : c;					// RL/RLC select
	wire rc =	 func[1] ?	(!func[0] ? cr : c) :		// RR/RRC select
							(!func[0] ? cl[sz] : 1'b0); // SRA/SRZ select

	
	always @*
	case (func)
4'b0000:	res = r_ld ;					// LOAD
4'b0001:	res = r_and;					// AND
4'b0010:    res = r_or ; 					// OR
4'b0011:    res = r_xor;					// XOR
4'b0100,									// ADD
4'b0101:    res = r_add[sz];				// ADC
4'b0110,    								// SUB
4'b0111:    res = r_sub[sz];				// SBC
4'b1000,									// RL
4'b1001:    res = r_rl [sz];				// RLC
4'b1010,    								// RR
4'b1011,    								// RRC
4'b1100,    								// SRA
4'b1101:    res = r_rr [sz];				// SRZ
4'b1110,    								// MUL
4'b1111:    res = r_mul[sz];				// MUL
	endcase

	
	always @*
	case (func)
4'b0000:	fc = 1'b0;					// LOAD
4'b0001:	fc = 1'b0;					// AND
4'b0010:    fc = 1'b0; 					// OR
4'b0011:    fc = 1'b0;					// XOR
4'b0100,								// ADD
4'b0101:    fc = ca[sz];				// ADC
4'b0110,    							// SUB
4'b0111:    fc = cs[sz];				// SBC
4'b1000,								// RL
4'b1001:    fc = cl[sz];				// RLC
4'b1010,    							// RR
4'b1011,    							// RRC
4'b1100,    							// SRA
4'b1101:    fc = cr;					// SRZ
4'b1110,    							// MUL
4'b1111:    fc = cm[sz];				// MUL
	endcase

	
// 0 - LOAD
	assign r_ld  = src;

	
// 1 - AND
	assign r_and = src & arg;

	
// 2 - OR
	assign r_or = src | arg;

	
// 3 - XOR
	assign r_xor = src ^ arg;

	
// 4, 5 - ADD, ADC
	assign {ca [2'b00], r_add [2'b00][ 7:0]} = src[ 7:0] + arg[ 7:0] + sc;
	assign {ca [2'b01], r_add [2'b01][15:0]} = src[15:0] + arg[15:0] + sc;
	assign {ca [2'b10], r_add [2'b10][23:0]} = src[23:0] + arg[23:0] + sc;
	assign {ca [2'b11], r_add [2'b11][31:0]} = src[31:0] + arg[31:0] + sc;
	assign r_add [2'b00][31: 8] = 24'h000000;
	assign r_add [2'b01][31:16] = 16'h0000;
	assign r_add [2'b10][31:24] = 8'h00;

	
// 6, 7 - SUB, SBC
	assign {cs [2'b00], r_sub [2'b00][ 7:0]} = src[ 7:0] - arg[ 7:0] - sc;
	assign {cs [2'b01], r_sub [2'b01][15:0]} = src[15:0] - arg[15:0] - sc;
	assign {cs [2'b10], r_sub [2'b10][23:0]} = src[23:0] - arg[23:0] - sc;
	assign {cs [2'b11], r_sub [2'b11][31:0]} = src[31:0] - arg[31:0] - sc;
	assign r_sub [2'b00][31: 8] = 24'h000000;
	assign r_sub [2'b01][31:16] = 16'h0000;
	assign r_sub [2'b10][31:24] = 8'h00;

	
// 8, 9 - RL, RLC
	assign r_rl  [2'b00] = {24'h000000,	src[ 6:0], lc};
	assign r_rl  [2'b01] = {16'h0000,	src[14:0], lc};
	assign r_rl  [2'b10] = {8'h00,		src[22:0], lc};
	assign r_rl  [2'b11] = {			src[30:0], lc};

	
// 10, 11, 12, 13 - RR, RRC, SRA, SRZ
	assign r_rr  [2'b00] = {24'h000000,	rc, src[ 7:1]};
	assign r_rr  [2'b01] = {16'h0000,	rc, src[15:1]};
	assign r_rr  [2'b10] = {8'h00,		rc, src[23:1]};
	assign r_rr  [2'b11] = {			rc, src[31:1]};
	

// 14, 15 - MUL
	assign {cm [2'b00], r_mul [2'b00][ 7:0]} = src[ 7:0] * arg[ 7:0];
	assign {cm [2'b01], r_mul [2'b01][15:0]} = src[15:0] * arg[15:0];
	assign {cm [2'b10], r_mul [2'b10][23:0]} = src[23:0] * arg[23:0];
	assign {cm [2'b11], r_mul [2'b11][31:0]} = src[31:0] * arg[31:0];
	assign r_mul [2'b00][31: 8] = 24'h000000;
	assign r_mul [2'b01][31:16] = 16'h0000;
	assign r_mul [2'b10][31:24] = 8'h00;

	
endmodule
