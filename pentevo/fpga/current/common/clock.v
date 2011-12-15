// This module receives 56 MHz as input clock
// and formes strobes for all clocked parts

//			0 1 2 3 4 5 6 7 |0 1 2 3 4 5 6 7 
// clk		-_-_-_-_-_-_-_-_|-_-_-_-_-_-_-_-_	56
// f0		--__--__--__--__|--__--__--__--__	28
// f1		__--__--__--__--|__--__--__--__--	28
// q0		--______--______|--______--______	14
// q2		____--______--__|____--______--__	14
// q3		______--______--|______--______--	14
// c0		--______________|--______________	7	(ex cbeg)
// c2		____--__________|____--__________	7	(ex post_cbeg)
// c4		________--______|________--______	7	(ex pre_cend)
// c6		____________--__|____________--__	7	(ex cend)
// c7		______________--|______________--	7


module clock (

	input wire clk,
	output wire f0, f1, q0, q2, q3, c0, c2, c4, c6, c7,
	output wire clk175

);


	reg [4:0] cnt = 0;

	always @(posedge clk)
		cnt <= cnt + 1;
		
		
		assign f0 = ~cnt[0];
		assign f1 = cnt[0];
		assign q0 = cnt[1:0] == 2'd0;
		assign q2 = cnt[1:0] == 2'd2;
		assign q3 = cnt[1:0] == 2'd3;
		assign c0 = cnt[2:0] == 3'd0;
		assign c2 = cnt[2:0] == 3'd2;
		assign c4 = cnt[2:0] == 3'd4;
		assign c6 = cnt[2:0] == 3'd6;
		assign c7 = cnt[2:0] == 3'd7;
		assign clk175 = cnt[4];
	

endmodule
