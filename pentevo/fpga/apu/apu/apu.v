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


	reg _c, _z, _n;			//flags

	
//- Instructions and operands decoding ------------------------
	localparam ld_imm	= 4'h0;
	localparam and_imm	= 4'h1;
	localparam or_imm	= 4'h2;
	localparam xor_imm	= 4'h3;
	localparam add_imm	= 4'h4;
	localparam sub_imm	= 4'h5;
	localparam log1		= 4'h6;
	localparam log2		= 4'h7;
	localparam wt_pin	= 4'h8;
	localparam o_pin	= 4'h9;
	localparam wt_port	= 4'hA;
	localparam io_port	= 4'hB;
	localparam out_imm	= 4'hC;
	localparam jr		= 4'hD;
	localparam djnz		= 4'hE;
	localparam misc		= 4'hF;

	localparam m_log	= 4'h0;
	localparam m_shft1	= 4'h1;
	localparam m_shft2	= 4'h2;
//	localparam xor_imm	= 4'h3;
//	localparam add_imm	= 4'h4;
//	localparam sub_imm	= 4'h5;
//	localparam log1		= 4'h6;
//	localparam log2		= 4'h7;
	localparam m_bit	= 4'h8;
	localparam m_o_pin	= 4'h9;
//	localparam wt_port	= 4'hA;
//	localparam port_io	= 4'hB;
//	localparam out_imm	= 4'hC;
	localparam m_jp		= 4'hD;
	localparam m_mul	= 4'hE;
	localparam m_misc	= 4'hF;

	localparam s8 	= 2'b00;
	localparam s16 	= 2'b01;
	localparam s24 	= 2'b10;
	localparam s32 	= 2'b11;

	wire [3:0] ih = instr[15:12];	// instruction class
	wire [3:0] im = instr[11:8];	// instruction sub-class
	
	wire [7:0] imm = instr[7:0];	// immediate 8
	wire [4:0] pin0 = instr[4:0];	// pin0 5
	wire [4:0] pin1 = instr[9:5];	// pin1 5
	wire [3:0] src = instr[3:0];	// source reg8 4
	wire [3:0] src16 = {instr[3:1], 1'b0};		// source reg16 4
	wire [3:0] src32 = {instr[3:2], 2'b0};		// source reg32 4
	wire [3:0] dst = instr[11:8];	// destination reg8 4
	wire [3:0] dst16 = {instr[11:9], 1'b0};		// destination reg16 4
	wire [3:0] dst32 = {instr[11:10], 2'b0};	// destination reg32 4
	wire [3:0] port = instr[3:0];	// source port8 4
	wire [3:0] jc = instr[11:8];	// jump condition 4
	wire [2:0] jc_m = instr[7:4];	// jump condition in miscs 4
	wire [2:0] bit = instr[6:4];	// bit select 3
	wire [1:0] sz = instr[5:4];		// sizeness 2
	wire [1:0] wc = instr[7:6];		// wait condition 2
	wire bit0 = instr[10];			// bit0 value
	wire bit1 = instr[11];			// bit1 value
	wire cond = instr[11];			// condition
	wire mask = instr[10];			// mask
	wire flag = instr[7];			// flag select

	reg [15:0] rel;
	
	always @*
	case (ih)		// synthesis full_case 

jr:		// jr jc, rel9
		rel = {8{instr[7]}, instr[7:0]};

djnz:	// djnz src, rel8
		rel = {8{instr[11]}, instr[11:4]};

	endcase

	
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


//- Jump Condition decoding -----------------------------------
	reg jcond;

	localparam	c_always		= 4'b0000;	//always
	localparam	c_equal			= 4'b0001;	//equal, Z
	localparam	c_less			= 4'b0010;	//less, C
	localparam	c_nequal		= 4'b0011;	//not equal, !Z
	localparam	c_greater		= 4'b0100;	//greater, !C&!Z
	localparam	c_lessequal		= 4'b0101;	//less of equal, C|Z
	localparam	c_greaterequal	= 4'b0110;	//greater or equal, !C
	localparam	c_negative		= 4'b0111;	//negative, N
	localparam	c_positive		= 4'b1000;	//positive, !N
	localparam	c_sless			= 4'b1001;	//signed less
	localparam	c_slessequal	= 4'b1010;	//signed less or equal
	localparam	c_sgreater		= 4'b1011;	//signed greater
	localparam	c_sgreaterequal	= 4'b1100;	//signed greater or equal
	localparam	c_hcarry		= 4'b1101;	//half carry, H
	localparam	c_nhcarry		= 4'b1110;	//not half carry, !H
	localparam	c_never			= 4'b1111;	//never
                
	wire jcond[0:15]

	assign jcond[c_always		 ] = 1'b1;
	assign jcond[c_equal		 ] = _z;
	assign jcond[c_less		 	 ] = _c;
	assign jcond[c_nequal		 ] = !_z;
	assign jcond[c_greater		 ] = !_c && !_z;
	assign jcond[c_lessequal	 ] = _c || _z;
	assign jcond[c_greaterequal  ] = !_c;
	assign jcond[c_negative	 	 ] = _n;
	assign jcond[c_positive	 	 ] = !_n;
	assign jcond[c_sless		 ] = 
	assign jcond[c_slessequal	 ] = 
	assign jcond[c_sgreater	 	 ] = 
	assign jcond[c_sgreaterequal ] = 
	assign jcond[c_hcarry		 ] = _h;
	assign jcond[c_nhcarry		 ] = !_h;
	assign jcond[c_never		 ] = 1'b0;


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
	wire [7:0]	sram_data_w		= r[dst];
	wire [15:0]	timer_data_w	= {r[dst16+1], r[dst16]};
	wire [7:0]	apu_covox		= r[dst];
	wire [5:0]	apu_border		= r[dst][5:0];

	// regs
	wire [15:0] sram_addr		= port_w[p_sram_addr][15:0];
	wire [20:0] apu_addr		= port_w[p_apu_addr][21:1];
	wire [15:0] apu_data_w		= port_w[p_apu_data_w][15:0];

	// sizes
	assign psz_w	[p_sram_addr]		= s16;
	assign psz_w	[p_sram_data_w]		= s8;
	assign psz_w	[p_apu_addr]		= s24;
	assign psz_w	[p_apu_data_w]		= s16;
	assign psz_w	[p_timer_data_w]	= s16;
	assign psz_w	[p_apu_covox]		= s8;
	assign psz_w	[p_apu_border]		= s8;
	
	// WEs - all should be sampled by fclk
	wire sram_we		= port == p_sram_data_w;
	wire timer_we		= port == p_timer_data_w;
	wire apu_covox_we	= port == p_apu_covox;
	wire apu_border_we	= port == p_apu_border;

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


//- Ports-in decoding -----------------------------------------
	localparam p_sram_data_r	= 4'h0;
	localparam p_apu_data_r		= 4'h1;
	localparam p_timer_data_r	= 4'h2;
	localparam p_hcnt			= 4'h3;
	localparam p_vcnt			= 4'h4;
	
	wire [31:0] port_r[0:15];
	wire [1:0] psz_r[0:15];

	assign port_r	[p_sram_data_r 	][7:0]	= sram_data_r;
	assign port_r	[p_apu_data_r  	][15:0]	= apu_data_r;
	assign port_r	[p_timer_data_r	][15:0]	= timer_data_r;
	assign port_r	[p_hcnt		   	][15:0]	= hcnt;
	assign port_r	[p_vcnt		   	][15:0]	= vcnt;

	assign psz_r	[p_sram_data_r 	]		= s8;
	assign psz_r	[p_apu_data_r  	]		= s16;
	assign psz_r	[p_timer_data_r	]		= s16;
	assign psz_r	[p_hcnt		   	]		= s16;
	assign psz_r	[p_vcnt		   	]		= s16;


//- Pins out decoding and clocking ----------------------------
	localparam p_apu_req		= 5'h0;	//data: outreg
	localparam p_apu_dram_bsel0	= 5'h1;	//data: outreg
	localparam p_apu_dram_bsel1	= 5'h2;	//data: outreg
	localparam p_apu_dram_rnw	= 5'h3;	//data: outreg

	reg pin_w[0:31];		//output pins
	
	wire 		apu_req			= pin_w[p_apu_req];
	wire [1:0]	apu_dram_bsel	= {pin_w[apu_dram_bsel1], pin_w[apu_dram_bsel0]};
	wire		apu_dram_rnw	= pin_w[p_apu_dram_rnw];
	
	always @(posedge clk)
	begin
		if (ld_pin0)
			pin_w[pin0] <= bit0;

		if (ld_pin1)
			pin_w[pin1] <= bit1;
	end


//- Pins in decoding ------------------------------------------
	localparam p_timer_end	= 5'h0;
	localparam p_apu_strobe	= 5'h1;
	localparam p_hsync_start= 5'h2;
	localparam p_hblank		= 5'h3;
	localparam p_vsync		= 5'h4;
	localparam p_vblank		= 5'h5;
	localparam p_cbeg		= 5'h6;
	localparam p_post_cbeg	= 5'h7;
	localparam p_pre_cend	= 5'h8;
	localparam p_cend		= 5'h9;

	wire p_in[0:31];		//input pins
	
	wire 		apu_req			= p[p_apu_req];
	wire [1:0]	apu_dram_bsel	= {p[apu_dram_bsel1], p[apu_dram_bsel0]};
	wire		apu_dram_rnw	= p[p_apu_dram_rnw];
	
	always @(posedge clk)
	begin
		if (ld_pin0)
			p[pin0] <= bit0;

		if (ld_pin1)
			p[pin1] <= bit1;
	end

	
//- PC clocking -----------------------------------------------
	always @(posedge clk)
		{r[15], r[14]} <= pc_next;


//- Registers, flags clocking ---------------------------------
	reg [7:0] r[0:15];		// GPRs
	reg z, c, n;			// flags
	
	always @(posedge clk)
	begin
		z <= _z;
		c <= _c;
		n <= _n;
		
		if (ld_dst)
			r[dst] <= res;	// RESTRICTION! Should be SZ added!
	

//- APU Halting -----------------------------------------------
	wire [15:0] instr;

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
					
	apu_timer apu_timer(
					.clk(clk),
					.wdata(timer_data_w),
					.wen(timer_we)
					.ctr(timer_data_r),
					.cnt_end(timer_end)
			)

endmodule
