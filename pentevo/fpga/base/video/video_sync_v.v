`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// vertical blank, sync and window. H is period of horizontal sync;
// from the last non-blanked line:
// 3H is pre-blank,
// 2.xxH is vertical sync (slightly more than 2H, all hsync edges preserved)
// vblank is total of 25H

module video_sync_v(

	input  wire        clk,

	input  wire        hsync_start, // synchronizing signal
	input  wire        line_start,  // to end vsync some time after hsync has ended

	input  wire        hint_start,



	// atm video mode input
	input  wire        mode_atm_n_pent,

	input  wire 	   mode_60hz,


	output reg         vblank,
	output reg         vsync,

	output reg         int_start, // one-shot positive pulse marking beginning of INT for Z80

	output reg         vpix // vertical picture marker: active when there is line with pixels in it, not just a border. changes with hsync edge
);





	localparam VBLNK_BEG = 9'd00;
	localparam VSYNC_BEG = 9'd08;
	localparam VSYNC_END = 9'd11;
	localparam VBLNK_END = 9'd32;

	localparam INT_BEG = 9'd0;

	// pentagon (x192)
	localparam VPIX_BEG_PENT = 9'd080;
	localparam VPIX_END_PENT = 9'd272;

	// ATM (x200)
	localparam VPIX_BEG_ATM = 9'd076;
	localparam VPIX_END_ATM = 9'd276;

	localparam VPERIOD = 9'd320; // pentagono foreva!

	// ntsc
	localparam VSYNC60_BEG = 9'd04;
	localparam VSYNC60_END = 9'd07;
	localparam VBLNK60_END = 9'd22;
	// pentagon (x192)
	localparam VPIX60_BEG_PENT = 9'd046;
	localparam VPIX60_END_PENT = 9'd238;
	// ATM (x200)
	localparam VPIX60_BEG_ATM = 9'd042;
	localparam VPIX60_END_ATM = 9'd242;
	//
	localparam VPERIOD60 = 9'd262;

	reg [8:0] vcount;
	reg mode60;




	initial
	begin
		vcount = 9'd0;
		vsync = 1'b0;
		vblank = 1'b0;
		vpix = 1'b0;
		int_start = 1'b0;
	end

	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==((mode60?VPERIOD60:VPERIOD)-9'd1) )
		begin
			vcount <= 9'd0;
			mode60 <= mode_60hz;
		end
		else
			vcount <= vcount + 9'd1;
	end



	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==VBLNK_BEG )
			vblank <= 1'b1;
		else if( vcount==(mode60?VBLNK60_END:VBLNK_END) )
			vblank <= 1'b0;
	end


	always @(posedge clk)
	begin
		if( (vcount==(mode60?VSYNC60_BEG:VSYNC_BEG)) && hsync_start )
			vsync <= 1'b1;
		else if( (vcount==(mode60?VSYNC60_END:VSYNC_END)) && line_start  )
			vsync <= 1'b0;
	end


	always @(posedge clk)
	begin
		if( (vcount==INT_BEG) && hint_start )
			int_start <= 1'b1;
		else
			int_start <= 1'b0;
	end



	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==(mode60?(mode_atm_n_pent ? VPIX60_BEG_ATM : VPIX60_BEG_PENT):(mode_atm_n_pent ? VPIX_BEG_ATM : VPIX_BEG_PENT)) )
			vpix <= 1'b1;
		else if( vcount==(mode60?(mode_atm_n_pent ? VPIX60_END_ATM : VPIX60_END_PENT):(mode_atm_n_pent ? VPIX_END_ATM : VPIX_END_PENT)) )
			vpix <= 1'b0;
	end


endmodule

