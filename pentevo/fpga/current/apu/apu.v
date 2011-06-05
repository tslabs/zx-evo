`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Auxilliary Processor
//
// Written by TS-Labs inc.
//


module apu(

	input clk,
	
// ZMAPS
	input code_wen,
	input code_wdata,
	input code_waddr,

// DRAM
	output reg [20:0] apu_addr,
	output reg [15:0] apu_data_w,
	
	
	);


// registers
	reg [7:0] r[0:15];

	
// dummy definitions
	wire [15:0] sram_data_r = 16'b0;
	wire [15:0] sram_addr;
//	wire [23:0] dummy;
	
	wire [23:0] port_in[0:15];
	wire [23:0] port_out[0:15];
	wire port_we[0:15];
	wire pin_in[0:31];
	wire pin_out[0:31];
	
// read ports
	assign port_in[0] = 24'b0;
	assign port_in[1] = {8'b0, sram_data_r};
	assign port_in[2] = 24'b0;
	assign port_in[3] = 24'b0;
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
	assign port_out[0] = sram_addr;
	assign port_out[1] = sram_data_w;
	assign port_out[2] = apu_addr;
	assign port_out[3] = apu_data_w;
	assign port_out[4] = timer_wdata;
	// assign port_out[5] = dummy;
	// assign port_out[6] = dummy;
	// assign port_out[7] = dummy;
	// assign port_out[8] = dummy;
	// assign port_out[9] = dummy;
	// assign port_out[10] = dummy;
	// assign port_out[11] = dummy;
	// assign port_out[12] = dummy;
	// assign port_out[13] = dummy;
	// assign port_out[14] = dummy;
	// assign port_out[15] = dummy;

// write ports WEs
	assign port_we[0] = dummy;
	assign port_we[1] = dummy;
	assign port_we[2] = dummy;
	assign port_we[3] = dummy;
	assign port_we[4] = dummy;
	// assign port_we[5] = dummy;
	// assign port_we[6] = dummy;
	// assign port_we[7] = dummy;
	// assign port_we[8] = dummy;
	// assign port_we[9] = dummy;
	// assign port_we[10] = dummy;
	// assign port_we[11] = dummy;
	// assign port_we[12] = dummy;
	// assign port_we[13] = dummy;
	// assign port_we[14] = dummy;
	// assign port_we[15] = dummy;

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
	assign pin_out[0] = dummy;
//	assign pin_out[1] = dummy;
//	assign pin_out[1] = dummy;
//	assign pin_out[2] = dummy;
//	assign pin_out[3] = dummy;
//	assign pin_out[4] = dummy;
//	assign pin_out[5] = dummy;
//	assign pin_out[6] = dummy;
//	assign pin_out[7] = dummy;
//	assign pin_out[8] = dummy;
//	assign pin_out[9] = dummy;
//	assign pin_out[10] = dummy;
//	assign pin_out[11] = dummy;
//	assign pin_out[12] = dummy;
//	assign pin_out[13] = dummy;
//	assign pin_out[14] = dummy;
//	assign pin_out[15] = dummy;


	always @(posedge clk)
	begin
	case



	endcase
	end


	wire [7:0]	code_raddr;
	wire [15:0]	code;
		
	apu_code apu_code(
					.clock(clk),
					.wraddress(code_waddr),
					.data(code_wdata),
					.wren(code_wen),
					.rdaddress(code_raddr),
					.q(code)
					);

					
	apu_timer apu_timer(
					.clk(clk),
					.wdata(timer_wdata),
					.wen(timer_wen)
					.ctr(timer_data),
					.cnt_end(timer_end)
			)
endmodule
