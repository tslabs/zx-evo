`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// fetches video data for renderer

// a special case here is to prefetch text symbol images from fontrom.
//  requires text attrs to be fetched last so that there are 4 cycles
//  to re-fetch fontrom

module video_fetch(

	input  wire        clk, // 28 MHz clock


	input  wire        cend,     // general
	input  wire        pre_cend, //        synchronization

	input  wire        vpix, // vertical window

	input  wire        fetch_start, // fetching start and stop
	input  wire        fetch_end,   //

	output reg         fetch_sync,     // 1 cycle after cend


	input  wire [15:0] video_data,   // video data receiving from dram arbiter
	input  wire        video_strobe, //
	output reg         video_go, // indicates need for data

	output reg  [63:0] pic_bits // picture bits -- data for renderer

	// currently, video_fetch assigns that there are only 1/8 and 1/4
	// bandwidth. !!needs correction for higher bandwidths!!


);
	reg [3:0] fetch_sync_ctr; // generates fetch_sync to synchronize
	                          // fetch cycles (each 16 dram cycles long)
	                          // fetch_sync coincides with cend

	reg [1:0] fetch_ptr; // pointer to fill pic_bits buffer
	reg       fetch_ptr_clr; // clears fetch_ptr


	reg [15:0] fetch_data [0:3]; // stores data fetched from memory

	// fetch window
	always @(posedge clk)
		if( fetch_start && vpix )
			video_go <= 1'b1;
		else if( fetch_end )
			video_go <= 1'b0;



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




	// ACHTUNG-NAEBACHTUNG!!!!
	// video_strobe coincides with cend,
	// fetch_sync ALSO coincides with cend!!!
	// so if last word is read during last dram cycle of 8,
	// it will be fucked!!!111
	//
	// fix:
	//  requires extra MUXes and conditions in pic_bits, loading
	//  last portion directly into pic_bits from video_data
	// alterantive fix:
	//  make two fetch buffers with rotation:
	//  while one is filled with data, second goes to renderer.
	//  requires extra MUXes as well.
	// one more fix:
	//  let pic_bits load 1 28MHz clock cycle later,
	//  so that fetch_data is correctly updated at that moment.
	//  this fix is OK only for <=14MHz-pixelclock modes and
	//  will require more shifting of signals in 'video_palframe'
	//
	// last fix is preferable - doing it


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


