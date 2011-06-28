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
4'b00XX:	fc = 1'b0;						// LOAD, AND, OR, XOR
4'b01XX:    fc = cx[sz];    				// ADD, ADC, SUB, SBC
4'b10XX:    fc = cr;	    				// RR, RRC, SRA, SRZ
4'b110X:    fc = cl[sz];    				// RL, RLC
4'b111X:    fc = 1'b0;	    				// MUL, MULS
	endcase


	// Result
	always @*
	casex (func)
4'b0000:	res = src;						// LOAD
4'b0001:	res = src & arg;				// AND
4'b0010:    res = src | arg;				// OR
4'b0011:    res = src ^ arg;				// XOR
4'b010X:    res = r_add;					// ADD, ADC
4'b011X:    res = r_sub;					// SUB, SBC
4'b10XX:    res = r_rr;						// RR, RRC, SRA, SRZ
4'b110X:    res = r_rl;						// RL, RLC
4'b111X:    res = r_mul;					// MUL, MULS
	endcase
	
	
	// Adder, Subtractor
	wire [31:0] r_add;
	wire [31:0] r_sub;

	wire [31:0] srcs[0:3];
	wire [31:0] args[0:3];
	
	wire ca, cs;
	wire cx[0:3];

	assign srcs[2'b00] = {24'h000000, src[ 7:0]};
	assign srcs[2'b01] = {	16'h0000, src[15:0]};
	assign srcs[2'b10] = {	   8'h00, src[23:0]};
    assign srcs[2'b11] = {			  src[31:0]};

	assign args[2'b00] = {24'h000000, arg[ 7:0]};
	assign args[2'b01] = {	16'h0000, arg[15:0]};
	assign args[2'b10] = {	   8'h00, arg[23:0]};
    assign args[2'b11] = {		  	  arg[31:0]};
	
	assign cx[2'b00] = res[ 8];
	assign cx[2'b01] = res[16];
	assign cx[2'b10] = res[24];	
	assign cx[2'b11] = !func[1] ? ca : cs;

	wire sc = c && func[0];		// ADD/ADC and SUB/SBC select

	
//  ADD, ADC
	assign {ca, r_add} = srcs[sz] + args[sz] + sc;

	
//  SUB, SBC
	assign {cs, r_sub} = srcs[sz] - args[sz] - sc;

	
	// Rotators
	wire [31:0] r_rl;
	wire [31:0] r_rr;
	
	wire cl[0:3];

	assign cl[2'b00] = src[ 7];
	assign cl[2'b01] = src[15];
	assign cl[2'b10] = src[23];
	assign cl[2'b11] = src[31];

	wire cr = src[0];
	wire lc = !func[0] ?	cl[sz] : c;					// RL/RLC select
	wire rc = !func[1] ?	(!func[0] ? cr : c) :		// RR/RRC select
							(!func[0] ? cl[sz] : 1'b0); // SRA/SRZ select

	wire rc0 = (sz == 2'b00) ? rc : src[ 8];
	wire rc1 = (sz == 2'b01) ? rc : src[16];
	wire rc2 = (sz == 2'b10) ? rc : src[24];
	
	
//  RL, RLC
	assign r_rl = {src[30:0], lc};
	

// RR, RRC, SRA, SRZ
	assign r_rr = {rc, src[31:25], rc2, src[23:17], rc1, src[15:9], rc0, src[7:1]};
	

	// Multiplier
	wire [31:0] r_mul;

	wire [31:0] srcm[0:3];
	wire [31:0] argm[0:3];

	assign srcm[2'b00] = {24'h000000, src[ 7:0]};	//  8 =  8*8
	assign srcm[2'b01] = {24'h000000, src[ 7:0]};	// 16 =  8*8
	assign srcm[2'b10] = {	16'h0000, src[15:0]};   // 24 = 16*8
	assign srcm[2'b11] = {	16'h0000, src[15:0]};   // 32 = 16*16

	assign argm[2'b00] = {24'h000000, arg[ 7:0]};
	assign argm[2'b01] = {24'h000000, arg[ 7:0]};
	assign argm[2'b10] = {24'h000000, arg[ 7:0]};
	assign argm[2'b11] = {	16'h0000, arg[15:0]};
	

// MUL, MULS
	assign r_mul = srcm[sz] * argm[sz];
	
	
endmodule
