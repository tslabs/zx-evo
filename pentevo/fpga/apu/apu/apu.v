`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Auxilliary Processor
//
// Written by TS-Labs inc.
//


module apu(

	input				clk,
	
// APU
	input				apu_halt,
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

	
// to do
//
// * jc
// * wc
// * pins in
// pins out
// * port in
// port out
// flags
// instructions
// ALU
// * GRP sz
// port_r_sz


//- Operands decoding -----------------------------------------

	wire [3:0]	ih		= instr[15:12];					// instruction class 4
	wire [3:0]	im		= instr[11:8];					// instruction sub-class 4

	wire [2:0]	a1fh	= instr[14:12];					// ALU1 function class 4
	wire [2:0]	a1fm	= {instr[12], instr[7:6]};		// ALU1 function sub-class 4
	wire [1:0]	a2fm	= {instr[7:6]};					// ALU2 function sub-class 4
	wire [1:0]	a3fm	= {instr[5:4]};					// ALU3 function sub-class 4
	wire [2:0]	bfm		= {instr[8:6]};					// Barrel function sub-class 4

	wire [7:0]	imm		= instr[7:0];					// immediate 8

	wire [15:0]	rel		= {{8{instr[7]}}, instr[7:0]};	// relative addr 16
	
	wire [3:0]	src		= instr[3:0];					// source reg8 4
	wire [3:0]	dst		= instr[11:8];					// destination reg8 4

	wire [1:0]	sz		= instr[5:4];					// sizeness 2

	wire [3:0]	jc		= instr[11:8];					// jump condition 4
	wire [1:0]	wc		= instr[7:6];					// wait condition 2
	wire 		ic		= instr[11];					// inverse condition 1

	wire [4:0]	pin0	= instr[4:0];					// pin0 5
	wire [4:0]	pin1	= instr[9:5];					// pin1 5
	wire [1:0]	stb		= instr[7:6];					// strobe signal 2


//- Instructions decoding -------------------------------------
	wire inst[0:15]
	
	generate
		genvar i;
		for(i=0;i<16;i=i+1)
		assign inst[i] = ih == i;
	endgenerate


//- ALU arguments decoding ------------------------------------
// input:	ih 4
// output:	ih_arg1 - 1st argument (dst)
// 			ih_arg1 - 2nd argument (src)
// used in:	ALU1, ALU2, ALU3, Barrel

	wire [31:0] ih_arg1[0:15];		// first argument for main class
	wire [31:0] ih_arg2[0:15];		// second argument for main class
	
	assign ih_arg1[4'b0000] = 32'hXXXX;
	assign ih_arg1[4'b0001] = dst_sz[sz];
	assign ih_arg1[4'b0010] = dst_sz[sz];
	assign ih_arg1[4'b0011] = dst_sz[sz];
	assign ih_arg1[4'b0100] = dst_sz[sz];
	assign ih_arg1[4'b0101] = dst_sz[sz];
	assign ih_arg1[4'b0110] = dst_sz[sz];
	assign ih_arg1[4'b0111] = dst_sz[sz];
	assign ih_arg1[4'b1000] = dst_sz[sz];
	assign ih_arg1[4'b1001] = dst_sz[sz];
	assign ih_arg1[4'b1010] = 32'hXXXX;
	assign ih_arg1[4'b1011] = port_r_sz[sz];
	assign ih_arg1[4'b1100] = 32'hXXXX;
	assign ih_arg1[4'b1101] = 32'hXXXX;
	assign ih_arg1[4'b1110] = dst_sz[sz];
	assign ih_arg1[4'b1111] = 32'hXXXX;

	assign ih_arg2[4'b0000] = {24'b0, imm};
	assign ih_arg2[4'b0001] = {24'b0, imm};
	assign ih_arg2[4'b0010] = {24'b0, imm};
	assign ih_arg2[4'b0011] = {24'b0, imm};
	assign ih_arg2[4'b0100] = {24'b0, imm};
	assign ih_arg2[4'b0101] = {24'b0, imm};
	assign ih_arg2[4'b0110] = {24'b0, imm};
	assign ih_arg2[4'b0111] = {24'b0, imm};
	assign ih_arg2[4'b1000] = src_sz[sz];
	assign ih_arg2[4'b1001] = src_sz[sz];
	assign ih_arg2[4'b1010] = 32'hXXXX;
	assign ih_arg2[4'b1011] = src_sz[sz];
	assign ih_arg2[4'b1100] = 32'hXXXX;
	assign ih_arg2[4'b1101] = 32'hXXXX;
	assign ih_arg2[4'b1110] = dst_sz[sz];
	assign ih_arg2[4'b1111] = src_sz[sz];


//- GPR Sizeness decoding -------------------------------------
// input:	sz 2
// output:	src_sz, dst_sz - number of 1st GPR in cluster
// used in:	ALU arguments
// notice:	used to aline gpr pairs and quads, just to minimize number of muxes

	wire [3:0] src_sz[0:3];
	wire [3:0] dst_sz[0:3];
	
	assign src_sz[2'b00] = src;
	assign src_sz[2'b01] = {src[3:1], 1'b0};
	assign src_sz[2'b10] = {src[3:2], 2'b0};
	assign src_sz[2'b11] = {src[3:2], 2'b0};

	assign dst_sz[2'b00] = dst;
	assign dst_sz[2'b01] = {dst[3:1], 1'b0};
	assign dst_sz[2'b10] = {dst[3:2], 2'b0};
	assign dst_sz[2'b11] = {dst[3:2], 2'b0};


// input:	sz 2
// output:	gpr_src_sz - "src" instruction argument, coerced to 

	wire [31:0] gpr_src_sz[0:3];
	
	assign gpr_src_sz[2'b00] = {24'b0, gpr[src8]};
	assign gpr_src_sz[2'b01] = {16'b0, gpr[src16+1], gpr[src16]};
	assign gpr_src_sz[2'b10] = {8'b0, gpr[src32+2], gpr[src32+1], gpr[src32]};
	assign gpr_src_sz[2'b11] = {gpr[src32+3], gpr[src32+2], gpr[src32+1], gpr[src32]};


	wire [31:0] gpr_dst_sz[0:3];
	
	assign gpr_dst_sz[2'b00] = {24'b0, gpr[dst8]};
	assign gpr_dst_sz[2'b01] = {16'b0, gpr[dst16+1], gpr[dst16]};
	assign gpr_dst_sz[2'b10] = {8'b0, gpr[dst32+2], gpr[dst32+1], gpr[dst32]};
	assign gpr_dst_sz[2'b11] = {gpr[dst32+3], gpr[dst32+2], gpr[dst32+1], gpr[dst32]};


//- ALU1 fuctions ---------------------------------------------
	wire [31:0] afunc1[0:7];

	assign afunc1[3'b000] = ih_arg2[ih];					// ld
	assign afunc1[3'b001] = ih_arg1[ih] & ih_arg2[ih];		// and
	assign afunc1[3'b010] = ih_arg1[ih] | ih_arg2[ih];		// or
	assign afunc1[3'b011] = ih_arg1[ih] ^ ih_arg2[ih];		// xor
	assign afunc1[3'b100] = ih_arg1[ih] + ih_arg2[ih];		// add
	assign afunc1[3'b101] = ih_arg1[ih] - ih_arg2[ih];		// sub
	assign afunc1[3'b110] = ih_arg1[ih] - ih_arg2[ih];		// cmp
	assign afunc1[3'b111] = ih_arg1[ih] & ih_arg2[ih];		// tst

	
//- ALU2 fuctions ---------------------------------------------
	wire [31:0] afunc2[0:3];

	assign afunc2[2'b00] = ih_arg2[ih] + 32'b1;				// inc
	assign afunc2[2'b01] = ih_arg2[ih] - 32'b1;				// dec
	assign afunc2[2'b10] = ih_arg2[ih] ^ 32'hFFFF;			// cpl
	assign afunc2[2'b11] = - ih_arg2[ih];					// neg

	
//- ALU3 fuctions ---------------------------------------------
	wire [7:0] afunc3[0:3];

	assign afunc3[2'b00] = gpr[src8] + {7'b0, f_c};			// adc
	assign afunc3[2'b01] = gpr[src8] - {7'b0, f_c};			// sbc
	assign afunc3[2'b10] = {gpr[src8][3:0],
							gpr[src8][7:4]};				// swap
	assign afunc3[2'b11] = {gpr[src8][0], gpr[src8][1],
							gpr[src8][2], gpr[src8][3],
							gpr[src8][4], gpr[src8][5],
							gpr[src8][6], gpr[src8][7]};	// flip

	
//- Barrel fuctions ---------------------------------------------
	wire [31:0] bfunc[0:15];


//- Jump Condition decoding -----------------------------------
	wire jcond[0:15];

	assign jcond[4'b0000] = 1'b1;				//always
	assign jcond[4'b0001] = _z;             	//equal, Z
	assign jcond[4'b0010] = _c;             	//less, C
	assign jcond[4'b0011] = !_z;            	//not equal, !Z
	assign jcond[4'b0100] = !_c && !_z;     	//greater, !C&!Z
	assign jcond[4'b0101] = _c || _z;       	//less of equal, C|Z
	assign jcond[4'b0110] = !_c;            	//greater or equal, !C
	assign jcond[4'b0111] = _n;             	//negative, N
	assign jcond[4'b1000] = !_n;            	//positive, !N
	assign jcond[4'b1001] = 1'b1;               //signed less
	assign jcond[4'b1010] = 1'b1;               //signed less or equal
	assign jcond[4'b1011] = 1'b1;               //signed greater
	assign jcond[4'b1100] = 1'b1;               //signed greater or equal
	assign jcond[4'b1101] = _h;             	//half carry, H
	assign jcond[4'b1110] = !_h;            	//not half carry, !H
	assign jcond[4'b1111] = 1'b0;           	//never


//- Wait Condition decoding -----------------------------------
	wire wcond[0:3];

	assign wcond[2'b00] = cbeg;
	assign wcond[2'b01] = post_cbeg;
	assign wcond[2'b10] = pre_cend;       
	assign wcond[2'b11] = cend;      


//- Pins in decoding ------------------------------------------
	wire pin_r[0:31];
	
	assign pin_r[5'h00] = 1'b1;		
	assign pin_r[5'h01] = timer_end;
	assign pin_r[5'h02] = apu_strobe;
	assign pin_r[5'h03] = hsync_start;
	assign pin_r[5'h04] = hblank;
	assign pin_r[5'h05] = vsync;
	assign pin_r[5'h06] = vblank;
	assign pin_r[5'h07] = line_start;
	assign pin_r[5'h08] = 1'bX;
	assign pin_r[5'h09] = 1'bX;
	assign pin_r[5'h0A] = 1'bX;
	assign pin_r[5'h0B] = 1'bX;
	assign pin_r[5'h0C] = 1'bX;
	assign pin_r[5'h0D] = 1'bX;
	assign pin_r[5'h0E] = 1'bX;
	assign pin_r[5'h0F] = 1'bX;
	assign pin_r[5'h10] = 1'bX;		
	assign pin_r[5'h11] = 1'bX;
	assign pin_r[5'h12] = 1'bX;
	assign pin_r[5'h13] = 1'bX;
	assign pin_r[5'h14] = 1'bX;
	assign pin_r[5'h15] = 1'bX;
	assign pin_r[5'h16] = 1'bX;
	assign pin_r[5'h17] = 1'bX;
	assign pin_r[5'h18] = 1'bX;
	assign pin_r[5'h19] = 1'bX;
	assign pin_r[5'h1A] = 1'bX;
	assign pin_r[5'h1B] = 1'bX;
	assign pin_r[5'h1C] = 1'bX;
	assign pin_r[5'h1D] = 1'bX;
	assign pin_r[5'h1E] = 1'bX;
	assign pin_r[5'h1F] = 1'bX;


//- Ports in decoding ------------------------------------------
	wire [31:0] port_r[0:15];
	
	assign port_r[4'h00] = { 24'b0, sram_data_r		};	// sram_data_r
	assign port_r[4'h01] = { 16'b0, apu_data_r		};	// apu_data_r
	assign port_r[4'h02] = { 16'b0, timer_data_r	};	// timer_data_r
	assign port_r[4'h03] = { 16'b0, hcnt			};	// hcnt
	assign port_r[4'h04] = { 16'b0, vcnt			};	// vcnt
	assign port_r[4'h05] = { 32'b0 					};	// 
	assign port_r[4'h06] = { 32'b0 					};	// 
	assign port_r[4'h07] = { 32'b0 					};	// 
	assign port_r[4'h08] = { 32'b0 					};	// 
	assign port_r[4'h09] = { 32'b0 					};	// 
	assign port_r[4'h0A] = { 32'b0 					};	// 
	assign port_r[4'h0B] = { 32'b0 					};	// 
	assign port_r[4'h0C] = { 32'b0 					};	// 
	assign port_r[4'h0D] = { 32'b0 					};	// 
	assign port_r[4'h0E] = { 32'b0 					};	// 
	assign port_r[4'h0F] = { 32'b0 					};	// 
	
	
	wire [15:0] port_r_e[0:15];
	
//						reg	=	  3210	   3210		3210	 3210
//						sz	=	  3333	   2222		1111	 0000
	assign port_r_e[4'h00] = { 4'b0001, 4'b0001, 4'b0001, 4'b0001 };	// sram_data_r
	assign port_r_e[4'h01] = { 4'b0011, 4'b0011, 4'b0010, 4'b0001 };	// apu_data_r
	assign port_r_e[4'h02] = { 4'b0011, 4'b0011, 4'b0010, 4'b0001 };	// timer_data_r
	assign port_r_e[4'h03] = { 4'b0011, 4'b0011, 4'b0010, 4'b0001 };	// hcnt
	assign port_r_e[4'h04] = { 4'b0011, 4'b0011, 4'b0010, 4'b0001 };	// vcnt
	assign port_r_e[4'h05] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h06] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h07] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h08] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h09] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0A] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0B] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0C] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0D] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0E] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 
	assign port_r_e[4'h0F] = { 4'b0000, 4'b0000, 4'b0000, 4'b0000 };	// 

	
//- PC handling -----------------------------------------------
	wire pc = {r[15], r[14]};
	wire pc_inc = pc + 16'b1;
	wire pc_rel = pc + rel;
	reg [15:0] pc_next;

	always @*
	if (apu_halt)
		pc_next = 16'b0;
	else
	case (ih)

wt_pin:		// wait	cond pin0 mask pin1
		pc_next = ~cond ^^ (~mask ? pin[pin0] || pin[pin1] : pin[pin0] && pin[pin1]) ? pc : pc_inc;

wt_port:	// wait sz (port) wc src
		begin
		end

jr:			// jr jc, rel8
		pc_next = jcond[jc] ? pc_rel : pc_inc;

misc:		// halt and stall
		if (instr[11:1] == 11'b11111111111)
			pc_next = pc;
		else
			pc_next = pc_inc;

default:
		pc_next = pc_inc;
	
	endcase


//- Ports control decoding ------------------------------------
	wire ld_port = (ih == io_port) && (instr [7:6] == 2'b10);		// out (port), dst
	wire ld_pin1 = (ih == o_pin) && (instr [7:6] == 2'b10);			// set/res pin0, set/res pin1
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

	// wires
//	assign [7:0]	sram_data_w		= gpr[dst8];
//	assign [15:0]	timer_data_w	= {gpr[dst16+1], r[dst16]};
//	assign [7:0]	apu_covox		= gpr[dst8];
//	assign [5:0]	apu_border		= gpr[dst8][5:0];

	// regs
//	assign [15:0] sram_addr		= port_w[p_sram_addr][15:0];
//	assign [20:0] apu_addr		= port_w[p_apu_addr][21:1];
//	assign [15:0] apu_data_w		= port_w[p_apu_data_w][15:0];

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

	always @(posedge clk)
	begin
		if (ld_port)
		case (psz_w)
	s8:
			port_w[port][7:0] <= r[dst];
	s16:
			port_w[port][15:0] <= {r[dst16+1], r[dst16]};
	s24,
	s32:
			port_w[port][31:0] <= {r[dst32+3], r[dst32+2], r[dst32+1], r[dst32]};
		endcase
	end


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


//- PC clocking -----------------------------------------------
	always @(posedge clk)
		{r[15], r[14]} <= pc_next;


//- Registers, flags clocking ---------------------------------
	reg _c, _z, _n;			// flags
	reg [7:0] r[0:15];		// GPRs
	reg z, c, n;			// flags
	
	always @(posedge clk)
	begin
		z <= _z;
		c <= _c;
		n <= _n;
		
		if (ld_dst)
			r[dst] <= res;	// RESTRICTION! Should be SZ added!
	end
	

//- APU Halting -----------------------------------------------
	always @*
	if (apu_halt)
		instr = 16'hfffe;
	else
		instr = instr_r;

	
//- Ready signal handling -------------------------------------
	always @(posedge clk)
	if (instr == 16'hffff)
		apu_rdy <= 1'b1;
	else
		apu_rdy <= 1'b0;


//- Write to APU microcode RAM --------------------------------
	reg [7:0] code_wdata_lo;
	wire code_wren = code_wen && code_waddr[0];
	wire [15:0] code_data = {code_wdata, code_wdata_lo};
	
	always @(posedge clk)
	if (code_wen && !code_waddr[0])
		code_wdata_lo <= code_wdata;
		

//- APU microcode RAM module ----------------------------------
	wire [15:0]	instr;
		
	apu_code apu_code(
					.clock(clk),
					.wraddress(code_waddr[8:1]),
					.data(code_data),
					.wren(code_wren),
					.rdaddress(pc_next),
					.q(instr_r)
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
