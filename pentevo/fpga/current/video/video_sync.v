// This module generates all video raster sync signals

module video_sync (

// clocks
	input wire clk,
	input wire q0,
	input wire c15,

// video syncs
	output wire hsync,
	output wire vsync,
	output wire csync,
	
// ZX controls
	output reg int_start

);

	
	assign hsync = ~hs_vga;
	assign vsync = ~vs;
	assign csync = ~(vs ^ hs);

	
	localparam HSYNC_BEG 	= 9'd10;
	localparam HSYNC_END 	= 9'd43;
	localparam HSYNCV0_BEG 	= 9'd5;
	localparam HSYNCV0_END 	= 9'd22;
	localparam HSYNCV1_BEG 	= HSYNCV0_BEG + HPERIOD/2;
	localparam HSYNCV1_END 	= HSYNCV0_END + HPERIOD/2;
	
	localparam HBLNK_BEG 	= 9'd00;
	localparam HBLNK_END 	= 9'd88;
	localparam HBLNKV0_BEG 	= 9'd00;
	localparam HBLNKV0_END 	= 9'd44;
	localparam HBLNKV1_BEG 	= HBLNKV0_BEG + HPERIOD/2;
	localparam HBLNKV1_END 	= HBLNKV0_END + HPERIOD/2;
	
	localparam HPIX_BEG_256 = 9'd140;
	localparam HPIX_END_256 = 9'd396;
	localparam HINT_BEG  	= 9'd2;
	localparam HPERIOD   	= 9'd448;

	localparam VSYNC_BEG 	= 9'd08;
	localparam VSYNC_END 	= 9'd11;
	localparam VBLNK_BEG 	= 9'd00;
	localparam VBLNK_END 	= 9'd32;
	
	localparam VPIX_BEG_192 = 9'd080;
	localparam VPIX_END_192 = 9'd272;
	localparam VINT_BEG  	= 9'd0;
	localparam VPERIOD   	= 9'd320;	// fucking pentagovn!!!

	
// counters
	reg [8:0] hcount = 0;
	reg [8:0] vcount = 0;
	reg hs		= 0;
	reg hs_vga	= 0;
	reg vs		= 0;
	reg hb		= 1;
	reg hb_vga	= 1;
	reg vb		= 1;

	
	always @(posedge clk) if (c15)
		if (hcount == (HPERIOD - 1))
			hcount <= 0;
		else
			hcount <= hcount + 1;

	
	always @(posedge clk)
	begin
		
		if (hcount == HSYNC_BEG)
			hs <= 1;
		else
		if (hcount == HSYNC_END)
			hs <= 0;

		if ((hcount == HSYNCV0_BEG) | (hcount == HSYNCV1_BEG))
			hs_vga <= 1;
		else
		if ((hcount == HSYNCV0_END) | (hcount == HSYNCV1_END))
			hs_vga <= 0;
			
		if (hcount == HBLNK_BEG)
			hb <= 1;
		else
		if (hcount == HBLNK_END)
			hb <= 0;

		if ((hcount == HBLNKV0_BEG) | (hcount == HBLNKV1_BEG))
			hb_vga <= 1;
		else
		if ((hcount == HBLNKV0_END) | (hcount == HBLNKV1_END))
			hb_vga <= 0;
			
	end


	always @(posedge clk) if (c15)
		if (hcount == (HPERIOD - 1))
			if (vcount == (VPERIOD - 1))
				vcount <= 0;
			else
				vcount <= vcount + 1;


	always @(posedge clk)
	begin
		
		if (vcount == VSYNC_BEG)
			vs <= 1;
		else
		if (vcount == VSYNC_END)
			vs <= 0;

		if (vcount == VBLNK_BEG)
			vb <= 1;
		else
		if (vcount == VBLNK_END)
			vb <= 0;

	end
	

	always @(posedge clk)
		int_start <= (hcount == HINT_BEG) & (vcount == VINT_BEG);




	
	

endmodule
