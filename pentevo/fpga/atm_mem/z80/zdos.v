// PentEvo project (c) NedoPC 2008-2010
//
// just DOS signal control

`include "../include/tune.v"


module zdos(

	input  wire        fclk,
	input  wire        rst_n,


	input  wire        dos_turn_on,
	input  wire        dos_turn_off,

	input  wire        cpm_n,


	output reg         dos
);







	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		dos = 1'b1;
	end
	else // posedge fclk
	begin
		if( !cpm_n )
			dos <= 1'b1;
		else if( dos_turn_off )
			dos <= 1'b0;
		else if( dos_turn_on )
			dos <= 1'b1;
	end







endmodule


