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

	output wire	[31:0]	res,
	output wire			fz,
	output wire			fs,
	output wire			fc,
	output wire			fv
	
		);

	wire [31:0]	rs[0:15];
	wire		zf[0:3];
	wire		sf[0:3];
	wire		cf[0:15];
	wire		vf[0:15];

	assign res = rs[func];
	assign fz = zf[sz];
	assign fs = sf[sz];
	assign fc = cf[func];
	assign fv = vf[func];

	wire [31:0] r_ld ;
	wire [31:0] r_and;
	wire [31:0] r_or ;
	wire [31:0] r_xor;
	wire [31:0] r_add[0:3];
	wire [31:0] r_sub[0:3];
	wire [31:0] r_adc[0:3];
	wire [31:0] r_sbc[0:3];
	wire [31:0] r_rl [0:3];
	wire [31:0] r_rr [0:3];
	wire [31:0] r_rlc[0:3];
	wire [31:0] r_rrc[0:3];
	wire [31:0] r_sra[0:3];
	wire [31:0] r_srz[0:3];
	wire [31:0] r_nul;
	wire [31:0] r_mul[0:3];
	
	wire c_ld , v_ld ;
	wire c_and, v_and;
	wire c_or , v_or ;
	wire c_xor, v_xor;
	wire c_add, v_add;
	wire c_sub, v_sub;
	wire c_adc, v_adc;
	wire c_sbc, v_sbc;
	wire c_rl , v_rl ;
	wire c_rr , v_rr ;
	wire c_rlc, v_rlc;
	wire c_rrc, v_rrc;
	wire c_sra, v_sra;
	wire c_srz, v_srz;
	wire c_nul, v_nul;
	wire c_mul, v_mul;
	
	assign zf [2'b00] = res[ 7:0] == 8'b0;
	assign zf [2'b01] = res[15:0] == 16'b0;
	assign zf [2'b10] = res[23:0] == 24'b0;
	assign zf [2'b11] = res[31:0] == 32'b0;

	assign sf [2'b00] = res[ 7];
	assign sf [2'b01] = res[15];
	assign sf [2'b10] = res[23];
	assign sf [2'b11] = res[31];

	assign cf[4'b0000] 	= c_ld ;				// LOAD
	assign cf[4'b0001] 	= c_and;				// AND
	assign cf[4'b0010] 	= c_or ; 				// OR
	assign cf[4'b0011] 	= c_xor;				// XOR
	assign cf[4'b0100] 	= c_add;				// ADD
	assign cf[4'b0101] 	= c_sub;				// SUB
	assign cf[4'b0110] 	= c_adc;				// ADC
	assign cf[4'b0111] 	= c_sbc;				// SBC
	assign cf[4'b1000] 	= c_rl ;	 			// RL
	assign cf[4'b1001] 	= c_rr ;				// RR
	assign cf[4'b1010] 	= c_rlc;				// RLC
	assign cf[4'b1011] 	= c_rrc;				// RRC
	assign cf[4'b1100] 	= c_sra;				// SRA
	assign cf[4'b1101] 	= c_srz;				// SRZ
	assign cf[4'b1110] 	= c_nul;				// 0
	assign cf[4'b1111] 	= c_mul;				// MUL
		
	assign vf[4'b0000] 	= v_ld ;				// LOAD
	assign vf[4'b0001] 	= v_and;				// AND
	assign vf[4'b0010] 	= v_or ; 				// OR
	assign vf[4'b0011] 	= v_xor;				// XOR
	assign vf[4'b0100] 	= v_add;				// ADD
	assign vf[4'b0101] 	= v_sub;				// SUB
	assign vf[4'b0110] 	= v_adc;				// ADC
	assign vf[4'b0111] 	= v_sbc;				// SBC
	assign vf[4'b1000] 	= v_rl ;	 			// RL
	assign vf[4'b1001] 	= v_rr ;				// RR
	assign vf[4'b1010] 	= v_rlc;				// RLC
	assign vf[4'b1011] 	= v_rrc;				// RRC
	assign vf[4'b1100] 	= v_sra;				// SRA
	assign vf[4'b1101] 	= v_srz;				// SRZ
	assign vf[4'b1110] 	= v_nul;				// 0
	assign vf[4'b1111] 	= v_mul;				// MUL

	assign rs[4'b0000] = r_ld ;					// LOAD
	assign rs[4'b0001] = r_and;					// AND
	assign rs[4'b0010] = r_or ; 				// OR
	assign rs[4'b0011] = r_xor;					// XOR
	assign rs[4'b0100] = r_add[sz];				// ADD
	assign rs[4'b0101] = r_sub[sz];				// SUB
	assign rs[4'b0110] = r_adc[sz];				// ADC
	assign rs[4'b0111] = r_sbc[sz];				// SBC
	assign rs[4'b1000] = r_rl [sz];				// RL
	assign rs[4'b1001] = r_rr [sz];				// RR
	assign rs[4'b1010] = r_rlc[sz];				// RLC
	assign rs[4'b1011] = r_rrc[sz];				// RRC
	assign rs[4'b1100] = r_sra[sz];				// SRA
	assign rs[4'b1101] = r_srz[sz];				// SRZ
	assign rs[4'b1110] = r_nul;					// 0
	assign rs[4'b1111] = r_mul[sz];				// MUL

	wire ca[0:3];
	wire cs[0:3];
	wire cm[0:3];
	wire cl[0:3];
	wire cr;

	wire sc = c && func[1];
	
// 0 - LOAD
	assign r_ld  = src;
	assign c_ld  = 1'b0;
	assign v_ld  = 1'b0;

	
// 1 - AND
	assign r_and = src & arg;
	assign c_and = 1'b0;
	assign v_and = 1'b0;

	
// 2 - OR
	assign r_or = src | arg;
	assign c_or = 1'b0;
	assign v_or = 1'b0;

	
// 3 - XOR
	assign r_xor = src ^ arg;
	assign c_xor = 1'b0;
	assign v_xor = 1'b0;


// 4 - ADD
	assign {ca [2'b00], r_add [2'b00][ 7:0]} = src[ 7:0] + arg[ 7:0] + sc;
	assign {ca [2'b01], r_add [2'b01][15:0]} = src[15:0] + arg[15:0] + sc;
	assign {ca [2'b10], r_add [2'b10][23:0]} = src[23:0] + arg[23:0] + sc;
	assign {ca [2'b11], r_add [2'b11][31:0]} = src[31:0] + arg[31:0] + sc;
	assign r_add [2'b00][31: 8] = 24'h000000;
	assign r_add [2'b01][31:16] = 16'h0000;
	assign r_add [2'b10][31:24] = 8'h00;
	assign c_add = ca[sz];
	assign v_add = 1'b0;

	
// 5 - SUB
	assign {cs [2'b00], r_sub [2'b00][ 7:0]} = src[ 7:0] - arg[ 7:0] - sc;
	assign {cs [2'b01], r_sub [2'b01][15:0]} = src[15:0] - arg[15:0] - sc;
	assign {cs [2'b10], r_sub [2'b10][23:0]} = src[23:0] - arg[23:0] - sc;
	assign {cs [2'b11], r_sub [2'b11][31:0]} = src[31:0] - arg[31:0] - sc;
	assign r_sub [2'b00][31: 8] = 24'h000000;
	assign r_sub [2'b01][31:16] = 16'h0000;
	assign r_sub [2'b10][31:24] = 8'h00;
	assign c_sub = cs[sz];
	assign v_sub = 1'b0;

	
// 6 - ADC
	assign r_adc [2'b00] = r_add [2'b00];
	assign r_adc [2'b01] = r_add [2'b01];
	assign r_adc [2'b10] = r_add [2'b10];
	assign r_adc [2'b11] = r_add [2'b11];
	assign c_adc = ca[sz];
	assign v_adc = 1'b0;

	
// 7 - SBC
	assign r_sbc [2'b00] = r_sub [2'b00];
	assign r_sbc [2'b01] = r_sub [2'b01];
	assign r_sbc [2'b10] = r_sub [2'b10];
	assign r_sbc [2'b11] = r_sub [2'b11];
	assign c_sbc = cs[sz];
	assign v_sbc = 1'b0;


// Rotators
	assign cl  [2'b00] = src[ 7];
	assign cl  [2'b01] = src[15];
	assign cl  [2'b10] = src[23];
	assign cl  [2'b11] = src[31];
	assign cr = src[0];

	wire lc =	!func[1] ? cl[sz] : c;
	wire rc =	!func[2] ?	(!func[1] ? cr : c) :
							(!func[1] ? cl[sz] : 1'b0);


// 8 - RL
	assign r_rl  [2'b00] = {24'h000000,	src[ 6:0], lc};
	assign r_rl  [2'b01] = {16'h0000,	src[14:0], lc};
	assign r_rl  [2'b10] = {8'h00,		src[22:0], lc};
	assign r_rl  [2'b11] = {			src[30:0], lc};
	assign c_rl  = cl[sz];
	assign v_rl	 = 1'b0;

	
// 9 - RR
	assign r_rr  [2'b00] = {24'h000000,	rc, src[ 7:1]};
	assign r_rr  [2'b01] = {16'h0000,	rc, src[15:1]};
	assign r_rr  [2'b10] = {8'h00,		rc, src[23:1]};
	assign r_rr  [2'b11] = {			rc, src[31:1]};
	assign c_rr  = cr;
	assign v_rr  = 1'b0;
	
// 10 - RLC
	assign r_rlc [2'b00] = r_rl [2'b00];
	assign r_rlc [2'b01] = r_rl [2'b01];
	assign r_rlc [2'b10] = r_rl [2'b10];
	assign r_rlc [2'b11] = r_rl [2'b11];
	assign c_rlc = cl[sz];
	assign v_rlc = 1'b0;

	
// 11 - RRC
	assign r_rrc [2'b00] = r_rr [2'b00];
	assign r_rrc [2'b01] = r_rr [2'b01];
	assign r_rrc [2'b10] = r_rr [2'b10];
	assign r_rrc [2'b11] = r_rr [2'b11];
	assign c_rrc = cr;
	assign v_rrc  = 1'b0;

	
// 12 - SRA
	assign r_sra [2'b00] = r_rr [2'b00];
	assign r_sra [2'b01] = r_rr [2'b01];
	assign r_sra [2'b10] = r_rr [2'b10];
	assign r_sra [2'b11] = r_rr [2'b11];
	assign c_sra = cr;
	assign v_sra  = 1'b0;

	
// 13 - SRZ
	assign r_srz [2'b00] = r_rr [2'b00];
	assign r_srz [2'b01] = r_rr [2'b01];
	assign r_srz [2'b10] = r_rr [2'b10];
	assign r_srz [2'b11] = r_rr [2'b11];
	assign c_srz = cr;
	assign v_srz  = 1'b0;


// 14 - NUL
	assign r_nul = src;
	assign c_nul = 1'b0;
	assign v_nul = 1'b0;

	
// 15 - MUL
	assign {cm [2'b00], r_mul [2'b00][ 7:0]} = src[ 7:0] * arg[ 7:0];
	assign {cm [2'b01], r_mul [2'b01][15:0]} = src[15:0] * arg[15:0];
	assign {cm [2'b10], r_mul [2'b10][23:0]} = src[23:0] * arg[23:0];
	assign {cm [2'b11], r_mul [2'b11][31:0]} = src[31:0] * arg[31:0];
	assign r_mul [2'b00][31: 8] = 24'h000000;
	assign r_mul [2'b01][31:16] = 16'h0000;
	assign r_mul [2'b10][31:24] = 8'h00;
	assign c_mul = cm[sz];
	assign v_mul = 1'b0;

	
endmodule
