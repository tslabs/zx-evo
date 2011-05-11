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
//	- xs


module video_addrgen(

	input  wire        clk,			// 28 MHz clock


	output reg  [20:0] video_addr,	// DRAM arbiter signals
	input  wire        video_next,	//

	input  wire        int_start,
	input  wire        hsync_start,
	input  wire        line_start,
	input  wire        yctr_init,
	input  wire        vpix,

	input  wire        scr_page,
	input  wire [7:0]  tp, tgp0, tgp1, fp,
	input  wire	[1:0]  fa,
	input  wire        mode_zx,			// decoded modes
	input  wire        mode_tm,
	input  wire        mode_tp1en
	

);


	//gfx counters
	reg [12:0] zxctr;	//zx_mode
	reg [7:0] xctr;		//hor counter for tiles and tile gfx
	reg [8:0] yctr;		//ver counter for tiles and tile gfx
	reg [1:0] tfsel;	//tile FIFO select

	
	// zx mode:
	// [0] - attr or pix
	// [4:1] - horiz pos 0..15 (words)
	// [12:5] - vert pos

	always @(posedge clk)
	if (int_start)
		zxctr <= 0;
	else if (video_next)
		zxctr <= zxctr + 1;


	// tile mode:
	//
	// xctr (at tiles fetching):                   	xctr (at GFX fetching):
	// [0] - tile plane 0 or 1                     	[0] - tile plane 0 or 1
	// [3:1] - tx[2:0];                            	[7:1] - adder to xcnt
	// [4] - 0-tiles or 1-Xsrolls fetching

	// xs:
	// [1:0] - pixel select
	// [8:2] - adder to xctr
	//
	// xcnt (at GFX fetching):
	// [0] - word select 0 or 1 of tile
	// [6:1] - tx;

	// yctr (at tiles fetching):					yctr (at GFX fetching):
	// [2:0] = tx[5:3] - 3 MSBs in tile X coord    	[8:0] - Y line number

	always @(posedge clk)
	if (hsync_start || line_start)
		xctr <= ((vtfetch && mode_tm) || line_start) ? 8'd0 : 8'd16;	//if tiles aren't read, counter is set right to Xs.
	else if (video_next)
		xctr <= xctr + (mode_tp1en ? 2'b1 : 2'b2);		//if 2nd tiles plane is off, counter skips over bit0

	always @(posedge clk) if (hsync_start)
	if (yctr_init)
	begin
		yctr <= 9'd0;
		tbuf <= 2'b0;
	end
	else
	begin
		yctr <= yctr + 9'd1;
		if (yctr[2:0] == 3'd7)
			tbuf <= tbuf + 2'b1;
	end
	

	wire [8:0] ty = ys[8:3] + yctr[8:3];
	wire [6:0] xcnt = xctr[7:1] + xs[8:2];
	
	wire tpn;
	wire [10:0] tnum = ~tpn ? tn0 : tn1;
	wire [7:0] tgp = (~tpn ? tgp0 : tgp1) + tnum[10:9];
	wire [3:0] tptr;
	
	wire [11:0] addr_zx_pix  = { zxctr[12:11], zxctr[7:5], zxctr[10:8], zxctr[4:1] };
	wire [11:0] addr_zx_attr = { 3'b110, zxctr[12:8], zxctr[4:1] };

	wire [20:0] addr_zx = { 6'b000001, scr_page, 2'b10, ( zxctr[0] ? addr_zx_attr : addr_zx_pix ) };

	wire 
	wire [20:0] addr_tm_gfx = {tgp, tnum[8:0], tptr};		// 8.9.4
	wire [20:0] addr_tm_tile = {tp, ty, tx, tpn}; 			// 8.6.6.1
	wire [20:0] addr_tm_xs = {fp, fa, 1'b0, tpn, yctr};		// 8.2.1.1.9

	wire [20:0] addr_tm =	( {21{video_gfx	}} & addr_tm_gfx	)|
							( {21{video_tile}} & addr_tm_tile	)|
							( {21{video_xs	}} & addr_tm_xs		);

	wire ldaddr = 	video_next ||
					(mode_zx && int_start) ||
					(mode_tm && )
					;	//ATTENTION!!!!!!! address for the 1st fetch latched!!!
	
	always @(posedge clk) if (ldaddr)
	begin
		video_addr <= 	( {21{mode_zx	}} & addr_zx  )	|
						( {21{mode_tm	}} & addr_tm  )	;
	end


endmodule

