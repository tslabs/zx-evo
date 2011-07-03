`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
//Written by TS-Labs inc.
//
//generates PWM on BEEP pin


module covox(
	input clk, hus_en,
	input [7:0] cvx,
	input [15:0] ldac, rdac,
	output beep
	);

	reg [7:0] acc;
	
	assign beep = (acc[7:0] < cvx);
	
always @(posedge clk)
		acc <= acc + 8'b1;

endmodule
