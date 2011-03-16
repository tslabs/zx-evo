`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// vertical blank, sync and window. H is period of horizontal sync;
// from the last non-blanked line:
// 3H is pre-blank,
// 2.xxH is vertical sync (slightly more than 2H, all hsync edges preserved)
// vblank is total of 25H

module syncv(

	input clk,

	input hsync_start, // synchronizing signal
	input line_start,  // to end vsync some time after hsync has ended

	input hint_start,


	output reg vblank,
	output reg vsync,

	output reg int_start, // one-shot positive pulse marking beginning of INT for Z80

	output reg vpix // vertical picture marker: active when there is line with pixels in it, not just a border. changes with hsync edge
);





	localparam VBLNK_BEG = 9'd00;
	localparam VSYNC_BEG = 9'd08;
	localparam VSYNC_END = 9'd11;
	localparam VBLNK_END = 9'd32;

	localparam INT_BEG = 9'd0;

	localparam VPIX_BEG = 9'd080;//9'd064;
	localparam VPIX_END = 9'd272;//9'd256;

	localparam VPERIOD = 9'd320; // pentagono foreva!


	reg [8:0] vcount;




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
		if( vcount==(VPERIOD-9'd1) )
			vcount <= 9'd0;
		else
			vcount <= vcount + 9'd1;
	end



	always @(posedge clk) if( hsync_start )
	begin
		if( vcount==VBLNK_BEG )
			vblank <= 1'b1;
		else if( vcount==VBLNK_END )
			vblank <= 1'b0;
	end


	always @(posedge clk)
	begin
		if( (vcount==VSYNC_BEG) && hsync_start )
			vsync <= 1'b1;
		else if( (vcount==VSYNC_END) && line_start  )
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
		if( vcount==VPIX_BEG )
			vpix <= 1'b1;
		else if( vcount==VPIX_END )
			vpix <= 1'b0;
	end


endmodule

