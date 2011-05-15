`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// fetches video data for renderer

module video_fetch(

	input  wire        clk, // 28 MHz clock


	// strobes from DRAM controller (7MHz)
	input  wire        cend,
	input  wire        pre_cend,

	input wire         fetch_gfx,  	  	//gfx fetching window
	input wire         fetch_tile, 	  	//tiles fetching window
	input wire         fetch_xs,    	//xscrolls fetching window

	output reg         fetch_sync,     // 1 cycle after cend

	input wire         mode_zx,		// standard ZX mode
	input wire         mode_tm,		// tiles mode

	input  wire [15:0] video_data,   // video data receiving from dram arbiter
	input  wire        video_strobe, //
	output wire        video_go,	 // indicates need for data

	output reg [15:0]  pic_bits [5:0] // picture bits -- data for renderer

	input wire		   gfetch_start,	    //gfx fetching
	input wire		   gfetch_end,   
	input wire		   tfetch_start,    //tiles fetching
    input wire		   tfetch_end,
    input wire		   xsfetch_start, 	//Xscrolls fetching
	input wire		   xsfetch_end,
	
	output reg		   video_gfx,
	output reg		   video_tile,
	output reg		   video_xs,
	
	output reg [15:0]  xs	[0:1]

	
);


	reg [3:0] fetch_sync_ctr; // generates fetch_sync to synchronize
	                          // fetch cycles (each 16 dram cycles long)
	                          // fetch_sync coincides with cend

	reg [1:0] fetch_ptr; // pointer to fill pic_bits buffer
	reg       fetch_ptr_clr; // clears fetch_ptr


	// fetch windows
	wire gfetch_go = gfetch_start && vpix && !mode_brd;		// gfx fetch window: at gfx window and no BM
	wire tfetch_go = tfetch_start && vtfetch && mode_tm;	// tiles: 16 lines before start and 8 before end of gfx in TM
	wire xsfetch_go = xsfetch_start && ((vpix && mode_tm) || xints_en);		// Xscrolls: at gfx in TM or when xints on

	assign video_go = video_gfx || video_tile || video_xs;
	
	always @(posedge clk)
	begin
		if (gfetch_go)
			video_gfx <= 1'b1;
			
		if (tfetch_go)
			video_tile <= 1'b1;
			
		if (xsfetch_go)
			video_xs <= 1'b1;
			
		if (gfetch_end || tfetch_end || xsfetch_end || xs_stop)
		begin
			video_gfx <= 1'b0;
			video_tile <= 1'b0;
			video_xs <= 1'b0;
		end
	end
	
			
	// fetch sync counter
	always @(posedge clk) if( cend )
	begin
		if( fetch_start )
			fetch_sync_ctr <= 0;
		else
			fetch_sync_ctr <= fetch_sync_ctr + 1;
	end


	// fetch sync signal
	always @(posedge clk)
		if( (fetch_sync_ctr==1) && pre_cend )
			fetch_sync <= 1'b1;
		else
			fetch_sync <= 1'b0;


	// fetch_ptr clear signal
	always @(posedge clk)
		if( (fetch_sync_ctr==0) && pre_cend )
			fetch_ptr_clr <= 1'b1;
		else
			fetch_ptr_clr <= 1'b0;


	// buffer fill pointer
	always @(posedge clk)
		if( fetch_ptr_clr )
			fetch_ptr <= 0;
		else if( video_strobe )
			fetch_ptr <= fetch_ptr + 1;



	// store fetched data
	always @(posedge clk) if( video_strobe )
		fetch_data[fetch_ptr] <= video_data;


	// pass fetched data to renderer
	always @(posedge clk) if( fetch_sync )
	begin
		pic_bits[ 7:0 ] <= fetch_data[0][15:8 ];
		pic_bits[15:8 ] <= fetch_data[0][ 7:0 ];
		pic_bits[23:16] <= fetch_data[1][15:8 ];
		pic_bits[31:24] <= fetch_data[1][ 7:0 ];
		pic_bits[39:32] <= fetch_data[2][15:8 ];
		pic_bits[47:40] <= fetch_data[2][ 7:0 ];
		pic_bits[55:48] <= fetch_data[3][15:8 ];
		pic_bits[63:56] <= fetch_data[3][ 7:0 ];
	end

endmodule


