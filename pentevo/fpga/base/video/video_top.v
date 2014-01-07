`include "../include/tune.v"

// Pentevo project (c) NedoPC 2010,2011,2012
//
// top module for video output.
//
//
// note: the only bandwidths currently in use are 1/8 and 1/4.

module video_top(

	input  wire        clk, // 28 MHz clock


	// external video outputs
	output wire [ 1:0] vred,
	output wire [ 1:0] vgrn,
	output wire [ 1:0] vblu,
	output wire        vhsync,
	output wire        vvsync,
	output wire        vcsync,


	// aux video inputs
	input  wire [ 3:0] zxborder, // border zxcolor


	// config inputs
	input  wire [ 1:0] pent_vmode, // 2'b00 - standard ZX
	                               // 2'b01 - hardware multicolor
	                               // 2'b10 - pentagon 16 colors
	                               // 2'b11 - not defined yet

	input  wire [ 2:0] atm_vmode,  // 3'b011 - zx modes (pent_vmode is active)
	                               // 3'b010 - 640x200 hardware multicolor
	                               // 3'b000 - 320x200 16 colors
	                               // 3'b110 - 80x25 text mode
	                               // 3'b??? (others) - not defined yet



	input  wire        scr_page,   // screen page (bit 3 of 7FFD)

	input  wire        vga_on,     // vga mode ON - scandoubler activated

	input  wire 	   mode_60hz,

	// memory synchronization inputs
	input  wire        cbeg,
	input  wire        post_cbeg,
	input  wire        pre_cend,
	input  wire        cend,


	// memory arbiter video port connection
	input  wire        video_strobe,
	input  wire        video_next,
	output wire [20:0] video_addr,
	input  wire [15:0] video_data,
	output wire [ 1:0] video_bw,
	output wire        video_go,


	// atm palette write strobe adn data
	input  wire        atm_palwr,
	input  wire [ 5:0] atm_paldata,


	output wire        int_start,



	input  wire [10:0] fnt_a,
	input  wire [ 7:0] fnt_d,
	input  wire        fnt_wr,

	output wire [ 5:0] palcolor, // for palette readback

	output wire [ 7:0] fontrom_readback
);

	// these decoded in video_modedecode.v
	wire mode_atm_n_pent;
	wire mode_zx;
	wire mode_p_16c;
	wire mode_p_hmclr;
	wire mode_a_hmclr;
	wire mode_a_16c;
	wire mode_a_text;
	wire mode_a_txt_1page;
	wire mode_pixf_14;



	// synchronization
	wire hsync_start;
	wire line_start;
	wire hint_start;


	wire vblank;
	wire hblank;

	wire vpix;
	wire hpix;

	wire vsync;
	wire hsync;

	wire vga_hsync;

	wire scanin_start;
	wire scanout_start;



	wire fetch_start;
	wire fetch_end;
	wire fetch_sync;


	wire [63:0] pic_bits;


	wire [3:0] pixels;


	wire [5:0] color;
	wire [5:0] vga_color;


	wire [2:0] typos;



	// decode video modes
	video_modedecode video_modedecode(

		.clk(clk),

		.pent_vmode(pent_vmode),
		.atm_vmode (atm_vmode),

		.mode_atm_n_pent (mode_atm_n_pent ),
		.mode_zx         (mode_zx         ),
		.mode_p_16c      (mode_p_16c      ),
		.mode_p_hmclr    (mode_p_hmclr    ),
		.mode_a_hmclr    (mode_a_hmclr    ),
		.mode_a_16c      (mode_a_16c      ),
		.mode_a_text     (mode_a_text     ),
		.mode_a_txt_1page(mode_a_txt_1page),

		.mode_pixf_14(mode_pixf_14),

		.mode_bw(video_bw)
	);






	// vertical sync generator
	video_sync_v video_sync_v(

		.clk(clk),

		.mode_atm_n_pent(mode_atm_n_pent),
		.mode_60hz(mode_60hz),

		.hsync_start(hsync_start),
		.line_start(line_start),
		.hint_start(hint_start),

		.vblank(vblank),
		.vsync(vsync),
		.vpix(vpix),

		.int_start(int_start)
	);


	// horizontal sync generator
	video_sync_h video_sync_h(

		.clk(clk),

		.mode_atm_n_pent(mode_atm_n_pent),
		.mode_a_text    (mode_a_text),


		.init(1'b0),

		.pre_cend(pre_cend),
		.cend    (cend    ),


		.hblank(hblank),
		.hsync(hsync),
		.hpix(hpix),

		.line_start(line_start),
		.hsync_start(hsync_start),

		.hint_start(hint_start),

		.scanin_start(scanin_start),

		.fetch_start(fetch_start),
		.fetch_end  (fetch_end  )

	);


	// address generation
	video_addrgen video_addrgen(

		.clk(clk),

		.video_addr(video_addr),
		.video_next(video_next),

		.line_start(hsync_start),
		.int_start (int_start ),
		.vpix      (vpix      ),

		.scr_page(scr_page),

		.typos(typos),

		.mode_atm_n_pent (mode_atm_n_pent ),
		.mode_zx         (mode_zx         ),
		.mode_p_16c      (mode_p_16c      ),
		.mode_p_hmclr    (mode_p_hmclr    ),
		.mode_a_hmclr    (mode_a_hmclr    ),
		.mode_a_16c      (mode_a_16c      ),
		.mode_a_text     (mode_a_text     ),
		.mode_a_txt_1page(mode_a_txt_1page)
	);


	// data fetch
	video_fetch video_fetch(

		.clk(clk),

		.pre_cend (pre_cend),
		.cend     (cend    ),

		.vpix(vpix),

		.fetch_start(fetch_start),
		.fetch_end  (fetch_end  ),

		.fetch_sync (fetch_sync ),

		.video_data  (video_data  ),
		.video_strobe(video_strobe),
		.video_go    (video_go    ),

		.pic_bits(pic_bits)
	);


	// render fetched data to pixels
	video_render video_render(

		.clk(clk),

		.pic_bits(pic_bits),

		.fetch_sync(fetch_sync),

		.cbeg     (cbeg     ),
		.post_cbeg(post_cbeg),
		.pre_cend (pre_cend ),
		.cend     (cend     ),

		.int_start(int_start),

		.mode_atm_n_pent(mode_atm_n_pent),
		.mode_zx        (mode_zx        ),
		.mode_p_16c     (mode_p_16c     ),
		.mode_p_hmclr   (mode_p_hmclr   ),
		.mode_a_hmclr   (mode_a_hmclr   ),
		.mode_a_16c     (mode_a_16c     ),
		.mode_a_text    (mode_a_text    ),
		.mode_pixf_14   (mode_pixf_14   ),

		.typos(typos),

		.pixels(pixels),


		.fnt_a (fnt_a ),
		.fnt_d (fnt_d ),
		.fnt_wr(fnt_wr),

		.fontrom_readback(fontrom_readback)
	);


	// combine border and pixels, apply palette
	video_palframe video_palframe(

		.clk(clk),

		.hblank(hblank),
		.vblank(vblank),

		.hpix(hpix),
		.vpix(vpix),

		.pixels(pixels),
		.border(zxborder),

		.atm_palwr  (atm_palwr  ),
		.atm_paldata(atm_paldata),

		.color(color),

		.palcolor(palcolor) // palette readback
	);


	// VGA hsync doubling
	video_vga_sync_h video_vga_sync_h(

		.clk(clk),

		.hsync_start(hsync_start),

		.scanout_start(scanout_start),

		.vga_hsync(vga_hsync)
	);


	// VGA scandoubling
	video_vga_double video_vga_double(

		.clk(clk),

		.hsync_start  (hsync_start  ),
		.scanout_start(scanout_start),
		.scanin_start (scanin_start ),

		.pix_in(color),

		.pix_out(vga_color)
	);


	// final MUXing of VGA and TV signals
	video_outmux video_outmux(

		.clk(clk),

		.vga_on(vga_on),


		.tvcolor(color),
		.vgacolor(vga_color),

		.vga_hsync(vga_hsync),
		.hsync    (hsync    ),
		.vsync    (vsync    ),

		.vred(vred),
		.vgrn(vgrn),
		.vblu(vblu),

		.vhsync(vhsync),
		.vvsync(vvsync),
		.vcsync(vcsync)
	);






endmodule

