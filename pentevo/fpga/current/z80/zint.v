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

	input  wire iorq_n,
	input  wire m1_n,

	output reg  int_n
);

	wire intend;

	reg [9:0] intctr;



`ifdef SIMULATE
	initial
	begin
		intctr = 10'b1100000000;
	end
`endif


	always @(posedge fclk)
	begin
		if( int_start )
			intctr <= 10'd0;
		else if( !intctr[9:8] )
			intctr <= intctr + 10'd1;
	end


	assign intend = intctr[9:8] || ( (!iorq_n) && (!m1_n) && zneg );


	always @(posedge fclk)
	begin
		if( int_start )
			int_n <= 1'b0;
		else if( intend )
			int_n <= 1'bZ;
	end





endmodule

