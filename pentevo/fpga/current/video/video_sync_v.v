`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// vertical blank, sync and window. H is period of horizontal sync;
// from the last non-blanked line:
// 3H is pre-blank,
// 2.xxH is vertical sync (slightly more than 2H, all hsync edges preserved)
// vblank is total of 25H
//
// refactored by TS-Labs


module video_sync_v(

	input  wire        clk,

	input  wire        hsync_start, // synchronizing signal
	input  wire        line_start,  // to end vsync some time after hsync has ended

	input  wire        hint_start,

	input  wire [1:0]  rres,	//raster Y resolution 00=192/01=200/10=240/11=288

	output reg         vblank,
	output reg         vsync,

	output reg         tpref,	//starts 16 lines before vpix to prefetch tiles & Xscrolls
								//end 8 lines before end of vpix

	output reg         int_start, // one-shot positive pulse marking beginning of INT for Z80

	output reg         vpix // vertical picture marker: active when there is line with pixels in it, not just a border. changes with hsync edge
);





	localparam VBLNK_BEG = 9'd00;
	localparam VSYNC_BEG = 9'd08;
	localparam VSYNC_END = 9'd11;
	localparam VBLNK_END = 9'd32;

	localparam INT_BEG = 9'd0;

	// 192	=>	32-48-192-32
	localparam VPIX_BEG_192 = 9'd080;
	localparam VPIX_END_192 = 9'd272;

	// 200	=>	32-44-200-44
	localparam VPIX_BEG_200 = 9'd076;
	localparam VPIX_END_200 = 9'd276;

	// 240	=>	32-24-240-24
	localparam VPIX_BEG_240 = 9'd056;
	localparam VPIX_END_240 = 9'd296;

	// 288	=>	32-0-288-0
	localparam VPIX_BEG_288 = 9'd032;
	localparam VPIX_END_288 = 9'd320;

	localparam VPERIOD = 9'd320; // pentagono foreva!


	reg [8:0] vcount;

	reg [8:0] vp_beg, vp_end;
	
	always @*
	begin
		case (rres)
		2'b00 : begin
					assign vp_beg = VPIX_BEG_192;
					assign vp_end = VPIX_END_192;
				end
		2'b01 : begin
					assign vp_beg = VPIX_BEG_200;
					assign vp_end = VPIX_END_200;
				end
		2'b10 : begin
					assign vp_beg = VPIX_BEG_240;
					assign vp_end = VPIX_END_240;
				end
		2'b11 : begin
					assign vp_beg = VPIX_BEG_288;
					assign vp_end = VPIX_END_288;
				end
		default : begin
					assign vp_beg = VPIX_BEG_192;
					assign vp_end = VPIX_END_192;
				end
		endcase
	end
	

	//simulation
	initial
	begin
		vcount = 9'd0;
		vsync = 1'b0;
		vblank = 1'b0;
		vpix = 1'b0;
		int_start = 1'b0;
	end

		
	//vert counter
	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==(VPERIOD-9'd1) )
			vcount <= 9'd0;
		else
			vcount <= vcount + 9'd1;
	end

	
	//vblank
	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==VBLNK_BEG )
			vblank <= 1'b1;
		else if( vcount==VBLNK_END )
			vblank <= 1'b0;
	end

	
	//vsync
	always @(posedge clk)
	begin
		if( (vcount==VSYNC_BEG) && hsync_start )
			vsync <= 1'b1;
		else if( (vcount==VSYNC_END) && line_start  )
			vsync <= 1'b0;
	end


	//tpref
	always @(posedge clk) if( hsync_start )
	begin
		if (vcount == (vp_beg - 9'd16)
			tpref <= 1'b1;
		else if (vcount == (vp_end - 9'd8)
			tpref <= 1'b0;
	end


	//INT
	always @(posedge clk)
	begin
		if( (vcount==INT_BEG) && hint_start )
			int_start <= 1'b1;
		else
			int_start <= 1'b0;
	end


	//vpix
	always @(posedge clk) if( hsync_start )
	begin
		if (vcount == vp_beg)
			vpix <= 1'b1;
		else if (vcount == vp_end)
			vpix <= 1'b0;
	end


endmodule

