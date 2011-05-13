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
	input  wire        mode_tp1en,
	
);


	//gfx counters
	reg [12:0] zxctr;	//zx_mode
	reg [7:0] xctr;		//hor counter for tiles and tile gfx
	reg [8:0] yctr;		//ver counter for tiles and tile gfx
	reg [1:0] tfsel;	//tile FIFO select

	wire fetch_start = gfetch_start || tfetch_start || xsfetch_start;
	reg fetch_start_r;
	
	always @(posedge clk);
		fetch_start_r <= fetch_start;


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

	// xctr (at tiles fetching):               	xctr (at gfx fetching):			xctr (at xscrolls fetching):   
	// [0] - tile plane		                 	[0] - tile plane		     	[0] - tile plane
	// [3:1] - posx[2:0];                      	[7:1] - adder to x

	// xs:	data from xscrolls array
	// [1:0] - pixel select
	// [8:2] - adder to xctr[7:1]

	// x:	sum of xctr & xs
	// [0] - tile word select
	// [6:1] - posx;

	// yctr (at tiles fetching):
	// [2:0] = posx[5:3]				    
	// [8:3] - adder to ys[8:3]
	
	// ys:	data from yscrolls array
	// [1:0] - pixel select
	// [8:2] - adder to xctr[7:1]

	// y:	sum of yctr & ys
	// [2:0] - tile line select
	// [8:3] - posy[5:0]

	always @(posedge clk)
	if (fetch_start)
		xctr <= xsfetch_start ? 8'd16 : 8'd0;
	else if (video_next)
		xctr <= xctr + (mode_tp1en ? 2'd1 : 2'd2);		//if 2nd tiles plane is off, counter skips over bit0

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
	

	wire [8:0] ty = ys[8:3] + yctr[8:3];		// tile pos Y
	wire [6:0] x = xctr[7:1] + xs[8:2];
	
	wire tpn;
	wire [10:0] tnum = ~tpn ? tn0 : tn1;
	wire [7:0] tgp = (~tpn ? tgp0 : tgp1) + tnum[10:9];
	wire [3:0] tptr;
	

	// zx mode addresses
	wire [11:0] addr_zx_pix  = { zxctr[12:11], zxctr[7:5], zxctr[10:8], zxctr[4:1] };
	wire [11:0] addr_zx_attr = { 3'b110, zxctr[12:8], zxctr[4:1] };

	wire [20:0] addr_zx = { 6'b000001, scr_page, 2'b10, ( zxctr[0] ? addr_zx_attr : addr_zx_pix ) };

	
	// tile mode addresses
	wire [20:0] addr_tm_gfx = {tgp, tnum[8:0], tptr};		// 8.9.4
	wire [20:0] addr_tm_tile = {tp, ty, tx, tpn}; 			// 8.6.6.1
	wire [20:0] addr_tm_xs = {fp, fa, 1'b0, tpn, yctr};		// 8.2.1.1.9

	wire [20:0] addr_tm =	( {21{video_gfx	}} & addr_tm_gfx	)|
							( {21{video_tile}} & addr_tm_tile	)|
							( {21{video_xs	}} & addr_tm_xs		);

							
	wire ldaddr = (video_next || xfetch_start_r);
	
	always @(posedge clk) if (ldaddr)
	begin
		video_addr <= 	( {21{mode_zx	}} & addr_zx  )	|
						( {21{mode_tm	}} & addr_tm  )	;
	end


endmodule

