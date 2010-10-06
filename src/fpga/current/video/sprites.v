`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
//Sprite Processor
//

module sprites(

	input wire clk,
	input wire vcfg,
	
	output [5:0] spixel,
	output wire spx_en
	

	);

	reg [7:0] cnt;
	
	always @(posedge clk)
	
//	if vcfg[6]
	begin
		cnt <= cnt + 6'b1;
	end

	assign spx_en = cnt [7];
	assign spixel = 6'd53;
	
	
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

endmodule
