`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
// generates PWM on BEEP pin
// PWM parameters: 27,34375 kHz(28000/1024) 10 bit

module covox(
	input fclk,  // global FPGA clock, 28MHz
	input [7:0] psd0, psd1, psd2, psd3,
	output reg beep
	);

	reg [9:0] saw;
	reg [9:0] pwm;

initial
begin
	saw = 10'd0;
	pwm = 0;
end
	
//'Saw' counter for PWM 0..1023
always @(posedge fclk)
begin
	saw <= saw + 10'd1;	//inc saw

	if (saw == pwm)
	beep <= 1'b0;	//BEEP is reset on the end of PWM duty cycle
	else if (saw == 10'b0)
	beep <= 1'b1;	//BEEP is set on the begin of PWM duty cycle
end

//Update PWM with the sum of 4 SD channels
always @(negedge saw[9])
begin
	pwm <= (psd0 + psd1 + psd2 + psd3);
end
		
endmodule
