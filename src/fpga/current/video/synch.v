`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// generates horizontal sync, blank and video start strobe, horizontal window
//
// =\                  /=========||...
// ==\                /==========||...
// ====---     -------===========||...
//    |  \   / |      |
//    |   ---  |      |
//    |  |   | |      |
//    0  t1  | t3     t4
//           t2
// at 0, video ends and blank begins
//    t1 = 10 clocks (@7MHz), sync begins
// t2-t1 = 33 clocks
// t3-t2 = 41 clocks, then video starts
//
// repetition period = 448 clocks


module synch(

	input clk,

	input init, // one-pulse strobe read at cend==1, initializes phase
	            // this is mainly for phasing with CPU clock 3.5/7 MHz

	input cend, // working strobes (7MHz)
	input pre_cend,


	output reg hblank,
	output reg hsync,

	output reg line_start,  // 1 video cycle prior to actual start of visible line
	output reg hsync_start, // 1 cycle prior to beginning of hsync: used in frame sync/blank generation
	                        // these signals coincide with cend

	output reg hint_start, // horizontal position of INT start, for fine tuning

	output reg scanin_start,

	output reg hpix // marks gate during which pixels are outting

);


	localparam HBLNK_BEG = 9'd00;
	localparam HSYNC_BEG = 9'd10;
	localparam HSYNC_END = 9'd43;
	localparam HBLNK_END = 9'd88;

	localparam HPIX_BEG = 9'd140; // 52 cycles from line_start to pixels beginning
	localparam HPIX_END = 9'd396;

	localparam SCANIN_BEG = 9'd88; // when scan-doubler starts pixel storing

	localparam HINT_BEG = 9'd443;


	localparam HPERIOD = 9'd448;


	reg [8:0] hcount;




	initial
	begin
		hcount = 9'd0;
		hblank = 1'b0;
		hsync = 1'b0;
		line_start = 1'b0;
		hsync_start = 1'b0;
		hpix = 1'b0;
	end

	always @(posedge clk) if( cend )
	begin
            if( init || (hcount==(HPERIOD-9'd1)) )
            	hcount <= 9'd0;
            else
            	hcount <= hcount + 9'd1;
	end



	always @(posedge clk) if( cend )
	begin
		if( hcount==HBLNK_BEG )
			hblank <= 1'b1;
		else if( hcount==HBLNK_END )
			hblank <= 1'b0;


		if( hcount==HSYNC_BEG )
			hsync <= 1'b1;
		else if( hcount==HSYNC_END )
			hsync <= 1'b0;
	end


	always @(posedge clk)
	begin
		if( pre_cend )
		begin
			if( hcount==HSYNC_BEG )
				hsync_start <= 1'b1;

			if( hcount==HBLNK_END )
				line_start <= 1'b1;

			if( hcount==SCANIN_BEG )
				scanin_start <= 1'b1;
		end
		else
		begin
			hsync_start <= 1'b0;
			line_start <= 1'b0;
			scanin_start <= 1'b0;
		end
	end


	always @(posedge clk)
	begin
		if( pre_cend && (hcount==HINT_BEG) )
			hint_start <= 1'b1;
		else
			hint_start <= 1'b0;
	end


	always @(posedge clk) if( cend )
	begin
		if( hcount==HPIX_BEG )
			hpix <= 1'b1;
		else if( hcount==HPIX_END )
			hpix <= 1'b0;
	end


endmodule

