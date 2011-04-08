`include "../include/tune.v"

// Pentevo project (c) NedoPC 2010
//
// decoding video modes: mode, raster, bandwidth, pixel freq
//
// refactored by TS-Labs


module video_modedecode(

	input  wire [ 7:0] vcfg,	   // videoconfig

	output wire         mode_zx, // standard ZX mode
	output wire         mode_tm0_en, // tiles0 enabled
	output wire         mode_tm1_en, // tiles1 enabled
	output wire         mode_brd, // no linear gfx - only border color
	
	output wire         mode_pixf_14, // 14MHz pixelclock on (default is 7MHz).

	output wire  [1:0]	rres;		//raster resolution 256/320/360

	output wire  [ 1:0] mode_bw 	// required bandwidth: 2'b00 - 1/8, 2'b01 - 1/4,
									//                     2'b10 - 1/2, 2'b11 - 1
);

	wire [2:0] vmode = vcfg[2:0];	//just alias
	wire [1:0] rres = vcfg[4:3];	//raster mode isn't decoded, just clipped from vconfig


//video mode decode

	wire zx =	(vmode == 3'b000);
	wire tmhr =	(vmode == 3'b001);
	wire tm0 =	(vmode == 3'b010);
	wire tm1 =	(vmode == 3'b011);
	wire brd =	(vmode == 3'b111);

	assign mode_zx = zx;
	assign mode_tm0_en = (tm0 | tm1 | tmhr);
	assign mode_tm1_en = (tm1 | tmhr);
	assign mode_brd = brd;

	
//bandwidth decode
	
	begin
		if (tm1 | tmhr)
			assign mode_bw = 2'b10; 	// 1/2
		else if (tm0)
			assign mode_bw = 2'b01; 	// 1/4
		else
			assign mode_bw = 2'b00; 	// 1/8
	end

			
//pixel frequency decode

	begin
		if (tmhr)
			assign mode_pixf_14 = 1'b1;
		else
			assign mode_pixf_14 = 1'b0;
	end

endmodule

