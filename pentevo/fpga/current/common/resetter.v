`include "tune.v"

// Reset from MCU must be long enough to 
module resetter(
	input wire clk,
	input wire rst_in_n,      // input of external asynchronous reset
	output reg rst_out_n );   // output of synchronized reset

`ifdef SIMULATE
	initial
	begin
		rst_out_n = 1'b0;
	end
`endif

	always @(posedge clk)
		rst_out_n <= rst_in_n;

endmodule

