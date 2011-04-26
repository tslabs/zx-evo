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
//	- tile
//	- yscr


module video_addrgen(

	input  wire        clk,			// 28 MHz clock


	output reg  [20:0] video_addr,	// DRAM arbiter signals
	input  wire        video_next,	//

	input  wire        int_start,
	input  wire        hsync_start,
	input  wire        line_start,
	input  wire        ycnt_init,
	input  wire        vpix,
	input  wire        htfetch,

	input  wire        scr_page,
	input  wire [7:0]  tp, tgp0, tgp1, fp,
	input  wire	[1:0]  fa,
	input  wire        mode_zx,			// decoded modes
	input  wire        mode_tm,
	input  wire        mode_tp1en
	
	input wire         fetch_gfx,     //gfx fetching window
	input wire         fetch_tile,    //tiles/Xscrolls fetching window


);

	//gfx counters
	reg [12:0] zxcnt;
	reg [8:0] ycnt;
	reg [7:0] xcnt;

	// zx mode:
	// [0] - attr or pix
	// [4:1] - horiz pos 0..15 (words)
	// [12:5] - vert pos

	always @(posedge clk)
	if (int_start)
		zxcnt <= 0;
	else if (video_next)
		zxcnt <= zxcnt + 1;

	// tile mode:
	//
	// xcnt (at tiles fetching):
	// [0] - tile plane 0 or 1
	// [3:1] - tile count to fetch in current line
	// [4] - 0-tiles or 1-Xsrolls fetching
	//
	// xcnt (at GFX fetching):
	// [0] - tile plane 0 or 1
	// [1] - word 0 or 1 of tile
	// [7:2] = tx[2:0]
	// note: {1'b0, xcnt[7:1]} should be added by xs[8:2]
	// to obtain physical word pointer in tile
	//
	// ycnt (at tiles fetching):
	// [2:0] = tx[5:3] - higher 3 bits in tile X coordinate
	//
	// ycnt (at GFX fetching):
	// [8:0] - Y line number
	//
	// note: Xscrolls should be taken from ycnt-16 lines
	
	
	always @(posedge clk) if (hsync_start)
	if (ycnt_init)
		ycnt <= 0;
	else
		ycnt <= ycnt + 1;
	
	always @(posedge clk)
	if (hsync_start || line_start)
		xcnt <= 0;
	else if (video_next)
		xcnt <= xctr + (mode_tp1en ? 2'b1 : 2'b2);

		
	wire tpn;
	wire [10:0] tnum = ~tpn ? tn0 : tn1;
	wire [7:0] tgp = (~tpn ? tgp0 : tgp1) + tnum[10:9];
	wire [3:0] tptr;
	
	wire [11:0] addr_zx_pix  = { zxcnt[12:11], zxcnt[7:5], zxcnt[10:8], zxcnt[4:1] };
	wire [11:0] addr_zx_attr = { 3'b110, zxcnt[12:8], zxcnt[4:1] };

	wire [20:0] addr_zx = { 6'b000001, scr_page, 2'b10, ( zxcnt[0] ? addr_zx_attr : addr_zx_pix ) };

	wire 
	wire [20:0] addr_tm_gfx = {tgp, tnum[8:0], tptr};		// 8.9.4
	wire [20:0] addr_tm_tile = {tp, ty, tx, tpn}; 			// 8.6.6.1
	wire [20:0] addr_tm_xs = {fp, fa, 1'b0, tpn, ycnt};		// 8.2.1.1.9

	wire [20:0] addr_tm =	( {21{fetch_gfx	}} & addr_tm_gfx	)|
							( {21{fetch_tile}} & addr_tm_tile	)|
							( {21{fetch_xs	}} & addr_tm_xs		);

	always @(posedge clk) if (ldaddr)
	begin
		video_addr <= 	( {21{mode_zx	}} & addr_zx  )	|
						( {21{mode_tm	}} & addr_tm  )	;
	end


endmodule

