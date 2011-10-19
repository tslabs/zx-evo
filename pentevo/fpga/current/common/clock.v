// This module receives 112 MHz as input clock
// and formes strobes for all clocked parts
// (now forms only 28 MHz strobes)


module clock (

	input wire clk,
	output reg s3

);


	localparam DIV = 6'd3;
	
	reg [5:0] cnt0 = 0;

	always @(posedge clk)
	begin
		if (cnt0 == DIV)
			cnt0 <= 0;
		else
			cnt0 <= cnt0 + 1;
			
		if (cnt0 == 0)
			s3 <= 1;
		else
			s3 <= 0;
	end
	

endmodule
