`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
//Sprite Processor
//

module sprites(

	input wire clk,
	input wire spr_en,
	
	output reg [5:0] spixel,
	output reg spx_en
	

	);

	reg [10:0] cnt;
	
	always @(posedge clk)
	if (spr_en)

	begin

		cnt <= cnt + 6'b1;

		spx_en <= cnt [10];
		spixel <= cnt [9:4];


	end
	
	else spx_en <= 1'b0;

endmodule



/*
module spbuf(

	input  wire        clk,

	input  wire [10:0] w_adr,
	input  wire [ 7:0] w_dat,
	input  wire        w_stb,

	input  wire [10:0] r_adr,
	output reg  [ 7:0] r_dat
);

	reg [7:0] spb0 [0:383];
	reg [7:0] spb1 [0:383];

	always @(posedge clk)
	begin
		if( w_stb )
		begin
			spb0[w_adr] <= w_dat;
		end

		r_dat <= spb0[r_adr];
	end
endmodule
*/
