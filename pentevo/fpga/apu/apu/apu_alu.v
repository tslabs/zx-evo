`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
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

	output wire	[31:0]	dst,
	output wire			fz,
	output wire			fs,
	output wire			fc,
	output wire			fv,
	
		);

	wire [31:0]	res[0:15];
	wire		zf[0:3];
	wire		sf[0:3];
	wire		cf[0:15];
	wire		vf[0:15];

	assign dst = res[func];
	assign fz = zf[sz];
	assign fs = sf[sz];
	assign fc = cf[func];
	assign fv = vf[func];

	wire [31:0] r_ld [0:3];
	wire [31:0] r_and[0:3];
	wire [31:0] r_or [0:3];
	wire [31:0] r_xor[0:3];
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
	wire [31:0] r_nul[0:3];
	wire [31:0] r_mul[0:3];
	
	wire c_ld [0:3];
	wire c_and[0:3];
	wire c_or [0:3];
	wire c_xor[0:3];
	wire c_add[0:3];
	wire c_sub[0:3];
	wire c_adc[0:3];
	wire c_sbc[0:3];
	wire c_rl [0:3];
	wire c_rr [0:3];
	wire c_rlc[0:3];
	wire c_rrc[0:3];
	wire c_sra[0:3];
	wire c_srz[0:3];
	wire c_nul[0:3];
	wire c_mul[0:3];
	
	wire v_ld [0:3];
	wire v_and[0:3];
	wire v_or [0:3];
	wire v_xor[0:3];
	wire v_add[0:3];
	wire v_sub[0:3];
	wire v_adc[0:3];
	wire v_sbc[0:3];
	wire v_rl [0:3];
	wire v_rr [0:3];
	wire v_rlc[0:3];
	wire v_rrc[0:3];
	wire v_sra[0:3];
	wire v_srz[0:3];
	wire v_nul[0:3];
	wire v_mul[0:3];
	
	assign zf [2'b00] = dst[ 7:0] == 8'b0;
	assign zf [2'b01] = dst[15:0] == 16'b0;
	assign zf [2'b10] = dst[23:0] == 24'b0;
	assign zf [2'b11] = dst[31:0] == 32'b0;

	assign sf [2'b00] = dst[ 7];
	assign sf [2'b01] = dst[15];
	assign sf [2'b10] = dst[23];
	assign sf [2'b11] = dst[31];

	assign cf[4'b0000] 	= c_ld [sz];			// LOAD
	assign cf[4'b0001] 	= c_and[sz];			// AND
	assign cf[4'b0010] 	= c_or [sz]; 			// OR
	assign cf[4'b0011] 	= c_xor[sz];			// XOR
	assign cf[4'b0100] 	= c_add[sz];			// ADD
	assign cf[4'b0101] 	= c_sub[sz];			// SUB
	assign cf[4'b0110] 	= c_adc[sz];			// ADC
	assign cf[4'b0111] 	= c_sbc[sz];			// SBC
	assign cf[4'b1000] 	= c_rl [sz]; 			// RL
	assign cf[4'b1001] 	= c_rr [sz];			// RR
	assign cf[4'b1010] 	= c_rlc[sz];			// RLC
	assign cf[4'b1011] 	= c_rrc[sz];			// RRC
	assign cf[4'b1100] 	= c_sra[sz];			// SRA
	assign cf[4'b1101] 	= c_srz[sz];			// SRZ
	assign cf[4'b1110] 	= c_nul[sz];			// 0
	assign cf[4'b1111] 	= c_mul[sz];			// MUL
		
	assign vf[4'b0000] 	= v_ld [sz];			// LOAD
	assign vf[4'b0001] 	= v_and[sz];			// AND
	assign vf[4'b0010] 	= v_or [sz]; 			// OR
	assign vf[4'b0011] 	= v_xor[sz];			// XOR
	assign vf[4'b0100] 	= v_add[sz];			// ADD
	assign vf[4'b0101] 	= v_sub[sz];			// SUB
	assign vf[4'b0110] 	= v_adc[sz];			// ADC
	assign vf[4'b0111] 	= v_sbc[sz];			// SBC
	assign vf[4'b1000] 	= v_rl [sz]; 			// RL
	assign vf[4'b1001] 	= v_rr [sz];			// RR
	assign vf[4'b1010] 	= v_rlc[sz];			// RLC
	assign vf[4'b1011] 	= v_rrc[sz];			// RRC
	assign vf[4'b1100] 	= v_sra[sz];			// SRA
	assign vf[4'b1101] 	= v_srz[sz];			// SRZ
	assign vf[4'b1110] 	= v_nul[sz];			// 0
	assign vf[4'b1111] 	= v_mul[sz];			// MUL

	assign res[4'b0000] = r_ld [sz];			// LOAD
	assign res[4'b0001] = r_and[sz];			// AND
	assign res[4'b0010] = r_or [sz]; 			// OR
	assign res[4'b0011] = r_xor[sz];			// XOR
	assign res[4'b0100] = r_add[sz];			// ADD
	assign res[4'b0101] = r_sub[sz];			// SUB
	assign res[4'b0110] = r_adc[sz];			// ADC
	assign res[4'b0111] = r_sbc[sz];			// SBC
	assign res[4'b1000] = r_rl [sz]; 			// RL
	assign res[4'b1001] = r_rr [sz];			// RR
	assign res[4'b1010] = r_rlc[sz];			// RLC
	assign res[4'b1011] = r_rrc[sz];			// RRC
	assign res[4'b1100] = r_sra[sz];			// SRA
	assign res[4'b1101] = r_srz[sz];			// SRZ
	assign res[4'b1110] = r_nul[sz];			// 0
	assign res[4'b1111] = r_mul[sz];			// MUL

// 0 - LOAD
	assign r_ld  [2'b00] = src;
	assign r_ld  [2'b01] = src;
	assign r_ld  [2'b10] = src;
	assign r_ld  [2'b11] = src;
                 
	assign c_ld  [2'b00] = 1'bX;
	assign c_ld  [2'b01] = 1'bX;
	assign c_ld  [2'b10] = 1'bX;
	assign c_ld  [2'b11] = 1'bX;
                 
	assign v_ld  [2'b00] = 1'bX;
	assign v_ld  [2'b01] = 1'bX;
	assign v_ld  [2'b10] = 1'bX;
	assign v_ld  [2'b11] = 1'bX;

// 1 - AND
	assign r_and [2'b00] = src && arg;
	assign r_and [2'b01] = src && arg;
	assign r_and [2'b10] = src && arg;
	assign r_and [2'b11] = src && arg;

	assign c_and [2'b00] = 1'bX;
	assign c_and [2'b01] = 1'bX;
	assign c_and [2'b10] = 1'bX;
	assign c_and [2'b11] = 1'bX;

	assign v_and [2'b00] = 1'bX;
	assign v_and [2'b01] = 1'bX;
	assign v_and [2'b10] = 1'bX;
	assign v_and [2'b11] = 1'bX;

// 2 - OR
	assign r_or [2'b00] = src || arg;
	assign r_or [2'b01] = src || arg;
	assign r_or [2'b10] = src || arg;
	assign r_or [2'b11] = src || arg;
             
	assign c_or [2'b00] = 1'bX;
	assign c_or [2'b01] = 1'bX;
	assign c_or [2'b10] = 1'bX;
	assign c_or [2'b11] = 1'bX;
             
	assign v_or [2'b00] = 1'bX;
	assign v_or [2'b01] = 1'bX;
	assign v_or [2'b10] = 1'bX;
	assign v_or [2'b11] = 1'bX;

// 3 - XOR
	assign r_xor [2'b00] = src ^^ arg;
	assign r_xor [2'b01] = src ^^ arg;
	assign r_xor [2'b10] = src ^^ arg;
	assign r_xor [2'b11] = src ^^ arg;

	assign c_xor [2'b00] = 1'bX;
	assign c_xor [2'b01] = 1'bX;
	assign c_xor [2'b10] = 1'bX;
	assign c_xor [2'b11] = 1'bX;

	assign v_xor [2'b00] = 1'bX;
	assign v_xor [2'b01] = 1'bX;
	assign v_xor [2'b10] = 1'bX;
	assign v_xor [2'b11] = 1'bX;

// 4 - ADD
	assign {c_add [2'b00], r_add [2'b00][ 7:0]} = src[ 7:0] + arg[ 7:0];
	assign {c_add [2'b01], r_add [2'b01][15:0]} = src[15:0] + arg[15:0];
	assign {c_add [2'b10], r_add [2'b10][23:0]} = src[23:0] + arg[23:0];
	assign {c_add [2'b11], r_add [2'b11][31:0]} = src[31:0] + arg[31:0];

	assign r_add [2'b00][31: 8] = 24'hXXXXXX;
	assign r_add [2'b01][31:16] = 16'hXXXX;
	assign r_add [2'b10][31:24] = 8'hXX;
	
	assign v_add [2'b00] = 1'bX;
	assign v_add [2'b01] = 1'bX;
	assign v_add [2'b10] = 1'bX;
	assign v_add [2'b11] = 1'bX;

// 5 - SUB
	assign {c_sub [2'b00], r_sub [2'b00][ 7:0]} = src[ 7:0] - arg[ 7:0];
	assign {c_sub [2'b01], r_sub [2'b01][15:0]} = src[15:0] - arg[15:0];
	assign {c_sub [2'b10], r_sub [2'b10][23:0]} = src[23:0] - arg[23:0];
	assign {c_sub [2'b11], r_sub [2'b11][31:0]} = src[31:0] - arg[31:0];

	assign r_sub [2'b00][31: 8] = 24'hXXXXXX;
	assign r_sub [2'b01][31:16] = 16'hXXXX;
	assign r_sub [2'b10][31:24] = 8'hXX;

	assign v_sub [2'b00] = 1'bX;
	assign v_sub [2'b01] = 1'bX;
	assign v_sub [2'b10] = 1'bX;
	assign v_sub [2'b11] = 1'bX;

// 6 - ADC
	assign {c_adc [2'b00], r_adc [2'b00][ 7:0]} = src[ 7:0] + arg[ 7:0] + c;
	assign {c_adc [2'b01], r_adc [2'b01][15:0]} = src[15:0] + arg[15:0] + c;
	assign {c_adc [2'b10], r_adc [2'b10][23:0]} = src[23:0] + arg[23:0] + c;
	assign {c_adc [2'b11], r_adc [2'b11][31:0]} = src[31:0] + arg[31:0] + c;

	assign r_adc [2'b00][31: 8] = 24'hXXXXXX;
	assign r_adc [2'b01][31:16] = 16'hXXXX;
	assign r_adc [2'b10][31:24] = 8'hXX;
	
	assign v_adc [2'b00] = 1'bX;
	assign v_adc [2'b01] = 1'bX;
	assign v_adc [2'b10] = 1'bX;
	assign v_adc [2'b11] = 1'bX;

// 7 - SBC
	assign {c_sbc [2'b00], r_sbc [2'b00][ 7:0]} = src[ 7:0] - arg[ 7:0] - c;
	assign {c_sbc [2'b01], r_sbc [2'b01][15:0]} = src[15:0] - arg[15:0] - c;
	assign {c_sbc [2'b10], r_sbc [2'b10][23:0]} = src[23:0] - arg[23:0] - c;
	assign {c_sbc [2'b11], r_sbc [2'b11][31:0]} = src[31:0] - arg[31:0] - c;

	assign r_sbc [2'b00][31: 8] = 24'hXXXXXX;
	assign r_sbc [2'b01][31:16] = 16'hXXXX;
	assign r_sbc [2'b10][31:24] = 8'hXX;

	assign v_sbc [2'b00] = 1'bX;
	assign v_sbc [2'b01] = 1'bX;
	assign v_sbc [2'b10] = 1'bX;
	assign v_sbc [2'b11] = 1'bX;

// 8 - RL
	assign r_rl  [2'b00][ 7:0] = {src[ 6:0], src[ 7]};
	assign r_rl  [2'b01][15:0] = {src[14:0], src[15]};
	assign r_rl  [2'b10][23:0] = {src[22:0], src[23]};
	assign r_rl  [2'b11][31:0] = {src[30:0], src[31]};

	assign r_rl  [2'b00][31: 8] = 24'hXXXXXX;
	assign r_rl  [2'b01][31:16] = 16'hXXXX;
	assign r_rl  [2'b10][31:24] = 8'hXX;

	assign c_rl  [2'b00] = src[ 7];
	assign c_rl  [2'b01] = src[15];
	assign c_rl  [2'b10] = src[23];
	assign c_rl  [2'b11] = src[31];

	assign v_rl  [2'b00] = 1'bX;
	assign v_rl  [2'b01] = 1'bX;
	assign v_rl  [2'b10] = 1'bX;
	assign v_rl  [2'b11] = 1'bX;

// 9 - RR
	assign r_rr  [2'b00][ 7:0] = {src[0], src[ 7:1]};
	assign r_rr  [2'b01][15:0] = {src[0], src[15:1]};
	assign r_rr  [2'b10][23:0] = {src[0], src[23:1]};
	assign r_rr  [2'b11][31:0] = {src[0], src[31:1]};

	assign r_rr  [2'b00][31: 8] = 24'hXXXXXX;
	assign r_rr  [2'b01][31:16] = 16'hXXXX;
	assign r_rr  [2'b10][31:24] = 8'hXX;

	assign c_rr  [2'b00] = src[0];
	assign c_rr  [2'b01] = src[0];
	assign c_rr  [2'b10] = src[0];
	assign c_rr  [2'b11] = src[0];

	assign v_rr  [2'b00] = 1'bX;
	assign v_rr  [2'b01] = 1'bX;
	assign v_rr  [2'b10] = 1'bX;
	assign v_rr  [2'b11] = 1'bX;

// 10 - RLC
	assign r_rlc [2'b00][ 7:0] = {src[ 6:0], c};
	assign r_rlc [2'b01][15:0] = {src[14:0], c};
	assign r_rlc [2'b10][23:0] = {src[22:0], c};
	assign r_rlc [2'b11][31:0] = {src[30:0], c};

	assign r_rlc [2'b00][31: 8] = 24'hXXXXXX;
	assign r_rlc [2'b01][31:16] = 16'hXXXX;
	assign r_rlc [2'b10][31:24] = 8'hXX;

	assign c_rlc [2'b00] = src[ 7];
	assign c_rlc [2'b01] = src[15];
	assign c_rlc [2'b10] = src[23];
	assign c_rlc [2'b11] = src[31];

	assign v_rlc [2'b00] = 1'bX;
	assign v_rlc [2'b01] = 1'bX;
	assign v_rlc [2'b10] = 1'bX;
	assign v_rlc [2'b11] = 1'bX;

// 11 - RRC
	assign r_rrc [2'b00][ 7:0] = {c, src[ 7:1]};
	assign r_rrc [2'b01][15:0] = {c, src[15:1]};
	assign r_rrc [2'b10][23:0] = {c, src[23:1]};
	assign r_rrc [2'b11][31:0] = {c, src[31:1]};

	assign r_rrc [2'b00][31: 8] = 24'hXXXXXX;
	assign r_rrc [2'b01][31:16] = 16'hXXXX;
	assign r_rrc [2'b10][31:24] = 8'hXX;

	assign c_rrc [2'b00] = src[0];
	assign c_rrc [2'b01] = src[0];
	assign c_rrc [2'b10] = src[0];
	assign c_rrc [2'b11] = src[0];

	assign v_rrc [2'b00] = 1'bX;
	assign v_rrc [2'b01] = 1'bX;
	assign v_rrc [2'b10] = 1'bX;
	assign v_rrc [2'b11] = 1'bX;

// 12 - SRA
	assign r_sra [2'b00][ 7:0] = {src[ 7], src[ 7:1]};
	assign r_sra [2'b01][15:0] = {src[15], src[15:1]};
	assign r_sra [2'b10][23:0] = {src[23], src[23:1]};
	assign r_sra [2'b11][31:0] = {src[31], src[31:1]};

	assign r_sra [2'b00][31: 8] = 24'hXXXXXX;
	assign r_sra [2'b01][31:16] = 16'hXXXX;
	assign r_sra [2'b10][31:24] = 8'hXX;

	assign c_sra [2'b00] = src[0];
	assign c_sra [2'b01] = src[0];
	assign c_sra [2'b10] = src[0];
	assign c_sra [2'b11] = src[0];

	assign v_sra [2'b00] = 1'bX;
	assign v_sra [2'b01] = 1'bX;
	assign v_sra [2'b10] = 1'bX;
	assign v_sra [2'b11] = 1'bX;

// 13 - SRZ
	assign r_srz [2'b00][ 7:0] = {1'b0, src[ 7:1]};
	assign r_srz [2'b01][15:0] = {1'b0, src[15:1]};
	assign r_srz [2'b10][23:0] = {1'b0, src[23:1]};
	assign r_srz [2'b11][31:0] = {1'b0, src[31:1]};

	assign r_srz [2'b00][31: 8] = 24'hXXXXXX;
	assign r_srz [2'b01][31:16] = 16'hXXXX;
	assign r_srz [2'b10][31:24] = 8'hXX;

	assign c_srz [2'b00] = src[0];
	assign c_srz [2'b01] = src[0];
	assign c_srz [2'b10] = src[0];
	assign c_srz [2'b11] = src[0];

	assign v_srz [2'b00] = 1'bX;
	assign v_srz [2'b01] = 1'bX;
	assign v_srz [2'b10] = 1'bX;
	assign v_srz [2'b11] = 1'bX;

// 14 - NUL
	assign r_nul [2'b00] = 32'hXXXXXXXX;
	assign r_nul [2'b01] = 32'hXXXXXXXX;
	assign r_nul [2'b10] = 32'hXXXXXXXX;
	assign r_nul [2'b11] = 32'hXXXXXXXX;
             
	assign c_nul [2'b00] = 1'bX;
	assign c_nul [2'b01] = 1'bX;
	assign c_nul [2'b10] = 1'bX;
	assign c_nul [2'b11] = 1'bX;
             
	assign v_nul [2'b00] = 1'bX;
	assign v_nul [2'b01] = 1'bX;
	assign v_nul [2'b10] = 1'bX;
	assign v_nul [2'b11] = 1'bX;

// 15 - MUL
	assign {c_mul [2'b00], r_mul [2'b00][ 7:0]} = src[ 7:0] * arg[ 7:0];
	assign {c_mul [2'b01], r_mul [2'b01][15:0]} = src[15:0] * arg[15:0];
	assign {c_mul [2'b10], r_mul [2'b10][23:0]} = src[23:0] * arg[23:0];
	assign {c_mul [2'b11], r_mul [2'b11][31:0]} = src[31:0] * arg[31:0];

	assign r_mul [2'b00][31: 8] = 24'hXXXXXX;
	assign r_mul [2'b01][31:16] = 16'hXXXX;
	assign r_mul [2'b10][31:24] = 8'hXX;

	assign v_mul [2'b00] = 1'bX;
	assign v_mul [2'b01] = 1'bX;
	assign v_mul [2'b10] = 1'bX;
	assign v_mul [2'b11] = 1'bX;

	
endmodule
