`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
//Written by TS-Labs inc.
//
//generates PWM on BEEP pin


module covox(
	input clk, hus_en,
	input [7:0] psd0, psd1, psd2, psd3,
	input [15:0] ldac, rdac,
	output beep
	);

	reg [10:0] acc;
	wire [9:0] pwm;
	
	assign beep = acc[10];
	assign pwm = hus_en ? (ldac[15:7] + rdac[15:7]) : (psd0[7:0] + psd1[7:0] + psd2[7:0] + psd3[7:0]);
	
always @(posedge clk)
		acc <= acc[9:0] + pwm;

endmodule
