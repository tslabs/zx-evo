// Pentevo project (c) NedoPC 2011
//
// frame INT generation

`include "../include/tune.v"

module zint
(
	input  wire fclk,

	input  wire zpos,
	input  wire zneg,

	input  wire int_start,
	input  wire vdos,

	input  wire iorq_n,
	input  wire m1_n,

	output reg  int_n
);

	wire intend;

	reg [9:0] intctr;


`ifdef SIMULATE
	initial
	begin
		intctr = 10'b1000000000;
	end
`endif


	always @(posedge fclk)
	begin
		if( int_start )
			intctr <= 10'd0;
		// else if (~&intctr[9:8])   // 48 clks 3,5MHz
		else if (!intctr[9])   // 32 clks 3,5MHz
			intctr <= intctr + 10'd1;
	end


	always @(posedge fclk)
	begin
		if (int_start & !vdos)
			int_n <= 1'b0;
		else if (intctr[9] | ((!iorq_n) && (!m1_n) && zneg))
			int_n <= 1'bZ;
	end



endmodule

