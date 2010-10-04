`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010
//
// generates PWM on BEEP pin
// PWM parameters: 27,34375 kHz(28000/1024) 10 bit

module covox(

	input fclk,  // global FPGA clock, 28MHz
	input [7:0] psd0, psd1, psd2, psd3

	output beep,
	
	reg [9:0] saw;
	
	wire [9:0] ppwm;

	);

initial
begin
	saw = 10'd0;
end
	
//The resulting PWM is the sum of 4 SD channels
always @*
begin
	ppwm <= psd0[7:0] + psd1[7:0] + psd2[7:0] + psd3[7:0]
end
	
//'Saw' counter for PWM 0..1023
always @(posedge clk)
begin
	saw <= saw + 10'd1;
end
		
always @*
begin
//BEEP is set on the begin of PWM duty cycle
	if (saw==(10'b0))
	beep <= 1'b1;

//BEEP is reset on the end of PWM duty cycle
	if (saw==ppwm[9:0])
	beep <= 1'b0;
end

endmodule
