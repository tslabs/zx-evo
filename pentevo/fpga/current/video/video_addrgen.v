`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// address generation module for video data fetching
//
// refactored by TS-Labs
//
//
// Which addresses for GFX could be?
// zx:
//	- gfx
//	- attr
// tm:
//	- gfx
//	- ttab
//	- yscr


module video_addrgen(

	input  wire        clk, // 28 MHz clock


	output reg  [20:0] video_addr, // DRAM arbiter signals
	input  wire        video_next, //

	input  wire        line_start, // some video sync signals
	input  wire        int_start,  //
	input  wire        vpix,       //

	input  wire        scr_page, // which screen to use

	input  wire        mode_zx,			// decoded modes
	input  wire        mode_tm,     //
	input  wire        mode_tp1en     //

);


	wire line_init, frame_init;

	wire gnext,tnext,ldaddr;

	reg line_start_r;	//right after HBLANK
	reg frame_init_r;
	reg line_init_r;

	wire mode_tm = (mode_tm || mode_tp1en);
	
	always @(posedge clk)
		line_start_r <= line_start;

	assign line_init  = line_start_r & vpix;
	assign frame_init = int_start;

	reg [13:0] gctr;


	always @(posedge clk)
		frame_init_r <= frame_init;

	always @(posedge clk)
		line_init_r <= line_init;


	assign gnext = video_next | frame_init_r;
	assign ldaddr = gnext;


	// gfx counter
	// zx mode:
	// [0] - attr or pix
	// [4:1] - horiz pos 0..15 (words)
	// [12:5] - vert pos

	always @(posedge clk)
	if( frame_init )
		gctr <= 0;
	else if( gnext )
		gctr <= gctr + 1;


	// tile counters
	//
	// start reading tiles 16 lines before pix area
	//
	// read 64(128) tiles for each tiles row (8 lines by 8(16) tiles)
	// thus need 8 or 16 DRAM cycles per line at the beginning of HBLANK
	// at the b/w of 1/2 it takes us 32 7MHz clocks (pix)
	// also, need 1(2) cycle(s) to read YSCRL(s)
	//
	//	7MHz
	// |  00 |  01 |  02 | ~~~ |  28 |  29 |  30 |  31 |  32 |  33 |  34 |
	// |t0-00| ... |t1-00| ~~~ |t0-07| ... |t1-07| ... | ys0 | ... | ys1 |
	//
	// we must read whole gfx for both planes 8 pix before it's visible area
	//
	//	7MHz
	// |  80 |  81 |  82 |  83 |  84 |  85 |  86 | ~~~ | 438 | 439 | 440 |
	// |g0-00| ... |g1-00| ... |g0-01| ... |g1-01| ~~~ |g0-44| ... |g1-44|

	// tm mode:
	// [0] - tiles0/1
	// []


	wire [20:0] addr_zx;   // standard zx mode
	wire [20:0] addr_tm;   // tiles mode

	wire [11:0] addr_zx_pix;
	wire [11:0] addr_zx_attr;

	wire [11:0] addr_tm_gfx;
	wire [11:0] addr_tm_ttab;
	wire [11:0] addr_tm_yscr;


	assign addr_zx_pix  = { gctr[12:11], gctr[7:5], gctr[10:8], gctr[4:1] };
	assign addr_zx_attr = { 3'b110, gctr[12:8], gctr[4:1] };

	assign addr_zx =   { 6'b000001, scr_page, 2'b10, ( gctr[0] ? addr_zx_attr : addr_zx_pix ) };


	always @(posedge clk) if( ldaddr )
	begin
		video_addr <=
			( {21{mode_zx	}} & addr_zx  )	|
			( {21{mode_tm	}} & addr_tm  )	;
	end


endmodule

