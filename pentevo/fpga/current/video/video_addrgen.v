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
	
	input  wire	[15:0] xs [0:1],
	input  wire [15:0] 
	
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


	// zx mode addresses
	
	wire [11:0] addr_zx_pix  = { zxctr[12:11], zxctr[7:5], zxctr[10:8], zxctr[4:1] };
	wire [11:0] addr_zx_attr = { 3'b110, zxctr[12:8], zxctr[4:1] };

	wire [20:0] addr_zx = { 6'b000001, scr_page, 2'b10, ( zxctr[0] ? addr_zx_attr : addr_zx_pix ) };


	// tile mode:

	// xctr (at tiles fetching):               	xctr (at gfx fetching):			xctr (at xscrolls fetching):   
	// [0] - tile plane		                 	[0] - tile plane		     	[0] - tile plane
	// [3:1] - posx[2:0];                      	[7:1] - adder to x

	// xs:	data from xscrolls
	// [1:0] - pixel select
	// [8:2] - adder to xctr[7:1]

	// x:	sum of xctr & xs
	// [0] - tile word select
	// [6:1] - posx;

	// yctr (at tiles fetching):
	// [2:0] = posx[5:3]				    
	// [8:3] - adder to ys[8:3]
	
	// ys:	data from yscrolls (at tiles fetching)
	// [2:0] - start line number
	// [8:3] - adder to yctr[8:3]

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
	

	// tile mode addresses

	wire tpn = xctr[0];
	wire [6:0] x = xctr[7:1] + xs[8:2];
	wire [8:0] y = {yctr[8:3] + ys[8:3], ys[2:0]};

	wire [10:0] tnum = ~tpn ? tn0 : tn1;
	wire [7:0] tgp = (~tpn ? tgp0 : tgp1) + tnum[10:9];
	wire [3:0] tptr;
	
	// Y Scroll Table Address							
	wire [8:0] ys_addr_tile = {1'b0, tpn, yctr[2:0], xctr[3:1], h_n_l};
	wire [8:0] ys_addr_gfx = {1'b0, tpn, x[6:1], h_n_l};
	wire [8:0] ys_addr = video_gfx ? ys_addr_gfx : ys_addr_tile;
	
	wire [20:0] addr_tm_tile = {tp, posy, yctr[2:0], xctr[3:1], tpn};	//Address for Tile Planes 																		
	wire [20:0] addr_tm_gfx = {tgp, tnum[8:0], tptr};
	wire [20:0] addr_tm_xs = {fp, fa, 1'b0, tpn, yctr};

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

