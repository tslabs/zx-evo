// This module generates all video raster signals


module video_sync (

// clocks
	input wire clk,
	input wire c0,
	input wire c6,

// video parameters
	input wire [8:0] hpix_beg,
	input wire [8:0] hpix_end,
	input wire [8:0] vpix_beg,
	input wire [8:0] vpix_end,
	input wire [4:0] go_offs,
	input wire [1:0] x_offs,
	
// video syncs
	output reg hsync,
	output reg vsync,
	output reg csync,

// video controls
	output wire hpix,
	output wire vpix,
	output wire hvpix,
	output wire hb,
	output wire vb,
	output wire vga_line,
	output wire line_start,
	output wire frame_start,
	output wire tv_pix_start,
	output wire vga_pix_start,
	output wire pix_start,
	output wire video_go,

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


// counters
	reg [8:0] hcount = 0;
	reg [8:0] vcount = 0;

	always @(posedge clk) if (c6)
		hcount <= hcount == (HPERIOD - 1) ? 0 : hcount + 1;

		
	always @(posedge clk) if (c6)
		if (hcount == (HPERIOD - 1))
			vcount <= vcount == (VPERIOD - 1) ? 0 : vcount + 1;
	
	
//	strobes
	wire hs = (hcount >= HSYNC_BEG) & (hcount < HSYNC_END);
	wire hs_vga = ((hcount >= HSYNCV_BEG) & (hcount < HSYNCV_END)) |
			((hcount >= (HSYNCV_BEG + HPERIOD/2)) & (hcount < (HSYNCV_END + HPERIOD/2)));
	assign hb = (hcount >= HBLNK_BEG) & (hcount < HBLNK_END);
			
	wire vs = (vcount >= VSYNC_BEG) & (vcount < VSYNC_END);
	assign vb = (vcount >= VBLNK_BEG) & (vcount < VBLNK_END);
	
	assign tv_pix_start = (hcount == (HBLNK_END - 1));
	assign vga_pix_start = ((hcount == (HBLNKV_END - 1)) | (hcount == (HBLNKV_END + HPERIOD/2 - 1))) & c0;

	assign vga_line = (hcount >= HPERIOD/2);
	
	assign hpix = (hcount >= hpix_beg) & (hcount < hpix_end);
	assign vpix = (vcount >= vpix_beg) & (vcount < vpix_end);
	assign hvpix = hpix & vpix;
	
	assign video_go = (hcount >= (hpix_beg - go_offs - x_offs)) & (hcount < (hpix_end - go_offs)) & vpix;
	
	assign line_start = (hcount == (HPERIOD - 1));
	assign frame_start = (hcount == (HPERIOD - 1)) & (vcount == (VPERIOD - 1));
	assign pix_start = (hcount == (hpix_beg - 1 - x_offs));
	
	assign int_start = (hcount == HINT_BEG) & (vcount == VINT_BEG);
	
	
	always @(posedge clk)
	begin
		hsync <= ~hs_vga;
		vsync <= ~vs;
		csync <= ~(vs ^ hs);
	end


endmodule
