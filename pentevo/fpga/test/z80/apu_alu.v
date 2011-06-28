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
	output reg			fz,
	output reg			fs,
	output reg			fc,
	output reg			fv
	
		);


	wire [31:0] r_add[0:3];
	wire [31:0] r_sub[0:3];
	wire [31:0] r_rl [0:3];
	wire [31:0] r_rr [0:3];
	wire [31:0] r_mul[0:3];
	
	wire ca[0:3];
	wire cs[0:3];
	wire cl[0:3];


	// Flag Z
	always @*
	case (sz)
2'b00:	fz = res[ 7:0] == 8'b0;
2'b01:	fz = res[15:0] == 16'b0;
2'b10:	fz = res[23:0] == 24'b0;
2'b11:	fz = res[31:0] == 32'b0;
	endcase

	
	// Flag S
	always @*
	case (sz)
2'b00:	fs = res[ 7];
2'b01:	fs = res[15];
2'b10:	fs = res[23];
2'b11:	fs = res[31];
	endcase


	// Flag V
	always @*
	case (sz)
2'b00:	fv = 1'b0;
2'b01:	fv = 1'b0;
2'b10:	fv = 1'b0;
2'b11:	fv = 1'b0;
	endcase


	// Flag C
	always @*
	casex (func)
4'b0000:	fc = 1'b0;						// LOAD
4'b0001:	fc = 1'b0;	    				// AND
4'b0010:    fc = 1'b0; 	    				// OR
4'b0011:    fc = 1'b0;	    				// XOR
4'b010X:    fc = ca[sz];    				// ADD, ADC
4'b011X:    fc = cs[sz];    				// SUB, SBC
4'b10XX:    fc = cr;	    				// RR, RRC, SRA, SRZ
4'b110X:    fc = cl[sz];    				// RL, RLC
4'b111X:    fc = 1'b0;	    				// MUL, MULS
	endcase


	always @*
	casex (func)
4'b0000:	res = src;						// LOAD
4'b0001:	res = src & arg;				// AND
4'b0010:    res = src | arg;				// OR
4'b0011:    res = src ^ arg;				// XOR
4'b010X:    res = r_add[sz];				// ADD, ADC
4'b011X:    res = r_sub[sz];				// SUB, SBC
4'b10XX:    res = r_rr [sz];				// RR, RRC, SRA, SRZ
4'b110X:    res = r_rl [sz];				// RL, RLC
4'b111X:    res = r_mul[sz];				// MUL, MULS
	endcase
	
	
	// Adder, Subtractor
	wire sc = c && func[0];		// ADD/ADC and SUB/SBC select

	
	// Rotators
	assign cl  [2'b00] = src[ 7];
	assign cl  [2'b01] = src[15];
	assign cl  [2'b10] = src[23];
	assign cl  [2'b11] = src[31];
	wire cr = src[0];
	wire lc =	!func[0] ?	cl[sz] : c;					// RL/RLC select
	wire rc =	!func[1] ?	(!func[0] ? cr : c) :		// RR/RRC select
							(!func[0] ? cl[sz] : 1'b0); // SRA/SRZ select

	
//  ADD, ADC
	assign {ca [2'b00], r_add [2'b00][ 7:0]} = src[ 7:0] + arg[ 7:0] + sc;
	assign {ca [2'b01], r_add [2'b01][15:0]} = src[15:0] + arg[15:0] + sc;
	assign {ca [2'b10], r_add [2'b10][23:0]} = src[23:0] + arg[23:0] + sc;
	assign {ca [2'b11], r_add [2'b11][31:0]} = src[31:0] + arg[31:0] + sc;
	assign r_add [2'b00][31: 8] = 24'h000000;
	assign r_add [2'b01][31:16] = 16'h0000;
	assign r_add [2'b10][31:24] = 8'h00;

	
//  SUB, SBC
	assign {cs [2'b00], r_sub [2'b00][ 7:0]} = src[ 7:0] - arg[ 7:0] - sc;
	assign {cs [2'b01], r_sub [2'b01][15:0]} = src[15:0] - arg[15:0] - sc;
	assign {cs [2'b10], r_sub [2'b10][23:0]} = src[23:0] - arg[23:0] - sc;
	assign {cs [2'b11], r_sub [2'b11][31:0]} = src[31:0] - arg[31:0] - sc;
	assign r_sub [2'b00][31: 8] = 24'h000000;
	assign r_sub [2'b01][31:16] = 16'h0000;
	assign r_sub [2'b10][31:24] = 8'h00;

	
// RR, RRC, SRA, SRZ
	assign r_rr  [2'b00][ 7:0] = {			rc, src[ 7:1]};
	assign r_rr  [2'b01][15:0] = {			rc, src[15:1]};
	assign r_rr  [2'b10][23:0] = {			rc, src[23:1]};
	assign r_rr  [2'b11][31:0] = {			rc, src[31:1]};
	assign r_rr  [2'b00][31: 8] = 24'h000000;
	assign r_rr  [2'b01][31:16] = 16'h0000;
	assign r_rr  [2'b10][31:24] = 8'h00;
	

//  RL, RLC
	assign r_rl  [2'b00][ 7:0] = {			src[ 6:0], lc};
	assign r_rl  [2'b01][15:0] = {			src[14:0], lc};
	assign r_rl  [2'b10][23:0] = {			src[22:0], lc};
	assign r_rl  [2'b11][31:0] = {			src[30:0], lc};
	assign r_rl  [2'b00][31: 8] = 24'h000000;
	assign r_rl  [2'b01][31:16] = 16'h0000;
	assign r_rl  [2'b10][31:24] = 8'h00;

	
// MUL, MULS
	assign {r_mul [2'b00][ 7:0]} = src[ 7:0] * arg[ 7:0];
	assign {r_mul [2'b01][15:0]} = src[ 7:0] * arg[ 7:0];
	assign {r_mul [2'b10][23:0]} = src[15:0] * arg[15:0];
	assign {r_mul [2'b11][31:0]} = src[15:0] * arg[15:0];
	assign r_mul [2'b00][31: 8] = 24'h000000;
	assign r_mul [2'b01][31:16] = 16'h0000;
	assign r_mul [2'b10][31:24] = 8'h00;

	
endmodule
