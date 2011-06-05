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
	
// FMAPS
	input code_wen,
	input wire [7:0] code_wdata,
	input [8:0] code_waddr,

// DRAM
	output reg [20:0] apu_addr,
	output reg [15:0] apu_data_w,
	input apu_strobe
	
	);


	// registers
	reg [7:0] r[0:15];

	
	// dummy definitions
	wire [15:0] sram_data_r = 16'b0;
//	wire [23:0] dummy;
	
	// port wires
	wire [23:0] port_in[0:15];
	wire [23:0] port_out[0:15];
//	wire port_we[0:15];
	wire pin_in[0:31];
	wire pin_out[0:31];
	
	// read ports
	assign port_in[0] = 24'b0;
	assign port_in[1] = {8'b0, sram_data_r};
	assign port_in[2] = 24'b0;
//	assign port_in[3] = {16'b0, sram_rdata};
	assign port_in[3] = 24'b0;	//dummy
	assign port_in[4] = {8'b0, timer_data};
	assign port_in[5] = 24'b0;
	assign port_in[6] = 24'b0;
	assign port_in[7] = 24'b0;
	assign port_in[8] = 24'b0;
	assign port_in[9] = 24'b0;
	assign port_in[10] = 24'b0;
	assign port_in[11] = 24'b0;
	assign port_in[12] = 24'b0;
	assign port_in[13] = 24'b0;
	assign port_in[14] = 24'b0;
	assign port_in[15] = 24'b0;
	
	// write ports
	assign sram_addr = port_out[0];
	assign sram_data_w = port_out[1];
	assign apu_addr = port_out[2];
	assign apu_data_w = port_out[3];
	assign timer_wdata = port_out[4];
                       
// read pins
	assign pin_in[0] = timer_end;
	assign pin_in[1] = 1'b0;
	assign pin_in[2] = 1'b0;
	assign pin_in[3] = apu_strobe;
	assign pin_in[4] = 1'b0;
	assign pin_in[5] = 1'b0;
	assign pin_in[6] = 1'b0;
	assign pin_in[7] = 1'b0;
	assign pin_in[8] = 1'b0;
	assign pin_in[9] = 1'b0;
	assign pin_in[10] = 1'b0;
	assign pin_in[11] = 1'b0;
	assign pin_in[12] = 1'b0;
	assign pin_in[13] = 1'b0;
	assign pin_in[14] = 1'b0;
	assign pin_in[15] = 1'b0;

// write pins
	assign apu_req = pin_out[0];


	// APU
	reg _c, _z, _n;
	wire pc = {r[15], r[14]};
	wire [15:0] pc_next;
	
	always @(posedge clk)
	begin
	
	pc <= pc_next;
	
	assign pc_next = pc + 16'd1;
	
	// HALT state handling
	if (apu_halt)
	begin
		pc <= 16'b0;
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

	// cashing of lower byte of APU code
	reg [7:0] code_wdata_lo;
	
	always @(posedge clk)
	if (code_wen && !code_waddr[0])
		code_wdata_lo <= code_wdata;
		

	wire [15:0]	instr;
		
	apu_code apu_code(
					.clock(clk),
					.wraddress(code_waddr[8:1]),
					.data(code_wdata, code_wdata_lo),
					.wren(code_wen && code_waddr[0]),
					.rdaddress(pc),
					.q(instr)
					);


	reg timer_wen;
					
	apu_timer apu_timer(
					.clk(clk),
					.wdata(timer_wdata),
					.wen(timer_wen)
					.ctr(timer_data),
					.cnt_end(timer_end)
			)
endmodule
