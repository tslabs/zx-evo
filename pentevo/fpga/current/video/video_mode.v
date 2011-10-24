// This module decodes video modes


module video_mode (

// clocks
	input wire clk,
	
// config data
	input  wire [7:0] vconfig,
	
// video parameters
	output wire [8:0] hpix_beg,
	output wire [8:0] hpix_end,
	output wire [8:0] vpix_beg,
	output wire [8:0] vpix_end,
	
// mode controls
	output wire mode_zx,
	output wire mode_256c
	
);


// raster resolution
	wire [1:0] rres = vconfig[4:3];
	// wire [1:0] rres = 3;
	
	wire [8:0] hp_beg[0:3];
	wire [8:0] hp_end[0:3];
	wire [8:0] vp_beg[0:3];
	wire [8:0] vp_end[0:3];

	assign hp_beg[0] = 9'd140;	// 256 (88-52-256-52)
	assign hp_beg[1] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[2] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[3] = 9'd88;	// 360 (88-0-360-0  )

	assign hp_end[0] = 9'd396;	// 256
	assign hp_end[1] = 9'd428;	// 320
	assign hp_end[2] = 9'd428;	// 320
	assign hp_end[3] = 9'd448;	// 360

	assign vp_beg[0] = 9'd080;	// 192 (32-48-192-32)
	assign vp_beg[1] = 9'd076;	// 200 (32-44-200-44)
	assign vp_beg[2] = 9'd056;	// 240 (32-24-240-24)
	assign vp_beg[3] = 9'd032;	// 288 (32-0-288-0  )

	assign vp_end[0] = 9'd272;	// 192
	assign vp_end[1] = 9'd276;	// 200
	assign vp_end[2] = 9'd296;	// 240
	assign vp_end[3] = 9'd320;	// 288

	assign hpix_beg = hp_beg[rres];
	assign hpix_end = hp_end[rres];
	assign vpix_beg = vp_beg[rres];
	assign vpix_end = vp_end[rres];
	
	
	
	
endmodule