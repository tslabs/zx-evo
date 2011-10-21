`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
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

// vertical blank, sync and window. H is period of horizontal sync;
// from the last non-blanked line:
// 3H is pre-blank,
// 2.xxH is vertical sync (slightly more than 2H, all hsync edges preserved)
// vblank is total of 25H


module video_sync(

	input  wire        clk, q0,

	input  wire        init, // one-pulse strobe read at c3==1, initializes phase
	                         // this is mainly for phasing with CPU clock 3.5/7 MHz
	                         // still not used, but this may change anytime

	input  wire        c3,     // working strobes from DRAM controller (7MHz)
	input  wire        c2,


	// modes inputs
	input  wire        mode_atm_n_pent,
	input  wire        mode_a_text,


	output reg         hblank,
	output reg         hsync,

	output reg         line_start,  // 1 video cycle prior to actual start of visible line
	output reg         hsync_start, // 1 cycle prior to beginning of hsync: used in frame sync/blank generation
	                                // these signals coincide with c3

	output reg         hint_start, // horizontal position of INT start, for fine tuning

	output reg         scanin_start,

	output reg         hpix, // marks gate during which pixels are outting

	                                // these signals turn on and turn off 'go' signal
	output reg         fetch_start, // 18 cycles earlier than hpix, coincide with c3
	output reg         fetch_end,    // --//--

	
	output reg         vblank,
	output reg         vsync,

	output reg         int_start, // one-shot positive pulse marking beginning of INT for Z80

	output reg         vpix // vertical picture marker: active when there is line with pixels in it, not just a border. changes with hsync edge
	
);


	localparam HBLNK_BEG = 9'd00;
	localparam HSYNC_BEG = 9'd10;
	localparam HSYNC_END = 9'd43;
	localparam HBLNK_END = 9'd88;

	// pentagon (x256)
	localparam HPIX_BEG_PENT = 9'd140; // 52 cycles from line_start to pixels beginning
	localparam HPIX_END_PENT = 9'd396;

	// atm (x320)
	localparam HPIX_BEG_ATM = 9'd108; // 52 cycles from line_start to pixels beginning
	localparam HPIX_END_ATM = 9'd428;


	localparam FETCH_FOREGO = 9'd18; // consistent with older go_start in older fetch.v:
	                                 // actual data starts fetching 2 dram cycles after
					 // 'go' goes to 1, screen output starts another
					 // 16 cycles after 1st data bundle is fetched


	localparam SCANIN_BEG = 9'd88; // when scan-doubler starts pixel storing

	localparam HINT_BEG = 9'd2;


	localparam HPERIOD = 9'd448;


	reg [8:0] hcount;


	// for simulation only
	//
	initial
	begin
		hcount = 9'd0;
		hblank = 1'b0;
		hsync = 1'b0;
		line_start = 1'b0;
		hsync_start = 1'b0;
		hpix = 1'b0;
	end




	always @(posedge clk) if( c3 )
	begin
            if( init || (hcount==(HPERIOD-9'd1)) )
            	hcount <= 9'd0;
            else
            	hcount <= hcount + 9'd1;
	end



	always @(posedge clk) if( c3 )
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


	always @(posedge clk) if (q0)
	begin
		if( c2 )
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
			hsync_start  <= 1'b0;
			line_start   <= 1'b0;
			scanin_start <= 1'b0;
		end
	end



	wire fetch_start_time, fetch_start_condition;
	wire fetch_end_condition;

	reg [3:0] fetch_start_wait;


	assign fetch_start_time = (mode_atm_n_pent                  ?
	                          (HPIX_BEG_ATM -FETCH_FOREGO-9'd4) :
	                          (HPIX_BEG_PENT-FETCH_FOREGO-9'd4) ) == hcount;

	always @(posedge clk) if( c3 )
		fetch_start_wait[3:0] <= { fetch_start_wait[2:0], fetch_start_time };

	assign fetch_start_condition = mode_a_text ? fetch_start_time  : fetch_start_wait[3];

	always @(posedge clk) if (q0)
	if( c2 && fetch_start_condition )
		fetch_start <= 1'b1;
	else
		fetch_start <= 1'b0;




	assign fetch_end_time = (mode_atm_n_pent             ?
	                        (HPIX_END_ATM -FETCH_FOREGO) :
	                        (HPIX_END_PENT-FETCH_FOREGO) ) == hcount;

	always @(posedge clk) if (q0)
	if( c2 && fetch_end_time )
		fetch_end <= 1'b1;
	else
		fetch_end <= 1'b0;





	always @(posedge clk) if (q0)
	begin
		if( c2 && (hcount==HINT_BEG) )
			hint_start <= 1'b1;
		else
			hint_start <= 1'b0;
	end


	always @(posedge clk) if( c3 )
	begin
		if( hcount==(mode_atm_n_pent ? HPIX_BEG_ATM : HPIX_BEG_PENT) )
			hpix <= 1'b1;
		else if( hcount==(mode_atm_n_pent ? HPIX_END_ATM : HPIX_END_PENT) )
			hpix <= 1'b0;
	end


	

	localparam VBLNK_BEG = 9'd00;
	localparam VSYNC_BEG = 9'd08;
	localparam VSYNC_END = 9'd11;
	localparam VBLNK_END = 9'd32;

	localparam INT_BEG = 9'd0;

	// pentagon (x192)
	localparam VPIX_BEG_PENT = 9'd080;//9'd064;
	localparam VPIX_END_PENT = 9'd272;//9'd256;

	// ATM (x200)
	localparam VPIX_BEG_ATM = 9'd076;//9'd060;
	localparam VPIX_END_ATM = 9'd276;//9'd260;

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

	always @(posedge clk) if (q0) if( hsync_start )
	begin
		if( vcount==(VPERIOD-9'd1) )
			vcount <= 9'd0;
		else
			vcount <= vcount + 9'd1;
	end



	always @(posedge clk) if (q0) if( hsync_start )
	begin
		if( vcount==VBLNK_BEG )
			vblank <= 1'b1;
		else if( vcount==VBLNK_END )
			vblank <= 1'b0;
	end


	always @(posedge clk) if (q0)
	begin
		if( (vcount==VSYNC_BEG) && hsync_start )
			vsync <= 1'b1;
		else if( (vcount==VSYNC_END) && line_start  )
			vsync <= 1'b0;
	end


	always @(posedge clk) if (q0)
	begin
		if( (vcount==INT_BEG) && hint_start )
			int_start <= 1'b1;
		else
			int_start <= 1'b0;
	end



	always @(posedge clk) if (q0) if( hsync_start )
	begin
		if( vcount==(mode_atm_n_pent ? VPIX_BEG_ATM : VPIX_BEG_PENT) )
			vpix <= 1'b1;
		else if( vcount==(mode_atm_n_pent ? VPIX_END_ATM : VPIX_END_PENT) )
			vpix <= 1'b0;
	end




endmodule

