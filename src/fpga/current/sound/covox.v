`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
//Written by TS-Labs inc.
//
// generates PWM on BEEP pin
// PWM parameters: 54,6875 kHz (28000/512), 9 bit

module covox(
	input fclk,  // global FPGA clock, 28MHz
	input [7:0] psd0, psd1, psd2, psd3,
	output reg beep
	);

	reg [8:0] saw;
	reg [8:0] pwm;

initial
begin
	saw = 9'd0;
	pwm = 0;
end
	
//'Saw' counter for PWM 0..511
always @(posedge fclk)
begin
	saw <= saw + 9'd1;	//inc saw

	if (saw == pwm)
	beep <= 1'b0;	//BEEP is reset on the end of PWM duty cycle
	
	else if (saw == 9'h1ff)
	begin
		beep <= 1'b1;	//BEEP is set on the begin of PWM duty cycle
		pwm <= (psd0[7:1] + psd1[7:1] + psd2[7:1] + psd3[7:1]); 	//pwm reloaded from registers
	end
end

endmodule
