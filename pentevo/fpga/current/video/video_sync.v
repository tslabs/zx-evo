// This module generates all video raster signals


module video_sync (

// clocks
	input wire clk,
	input wire c0,
	input wire c12,

// video syncs
	output reg hsync,
	output reg vsync,
	output reg csync,

// video controls
	output wire tv_pix_stb,
	output wire vga_pix_stb,
	output wire blank,
	output wire line_start,
	output wire frame_start,
	output wire hpix,
	output wire vpix,
	output wire hvpix,
	output wire fetch_zx,
	output wire pix_start,
	
// ZX controls
	output wire int_start
	
);
	
	localparam HSYNC_BEG 	= 9'd10;
	localparam HSYNC_END 	= 9'd43;
	localparam HBLNK_BEG 	= 9'd00;
	localparam HBLNK_END 	= 9'd88;
	
	localparam HSYNCV_BEG 	= 9'd5;
	localparam HSYNCV_END 	= 9'd31;
	localparam HBLNKV_END 	= 9'd44;
	
	localparam HINT_BEG  	= 9'd2;
	localparam HPERIOD   	= 9'd448;

	localparam VSYNC_BEG 	= 9'd08;
	localparam VSYNC_END 	= 9'd11;
	localparam VBLNK_BEG 	= 9'd00;
	localparam VBLNK_END 	= 9'd32;
	
	localparam VINT_BEG  	= 9'd0;
	localparam VPERIOD   	= 9'd320;	// fucking pentagovn!!!


	wire [8:0] hpix_beg = 9'd140;	// 256
	wire [8:0] hpix_end = 9'd396;	// 256
	wire [8:0] vpix_beg = 9'd080;	// 192
	wire [8:0] vpix_end = 9'd272;	// 192
	
	
// counters
	reg [8:0] hcount = 0;
	reg [8:0] vcount = 0;

	always @(posedge clk) if (c12)
		hcount <= hcount == (HPERIOD - 1) ? 0 : hcount + 1;

		
	always @(posedge clk) if (c12)
		if (hcount == (HPERIOD - 1))
			vcount <= vcount == (VPERIOD - 1) ? 0 : vcount + 1;
	
	
//	strobes
	wire hs = (hcount >= HSYNC_BEG) & (hcount < HSYNC_END);
	wire hs_vga = ((hcount >= HSYNCV_BEG) & (hcount < HSYNCV_END)) |
			((hcount >= (HSYNCV_BEG + HPERIOD/2)) & (hcount < (HSYNCV_END + HPERIOD/2)));
	wire hb = (hcount >= HBLNK_BEG) & (hcount < HBLNK_END);
			
	wire vs = (vcount >= VSYNC_BEG) & (vcount < VSYNC_END);
	wire vb = (vcount >= VBLNK_BEG) & (vcount < VBLNK_END);
	
	assign tv_pix_stb = (hcount == (HBLNK_END - 1));
	assign vga_pix_stb = ((hcount == (HBLNKV_END - 1)) | (hcount == (HBLNKV_END + HPERIOD/2 - 1))) & c0;

	assign blank = hb | vb;
	
	assign hpix = (hcount >= hpix_beg) & (hcount < hpix_end);
	assign vpix = (vcount >= vpix_beg) & (vcount < vpix_end);
	assign hvpix = hpix & vpix;
	
	assign fetch_zx = (hcount >= (hpix_beg - 18)) & (hcount < (hpix_end - 18)) & vpix;
	
	assign line_start = (hcount == (HPERIOD - 1));
	assign frame_start = (hcount == (HPERIOD - 1)) & (vcount == (VPERIOD -1));
	assign pix_start = (hcount == (hpix_beg - 1));
	
	assign int_start = (hcount == HINT_BEG) & (vcount == VINT_BEG);
	
	
	always @(posedge clk)
	begin
		hsync <= ~hs_vga;
		vsync <= ~vs;
		csync <= ~(vs ^ hs);
	end


endmodule
