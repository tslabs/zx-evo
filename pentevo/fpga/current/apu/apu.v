`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Auxilliary Processor
//
// Written by TS-Labs inc.
//


module apu(

	input clk,
	
// APU
	input apu_halt,
	output reg apu_rdy,
	
// ZMAPS
	input code_wen,
	input wire [7:0] code_wdata,
	input [8:0] code_waddr,

// DRAM
	output reg [20:0] apu_addr,
	output reg [15:0] apu_data_w,
	input apu_strobe
	
	);


//- Ports -----------------------------------------	

	wire [15:0] sram_data_r = 16'b0;
	
	// port wires
	wire [23:0] port_in[0:15];
	wire [23:0] port_out[0:15];
	wire pin_in[0:31];
	wire pin_out[0:31];
	
	// read ports
	assign port_in[1] = {8'b0, sram_data_r};
	assign port_in[4] = {8'b0, timer_data};
	
	// write ports
	assign sram_addr = port_out[0];
	assign sram_data_w = port_out[1];
	assign apu_addr = port_out[2];
	assign apu_data_w = port_out[3];
	assign timer_wdata = port_out[4];
                       
// read pins
	assign pin_in[0] = timer_end;
	assign pin_in[3] = apu_strobe;

// write pins
	assign apu_req = pin_out[0];

	
//- APU -------------------------------------------------------------------

	reg _c, _z, _n;			//flags
	reg [7:0] r[0:15];		// registers
	
	wire pc = {r[15], r[14]};

	
//- Operands decoding ----------------------------------
	reg [15:0] rel;
	
	wire [7:0] imm = instr[7:0];	// immediate 8
	wire [4:0] pin0 = instr[4:0];	// pin0 5
//	wire [4:0] pin1 = instr[9:5];	// pin1 5
	wire [4:0] pin1 = instr[4:0];	// !!! RESTRICTION 3 !!!
	wire [3:0] src = instr[3:0];	// source reg8 4
	wire [3:0] dst = instr[11:8];	// destination reg8 4
	wire [3:0] port = instr[3:0];	// source port8 4
	wire [2:0] bit = instr[6:4];	// bit select 3
	wire [2:0] jc = instr[11:9];	// jump condition 3
	wire [2:0] jc_m = instr[6:4];	// jump condition in miscs 3
	wire [1:0] sz = instr[5:4];		// sizeness 2
	wire [1:0] wc = instr[7:6];		// wait condition 2
	wire bit0 = instr[10];			// bit0 value
	wire bit1 = instr[11];			// bit1 value
	wire cond = instr[11];			// condition
	wire mask = instr[10];			// mask
	wire flag = instr[7];			// flag select

	always @*
	case instr[15:12]		// synthesis full_case 

4'b1101:	// jr jc, rel9
		rel = {7{instr[8]}, instr[8:0]};

4'b1101:	// djnz src, rel8
		rel = {8{instr[11]}, instr[11:4]};

	endcase

	
//- PC calculating and modifying ------------------------
	reg [15:0] pc_next;
	wire pc_inc = pc + 16'b1;
	wire pc_rel = pc + rel;

	always @*
	if (apu_halt)
		pc_next = 16'b0;
	else
	case (instr[15:12])

4'b1000:	// wait	cond pin0 mask pin1
		pc_next = ~cond ^^ (~mask ? pin[pin0] || pin[pin1] : pin[pin0] && pin[pin1]) ? pc : pc_inc;

4'b1010:	// wait sz (port) wc src

4'b1101:	// jr jc, rel9
		pc_next = jcond ? pc_rel : pc_inc;

4'b1111:	// halt
		pc_next = instr[11:0] == 12'hfff ? pc : pc_inc;

default:
		pc_next = pc_inc;
	
	endcase

	
//- Jump Condition decoding -----------------------------
	reg jcond;
	
	always @*	
	case (jc)

3'b000:		// always
		jcond = 1'b1;

3'b001:		// carry
		jcond = _c;
		
3'b010:		// zero
		jcond = _z;
		
3'b011:		// negative
		jcond = _n;
		
3'b100:		// never
		jcond = 1'b0;

3'b101:		// not carry
		jcond = ~_c;
		
3'b110:		// not zero
		jcond = ~_z;
		
3'b111:		// not negative
		jcond = ~_n;
	
	endcase
	

//- PC clocking -----------------------------------------
	always @(posedge clk)
		pc <= pc_next;


//- Ready signal handling -------------------------------
	always @(posedge clk)
	if (instr == 16'hffff)
		apu_rdy <= 1'b1;
	else
		apu_rdy <= 1'b0;


//-------------------------------------------------------
	always @(posedge clk)
	if (apu_halt)
	begin
		apu_rdy <= 1'b0;
	end
	
	else
	
	// instructions by class
	case (instr[15:12])

4'b0000:
	// ld imm
	begin
		r[instr[11:8]] <= instr[7:0];
	end

4'b0001:
	// and imm
	begin
		r[instr[11:8]] <= r[instr[11:8]] & instr[7:0];
//		_z <= (r[instr[11:8]] & instr[7:0]) == 8'b0;
//		_n <= r[instr[11:8]][7] & instr[7];
	end

4'b0010:
	// or imm
	begin
		r[instr[11:8]] <= r[instr[11:8]] | instr[7:0];
	end

4'b0011:
	// xor imm
	begin
		r[instr[11:8]] <= r[instr[11:8]] ^ instr[7:0];
	end

4'b0100:
	// add imm
	begin
		{_c, r[instr[11:8]]} <= {1'b0, r[instr[11:8]]} + {1'b0, instr[7:0]};
	end

4'b0101:
	// sub imm
	begin
		{_c, r[instr[11:8]]} <= {1'b0, r[instr[11:8]]} - {1'b0, instr[7:0]};
	end

4'b0110:
	// ld/and/or/xor
	begin
	
	case (instr[11:10])

	2'b00:
		// ld
		begin
			r[instr[7:4]] <= r[instr[3:0]];
		end

	2'b01:
		// and
		begin
			r[instr[7:4]] <= r[instr[7:4]] & r[instr[3:0]];
		end

	2'b10:
		// or
		begin
			r[instr[7:4]] <= r[instr[7:4]] | r[instr[3:0]];
		end

	2'b11:
		// xor
		begin
			r[instr[7:4]] <= r[instr[7:4]] ^ r[instr[3:0]];
		end

		endcase
		end

4'b0111:
	// 
	begin
	end

4'b1000:
	// 
	begin
	end

4'b1001:
	// 
	begin
	end

4'b1010:
	// 
	begin
	end

4'b1011:
	// 
	begin
	end

4'b1100:
	// 
	begin
	end

4'b1101:
	// 
	begin
	end

4'b1110:
	// 
	begin
	end

4'b1111:
	// misc
	begin
	end

	// instructions by sub-class
	case (instr[11:8])

	4'b0000:
		// 
		begin
		end

	4'b0001:
		// 
		begin
		end

	4'b0010:
		// 
		begin
		end

	4'b0011:
		// 
		begin
		end

	4'b0100:
		// 
		begin
		end

	4'b0101:
		// 
		begin
		end

	4'b0110:
		// 
		begin
		end

	4'b0111:
		// 
		begin
		end

	4'b1000:
		// 
		begin
		end

	4'b1001:
		// 
		begin
		end

	4'b1010:
		// 
		begin
		end

	4'b1011:
		// 
		begin
		end

	4'b1100:
		// 
		begin
		end

	4'b1101:
		// 
		begin
		end

	4'b1110:
		// 
		begin
		end

	4'b1111:
		// misc
		// halt
		if (instr[7:0] = 8'b11111111)
		begin
			apu_rdy <= 1'b1;
			assign pc_next = pc;
		end

		endcase

	endcase
	end


//- Write to APU microcode RAM ---------------------------------
	reg [7:0] code_wdata_lo;
	wire code_wren = code_wen && code_waddr[0];
	wire [15:0] code_data = {code_wdata, code_wdata_lo};
	
	always @(posedge clk)
	if (code_wen && !code_waddr[0])
		code_wdata_lo <= code_wdata;
		

//- APU microcode RAM module -----------------------------------
	wire [15:0]	instr;
		
	apu_code apu_code(
					.clock(clk),
					.wraddress(code_waddr[8:1]),
					.data(code_data),
					.wren(code_wren),
					.rdaddress(pc),
					.q(instr)
					);


//- Timer module ------------------------------------------------
	reg timer_wen;
					
	apu_timer apu_timer(
					.clk(clk),
					.wdata(timer_wdata),
					.wen(timer_wen)
					.ctr(timer_data),
					.cnt_end(timer_end)
			)

endmodule
