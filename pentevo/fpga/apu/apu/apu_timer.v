`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// APU Timer
//
// Written by TS-Labs inc.
//


module	apu_timer(

	input	clk,
	input wire [15:0]	wdata,		//counter limit input
	input	wen,					//counter limit write enable
	output reg [15:0]	ctr,		//current value of counter
	output reg	cnt_end				//is set to 1 when current == limit, then counter is set to 0

		);


	reg [15:0] ctr_data;

	
	always @(posedge clk)
	if (wen)
	begin
		ctr_data <= wdata;
		ctr <= 16'b0;
	end
	else
		ctr <= ctr + 16'b1;
	

	always @(posedge clk)
	if (ctr == ctr_data)
		cnt_end <= 1'b1;
	else
		cnt_end <= 1'b0;

		
endmodule
