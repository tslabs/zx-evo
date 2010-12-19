`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
//Written by TS-Labs inc.
//
//generates PWM on BEEP pin


module covox(
	input clk, hus_en,
	input [5:0] psd0, psd1, psd2, psd3,
	input [15:0] ldac, rdac,
	output beep
	);

	reg [7:0] acc;
	wire [7:0] pwm;
	
	assign pwm = hus_en ? (ldac[15:9] + rdac[15:9]) : (psd0[5:0] + psd1[5:0] + psd2[5:0] + psd3[5:0]);
	assign beep = (acc < pwm);
	
always @(posedge clk)
		acc <= acc + 8'b1;

endmodule
