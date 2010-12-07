`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
//Written by TS-Labs inc.
//
// generates PWM on BEEP pin
// PWM parameters: (28000/1024)kHz, 10 bit

module covox(
	input clk,  // global FPGA clock, 28MHz
	input [7:0] psd0, psd1, psd2, psd3,
	output reg beep, covox_stb
	);

	localparam bits = 10;
	reg [bits-1:0] saw;
	reg [bits-1:0] pwm;

initial
begin
	saw = 0;
	pwm = 0;
end
	
	
//Zero strobe
always @(posedge clk)
if (saw == 10'hffff)
	begin
		covox_stb = 1'b1;
		pwm <= (psd0[7:0] + psd1[7:0] + psd2[7:0] + psd3[7:0]); 	//pwm reloaded from registers
	end
else
	covox_stb = 1'b0;
	
	
//'Saw' counter for PWM
always @(posedge clk)
		saw <= saw + 10'd1;	//inc saw


//Output managing
always @(posedge clk)
begin
	if (covox_stb)
		beep <= 1'b1;	//BEEP is set on the begin of PWM duty cycle

	else if (saw == pwm)
		beep <= 1'b0;	//BEEP is reset on the end of PWM duty cycle
end

endmodule
