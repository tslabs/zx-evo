`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Auxilliary Processor
//
// Written by TS-Labs inc.
//
// Version APU-1


module apu(

	input				clk,
	
	input				cbeg,
	input				post_cbeg,
	input				pre_cend,   
	input				cend,  
		
// APU
	input				apu_halt_in,
	output reg			apu_rdy,
	
// ZMAPS
	input				code_wen,
	input wire	[7:0]	code_wdata,
	input		[8:0]	code_waddr,

// DRAM
	output 		[20:0]	apu_addr,
	output 		[15:0]	apu_data_w,
	output wire	[15:0]	apu_data_r,
	output 				apu_req,
	output 				apu_dram_rnw,
	output 		[1:0]	apu_dram_bsel,
	input				apu_strobe,

// Ports
	output		[5:0]	apu_border,
	output				apu_border_we,		//must be sampled by fclk
	output		[7:0]	apu_covox,
	output				apu_covox_we,		//must be sampled by fclk
	
// Video
	input wire	[8:0]	hcnt,
	input wire	[8:0]	vcnt,
	input wire			hsync_start,
	input wire			line_start,
	input wire			vsync,
	input wire			hblank,
	input wire			vblank
	
	);

	wire sram_data_r = 16'd0;

	
//- Instructions decoding -------------------------------------

	localparam ALU_LD   = 4'b0000;
	localparam ALU_AND  = 4'b0001;
	localparam ALU_OR   = 4'b0010;
	localparam ALU_XOR  = 4'b0011;
	localparam ALU_ADD  = 4'b0100;
	localparam ALU_ADC  = 4'b0101;
	localparam ALU_SUB  = 4'b0110;
	localparam ALU_SBC  = 4'b0111;
	localparam ALU_RR   = 4'b1000;
	localparam ALU_RRC  = 4'b1001;
	localparam ALU_SRA  = 4'b1010;
	localparam ALU_SRZ  = 4'b1011;
	localparam ALU_RL   = 4'b1100;
	localparam ALU_RLC  = 4'b1101;
	localparam ALU_MUL  = 4'b1110;
	localparam ALU_MULS = 4'b1111;

	reg	 [ 7:0]	gpr[0:15];									// 16 8-bit regs
	

// ALU function decoding
	always @*
	casex (opcode[15:12])
4'b0x01:		alu_func = ALU_AND;	// and, tst
4'b0010:		alu_func = ALU_OR;	// or
4'b0011:		alu_func = ALU_XOR;	// xor
4'b0100:		alu_func = ALU_ADD;	// add
4'b0110:		alu_func = ALU_SUB;	// cmp

4'b0111:	casex(opcode[7:6])
	2'b00:		casex (opcode[11:8])
		4'b0000:	alu_func = ALU_ADD;	// inc
		4'b00x1:	alu_func = ALU_SUB;	// dec, neg
		4'b0010:	alu_func = ALU_XOR;	// cpl
		4'b0100:	alu_func = ALU_ADC;	// adc
		4'b0101:	alu_func = ALU_SBC;	// sbc
		4'b1000:	alu_func = ALU_RR;	// rr
		4'b1001:	alu_func = ALU_RRC;	// rrc
		4'b1010:	alu_func = ALU_SRA;	// sra
		4'b1011:	alu_func = ALU_SRZ;	// srz
		4'b11x0:	alu_func = ALU_RL;	// rl, rlt
		4'b11x1:	alu_func = ALU_RLC;	// rlc, rltc
		default:	alu_func = ALU_LD;
				endcase

	2'b1x:		alu_func = opcode[6] ? ALU_MUL : ALU_MULS;
	default:	alu_func = ALU_LD;
			endcase
			
4'b100x:	casex ({opcode[12], opcode[7:6]})
	3'bx01:		alu_func3 = ALU_AND;	// and, tst
	3'b010:		alu_func3 = ALU_OR;		// or
	3'b011:		alu_func3 = ALU_XOR;	// xor
	3'b100:		alu_func3 = ALU_ADD;	// add
	3'b11x:		alu_func3 = ALU_SUB;	// cmp, sub
	default:	alu_func3 = ALU_LD;	
			endcase

4'b1x10:	alu_func = ALU_SUB;		// wait, djnz
default:	alu_func = ALU_LD;
	endcase

	
// ALU size decoding
	always @*
	casex (opcode[15:12])
4'b00xx,
4'b010x,
4'b0110:	sz = 2'b0;			// ld, and, or, xor, add, tst, cmp

4'b0111:	if (opcode[11:10] == 2'b01 && opcode[7:6] == 2'b00)	sz = 2'b0;		// adc, sbc, swap, flip
			else if (opcode[7]) sz = mul_sz;	// mul, muls
			else sz = opcode[5:4];
				
4'b101x:	sz = pp_sz;			// wait, in, out
default:	sz = opcode[5:4];
	endcase


	wire [31:0] r_src = {gpr[src+3], gpr[src+2], gpr[src+1], gpr[src]};
	wire [31:0] r_dst = {gpr[dst+3], gpr[dst+2], gpr[dst+1], gpr[dst]};

// ALU argument0 decoding
	always @*
	casex (opcode[15:12])

4'b0111:	casex (opcode[11:6])
	6'b000x00:	alu_arg0 = r_src;	// inc, dec
	6'b001000:	alu_arg0 = r_src;	// cpl
	6'b001100:	alu_arg0 = 32'b0;	// neg
	default:	alu_arg0 = r_dst;
			endcase
			
4'b1010:	alu_arg0 = in_port;			// in
default:	alu_arg0 = r_dst;
	endcase


// ALU argument1 decoding
	always @*
	casex (opcode[15:12])
4'b00xx,
4'b010x,
4'b0110:	alu_arg1 = {24'b0, opcode[7:0]};	// imm8

4'b0111:	casex (opcode[11:6])
	6'b000x00:	alu_arg1 = 32'b1;		// inc, dec
	6'b001000:	alu_arg1 = {8{4'hF}};	// cpl
	6'b001100:	alu_arg1 = r_src;		// neg
	6'b01xx00:	alu_arg1 = 32'b0;		// adc, sbc, swap, flip
	default:	alu_arg1 = r_dst;
			endcase
			
4'b1110:	alu_arg1 = 32'b1;		//djnz
default:	alu_arg1 = r_src;
	endcase


// Result source decoding
	reg	 [31:0]	result;				// The result of execution

	always @*
	casex (opcode[15:12])

4'b0111:	casex (opcode[11:6])
	6'b011000:	result = {alu_res[31:24],
					r_src[3:0], r_src[7:4]
					};				// swap
	6'b011100:	result = {alu_res[31:24],
					r_src[0], r_src[1], r_src[2], r_src[3],
					r_src[4], r_src[5], r_src[6], r_src[7]
					};				// flip
	default:	result = alu_res;
			endcase
			
4'b1011:	result = in_port;		// in
default:	result = alu_res;
	endcase


// Destination address decoding
	reg	 [3:0]	res;

	wire src  = opcode[3:0]  & (sz == 2'b00 ? 4'b1111 :
								sz == 2'b01 ? 4'b1110 : 4'b1100 );
	wire dst  = opcode[11:8] & (sz == 2'b00 ? 4'b1111 :
								sz == 2'b01 ? 4'b1110 : 4'b1100 );

	always @*
	if ((opcode[15:12] == 4'b0111) && (opcode[7:6] == 2'b0)) res = src;
	else res = dst;


// Destination latch decoding
	reg	 [3:0]	dst_lat;									// Result latch enables
	wire r_lat = {sz > 2'd2, sz > 2'd1, sz > 2'd0, 1'b1};	// what regs to latch result into

	always @*
	casex (opcode[15:12])
4'b0101,
4'b0110,
4'b1010,
4'b110x,
4'b1111:	dst_lat = 4'b0;		// tst, cmp

4'b1001:	if (opcode[7:6] = 2'b01 || opcode[7:6] = 2'b10) dst_lat = 4'b0;		// tst, cmp
			else dst_lat = r_lat;

4'b1011:	if (opcode[7:6] = 2'b01 || opcode[7]) dst_lat = 4'b0;		// out
			else dst_lat = r_lat;

default:	dst_lat = r_lat;
	endcase

	
// Destination latching
	always @(posedge clk)
	begin
		if (dst_lat[0])		gpr[res]	<= result[ 7: 0];
		if (dst_lat[1])		gpr[res+1]	<= result[15: 8];
		if (dst_lat[2])		gpr[res+2]	<= result[23:16];
		if (dst_lat[3])		gpr[res+3]	<= result[24:31];
	end


// Flags 
	reg			c_reg, z_reg, s_reg, v_reg;		// Flags regs
	wire		c_fg, z_fg, s_fg, v_fg;			// Flags
	reg	 		c_lat, z_lat, s_lat, v_lat;		// Flags latch enables
	
	assign c_fg = c_lat ? alu_out_c : c_reg;
	assign z_fg = z_lat ? alu_out_z : z_reg;
	assign s_fg = s_lat ? alu_out_s : s_reg;
	assign v_fg = v_lat ? alu_out_v : v_reg;


// C flag latch decoding
	always @*
	casex (opcode[15:12])
4'b01x0:	c_lat = 1'b1;					// add, cmp

4'b0111:	casex (opcode[11:6])
	6'b0x0x00:	c_lat = 1'b1;			// inc, dec, adc, sbc
	6'b10xx00:	c_lat = 1'b1;			// rr, rrc, sra, srz
	6'b110x00:	c_lat = 1'b1;			// rl, rlc
	6'bxxxx1x:	c_lat = 1'b1;			// mul, muls
	default:	c_lat = 1'b0;
			endcase

4'b1001:	casex (opcode[7:6])
	2'b00:		c_lat = 1'b1;			// add
	2'b1x:		c_lat = 1'b1;			// cmp, sub
	default:	c_lat = 1'b0;
			endcase

default:	c_lat = 1'b0;
	endcase


// Z flag latch decoding
	always @*
	casex (opcode[15:12])
4'b0000,
4'b001x,
4'b010x,
4'b0110,
4'b1001:	z_lat = 1'b1;	// and, or, xor, add, tst, cmp
							// add, tst, cmp, sub (r,r)

4'b0111:	casex (opcode[11:6])
	6'b00xx00:	z_lat = 1'b1;			// inc, dec, cpl, neg
	6'b010x00:	z_lat = 1'b1;			// adc, sbc
	default:	z_lat = 1'b0;
			endcase

4'b1000:	casex (opcode[7:6])
	2'b01:		z_lat = 1'b1;			// and
	2'b1x:		z_lat = 1'b1;			// or, xor
	default:	z_lat = 1'b0;
			endcase

default:	z_lat = 1'b0;
	endcase

	
// Flags latching
	always @(posedge clk)
	begin
		if (c_lat)	c_reg <= alu_out_c;
		if (z_lat)	z_reg <= alu_out_z;
		if (s_lat)	s_reg <= alu_out_s;
		if (v_lat)	v_reg <= alu_out_v;
	end

	
// PC modification decoding
	reg  [15:0]	pc, pc_next;								// Program counter
	wire [15:0]	rel	= {{8{opcode[7]}}, opcode[7:0]};		// relative addr 16
	wire [15:0]	pc_inc = pc + 16'b1;						// Next PC
	wire [15:0]	pc_rel = pc + rel;							// PC for rel jump
	wire [ 0:3] stb = {cbeg, post_cbeg, pre_cend, cend};	// Synchro strobes
	wire ss = stb(opcode[7:6]);								// Synchro strobe select
	wire ic = pin_r ^ opcode[5];
	wire wc = jcnd[{2'b0, opcode[7:6]}];
	
	always @*
	casex (opcode[15:12])
4'b1010:	pc_next = wc ? pc_inc : pc;				// wait port
4'b1101:	pc_next = jc ? pc_rel : pc_inc;			// jr
4'b1110:	pc_next = !z_fg ? pc_rel : pc_inc;		// djnz

4'b1111:	casex (opcode[11:8])
	4'b1010:	pc_next = ic ? pc_inc : pc;			// wait pin
	4'b1011:	pc_next = ic && ss(opcode[7:6]) ? pc_inc : pc;	// wait ss pin
	4'b1101:	pc_next = jc ? {gpr[src+1], gpr[src]} : pc_inc; 	// jp
	4'b1111:	pc_next = pc; 						// halt
	default:	pc_next = pc_inc;
			endcase

default:	pc_next = pc_inc;
	endcase


// PC latching
	wire [15:0] pc_code = apu_halt ? 16'b0 : pc_next;
	
	always @(posedge clk)
		pc <= pc_code;


// Jump Condition decoding
	reg jc;

	always @*
	casex (opcode[15:12] == 4'b1101 ? opcode[11:8] : opcode[7:4])
4'b0000:	jc = z_fg;             	// 	equal, E, Z
4'b0001:	jc = c_fg;             	// 	less, L, C
4'b0010:	jc = !z_fg;            	// 	not equal, NE, NZ
4'b0011:	jc = !c_fg && !z_fg;   	// 	greater, G
4'b0100:	jc = c_fg || z_fg;     	// 	less of equal, LE
4'b0101:	jc = !c_fg;            	// 	greater or equal, GE, NC
4'b0110:	jc = s_fg;             	// 	negative, N
4'b0111:	jc = !s_fg;            	// 	positive, P
4'b1000:	jc = 1'b1;	          	// 	signed less or equal
4'b1001:	jc = 1'b1;	          	// 	signed greater
4'b1010:	jc = 1'b1;	          	// 	signed less
4'b1011:	jc = 1'b1;	          	// 	signed greater or equal
default:	jc = 1'b1;
	endcase

	
	wire [4:0]	pin0n	= opcode[4:0];					// pin0 5
	wire [4:0]	pin1n	= opcode[9:5];					// pin1 5
	
	
// Pins in decoding
	reg pin_r;
	
	always @*
	casex (opcode[4:0])
5'h00:		pin_r = timer_end;	
5'h01:		pin_r = apu_strobe;
5'h02:		pin_r = hsync_start;
5'h03:		pin_r = hblank;	   
5'h04:		pin_r = vsync;	   
5'h05:		pin_r = vblank;	   
5'h06:		pin_r = line_start;
default:	pin_r = 1'b1;
	endcase

	
// Ports in decoding
	reg [31:0] in_port;
	
	always @*
	casex (opcode[3:0])
4'h00:		in_port = {24'b0, sram_data_r	};
4'h01:		in_port = {16'b0, apu_data_r	};
4'h02:		in_port = {16'b0, timer_data_r	};
4'h03:		in_port = {23'b0, hcnt			};
4'h04:		in_port = {23'b0, vcnt			};
default:	in_port = 32'b0;
	endcase
	

// Ports in size decoding
	reg [3:0] pp_sz;
	
	always @*
	casex (opcode[3:0])
4'h00:		pp_sz = 2'b00;		// sram_data_r
4'h01:		pp_sz = 2'b01;		// apu_data_r
4'h02:		pp_sz = 2'b01;		// timer_data_r
4'h03:		pp_sz = 2'b01;		// hcnt
4'h04:		pp_sz = 2'b01;		// vcnt
default:	pp_sz = 2'b00;
	endcase
	

//- Ports control decoding ------------------------------------
	wire ld_port = (ih == io_port) && (opcode [7:6] == 2'b10);		// out (port), dst
	wire ld_pin1 = (ih == o_pin) && (opcode [7:6] == 2'b10);		// set/res pin0, set/res pin1
	wire ld_pin0 = ((ih == misc) && (im == m_o_pin)) || ld_pin1;	// set/res pin0


//- Ports out decoding and clocking ---------------------------
	localparam p_sram_addr		= 4'h0;	//data: reg, we: none
	localparam p_sram_data_w	= 4'h1;	//data: wire, we: wire
	localparam p_apu_addr		= 4'h2;	//data: outreg, we: none
	localparam p_apu_data_w		= 4'h3;	//data: outreg, we: none
	localparam p_timer_data_w	= 4'h4;	//data: wire, we: wire
	localparam p_apu_covox		= 4'h5;	//data: outwire, we: outwire
	localparam p_apu_border		= 4'h6;	//data: outwire, we: outwire
	
	reg [31:0] port_w [0:15];
	wire [1:0] psz_w[0:15];

	// sizes
	assign psz_w	[p_sram_addr]		= s16;
	assign psz_w	[p_sram_data_w]		= s8;
	assign psz_w	[p_apu_addr]		= s24;
	assign psz_w	[p_apu_data_w]		= s16;
	assign psz_w	[p_timer_data_w]	= s16;
	assign psz_w	[p_apu_covox]		= s8;
	assign psz_w	[p_apu_border]		= s8;
	
	// WEs - all should be sampled by fclk
	assign sram_we		= port == p_sram_data_w;
	assign timer_we		= port == p_timer_data_w;
	assign apu_covox_we	= port == p_apu_covox;
	assign apu_border_we	= port == p_apu_border;


//- Pins out decoding and clocking ----------------------------
	localparam p_apu_req		= 5'h0;	//data: outreg
	localparam p_apu_dram_bsel0	= 5'h1;	//data: outreg
	localparam p_apu_dram_bsel1	= 5'h2;	//data: outreg
	localparam p_apu_dram_rnw	= 5'h3;	//data: outreg

	reg pin_w[0:31];		//output pins
	
//	assign 		apu_req			= pin_w[p_apu_req];
//	assign [1:0]	apu_dram_bsel	= {pin_w[apu_dram_bsel1], pin_w[apu_dram_bsel0]};
//	assign		apu_dram_rnw	= pin_w[p_apu_dram_rnw];
	
	always @(posedge clk)
	begin
		if (ld_pin0)
			pin_w[pin0] <= bit0;

		if (ld_pin1)
			pin_w[pin1] <= bit1;
	end


// APU Halting
	reg apu_halt;
	
	always @(posedge clk)
		apu_halt <= apu_halt_in;

	always @(posedge clk)
		if (opcode[15:8] == 8'hFF)	apu_rdy <= 1'b1;
		else						apu_rdy <= 1'b0;
	
	wire [15:0] opcode = apu_halt ? = 16'hffff: opcode_r;

	
//- Write to APU microcode RAM --------------------------------
	reg [7:0] code_wdata_lo;
	wire code_wren = code_wen && code_waddr[0];
	wire [15:0] code_data = {code_wdata, code_wdata_lo};
	
	always @(posedge clk)
	if (code_wen && !code_waddr[0])
		code_wdata_lo <= code_wdata;
		

//- APU microcode RAM module ----------------------------------
	wire [15:0]	opcode;
		
	apu_code apu_code(
					.clock(clk),
					.wraddress(code_waddr[8:1]),
					.data(code_data),
					.wren(code_wren),
					.rdaddress(pc_code),
					.q(opcode_r)
					);


//- ALU module ------------------------------------------------
	reg		[3:0]	alu_func;
	reg		[1:0]	alu_sz;
	reg		[31:0]	alu_arg0;
	reg		[31:0]	alu_arg1;
	reg				alu_in_c;

	wire	[31:0]	alu_res;
	wire			alu_out_z;
	wire			alu_out_s;
	wire			alu_out_c;
	wire			alu_out_v;
	
	apu_alu(
					.func	(alu_func),
					.sz 	(alu_sz),
					.arg0	(alu_arg0),
					.arg1	(alu_arg1),
					.c	    (alu_in_c),
					
					.res	(alu_res)
					.fc	    (alu_out_c),
					.fz	    (alu_out_z),
					.fs	    (alu_out_s),
					.fv     (alu_out_v)
			);


//- Timer module ----------------------------------------------
	reg timer_wen;
	wire [15:0]	timer_data_r;
	wire timer_end;
	
	apu_timer apu_timer(
					.clk(clk),
					.wdata(timer_data_w),
					.wen(timer_we),
					.ctr(timer_data_r),
					.cnt_end(timer_end)
			);

			
endmodule

// just remark
// synthesis full_case