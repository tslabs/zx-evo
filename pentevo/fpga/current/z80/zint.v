
`include "../include/tune.v"

module zint
(
	input  wire clk,
	input  wire int_start,
	input  wire vdos,
	input  wire intack_s,
	output reg  int_n
);


// INT generating
	always @(posedge clk)
	begin
		if (int_start && !vdos)
			int_n <= 1'b0;
		else if (intctr[9] || intack_s)
			int_n <= 1'bZ;
	end


// INT counter
	reg [9:0] intctr;
	always @(posedge clk)
	begin
		if (int_start)
			intctr <= 10'd0;
		else if (!intctr[9])   // 32 clks 3,5MHz
			intctr <= intctr + 10'd1;
	end
    

endmodule
