`include "../include/tune.v"

// Pentevo project (c) NedoPC 2010-2011
//
// mix up border and pixels, add palette and blanks
//

module video_palframe(

	input  wire        clk, // 28MHz clock


	input  wire        hpix,
	input  wire        vpix,

	input  wire        hblank,
	input  wire        vblank,

	input  wire [ 3:0] pixels,
	input  wire [ 3:0] border,


	input  wire        atm_palwr,
	input  wire [ 5:0] atm_paldata,


	output wire [ 5:0] color
);


	wire [ 3:0] zxcolor;

	reg  [ 5:0] palcolor;


	reg       win;
	reg [3:0] border_r;



//	always @(posedge clk)
//		win <= hpix & vpix;
//
//	always @(posedge clk)
//		border_r <= border;
//
//	assign zxcolor = win ? pixels : border_r;

	assign zxcolor = (hpix&vpix) ? pixels : border;


	// palette
	reg [5:0] palette [0:15]; // let quartus instantiate it as RAM if needed

	always @(posedge clk)
	begin
		if( atm_palwr )
			palette[zxcolor] <= atm_paldata;

		palcolor <= palette[zxcolor];
	end


	assign color = (hblank | vblank) ? 6'd0 : palcolor;


endmodule

