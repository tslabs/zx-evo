// This module is a video top-level


module video_top (

// clocks
	input wire clk, zclk,
	input wire f0, f1,
	input wire h0, h1,
	input wire c0, c1, c2, c3,
	// input wire t0,	// debug!!!

// video DAC
	output wire	[1:0] vred,
	output wire	[1:0] vgrn,
	output wire	[1:0] vblu,

// video syncs
	output wire	hsync,
	output wire	vsync,
	output wire	csync,

// video config
	input wire [7:0] border,
	input wire [7:0] vpage,     // strobed at line_start
	input wire [7:0] vconf,     // strobed at line_start
	input wire [8:0] x_offs,     // strobed at line_start
	input wire [8:0] y_offs,     // strobed at line_start
	input wire [7:0] hint_beg,
	input wire [8:0] vint_beg,
	input wire [7:0] tsconf,
	input wire [3:0] palsel,
	input wire [4:0] tgpage,
	
// Z80 controls
	input wire [15:0] a,
	input wire [14:0] cram_data_in,
	input wire [15:0] sfys_data_in,
	input wire 		  cram_we,
	input wire 		  sfys_we,
	input wire        y_offs_wr,
	input wire        p7ffd_wr,
	
// ZX controls
	output wire int_start,

// DRAM interface
	output wire [20:0] video_addr,
	output wire [ 4:0] video_bw,
	output wire        video_go, 
	input  wire [15:0] dram_rddata,
	input  wire        video_next,
	input  wire        video_strobe,
	output wire [20:0] ts_addr,
	output wire        ts_req,
	input  wire        ts_next,
	input  wire        ts_strobe,
	
// video controls
	input wire vga_on
	
);


    wire [9:0] x_offs_mode;
	wire [7:0] vpage_d;
	wire [8:0] hpix_beg;
	wire [8:0] hpix_end;
	wire [8:0] vpix_beg;
	wire [8:0] vpix_end;
	wire [5:0] x_tiles;
	wire [9:0] vga_cnt_in;
	wire [9:0] vga_cnt_out;
	wire [8:0] lcount;
    wire [4:0] go_offs;
	wire [1:0] render_mode;
	wire tv_hires;
	wire vga_hires;
	wire nogfx;
	wire tv_blank;
	wire vga_blank;
	wire vga_line;
	wire frame_start;
	wire line_start;
	wire pix_start;
	wire flash;
	wire tspix_start;
	wire tv_pix_start;
    wire vga_pix_start;
	wire hpix;
	wire vpix;
	wire hvpix;
    wire [3:0] palsel_d;
    wire [7:0] cnt_col;
    wire [8:0] cnt_row;
	wire cptr;
    wire [3:0] scnt;
	wire [31:0] fetch_data;
	wire [31:0] fetch_temp;
	wire [3:0] fetch_sel;
	wire [1:0] fetch_bsl;
	wire fetch_stb;
	wire [7:0] tsdata;
	wire [8:0] tsbuf_wr_addr;
	wire [7:0] tsbuf_wr_data;
	wire tsbuf_we;
	wire pix_stb;
	wire [7:0] vplex;
	wire [7:0] vgaplex;
	

	video_mode video_mode (
		.clk		  	(clk),
		.f1			    (f1),
		.c3			    (c3),
		.vconf		    (vconf),
		.vpage	    	(vpage),
		.vpage_d    	(vpage_d),
		.palsel	    	(palsel),
		.palsel_d    	(palsel_d),
		.fetch_sel		(fetch_sel),
		.fetch_bsl		(fetch_bsl),
		.fetch_cnt	    (scnt),
		.fetch_stb	    (fetch_stb),
		.txt_char	    (fetch_temp[15:0]),
		.x_offs			(x_offs),
		.x_offs_mode	(x_offs_mode),
        .line_start     (line_start),
		.p7ffd_wr	    (p7ffd_wr),
		.hpix_beg	    (hpix_beg),
		.hpix_end	    (hpix_end),
		.vpix_beg	    (vpix_beg),
		.vpix_end	    (vpix_end),
		.x_tiles	    (x_tiles),
        .go_offs        (go_offs),
        .cnt_col        (cnt_col),
        .cnt_row        (cnt_row),
        .cptr	        (cptr),
		.pix_start	    (pix_start),
		.tv_hires		(tv_hires),
		.vga_hires	    (vga_hires),
		.nogfx		    (nogfx),
		.pix_stb	    (pix_stb),
		.render_mode	(render_mode),
		.video_addr	    (video_addr),
		.video_bw		(video_bw)
);
	
	
	video_sync video_sync (
		.clk			(clk),
		.f1				(f1),
		.c3				(c3),
		.hpix_beg		(hpix_beg),
		.hpix_end		(hpix_end),
		.vpix_beg		(vpix_beg),
		.vpix_end		(vpix_end),
        .go_offs        (go_offs),
        .x_offs         (x_offs_mode[1:0]),
        .y_offs_wr      (y_offs_wr),
		.hint_beg		(hint_beg),
		.vint_beg		(vint_beg),
		.hsync			(hsync),
		.vsync			(vsync),
		.csync			(csync),
		.tv_blank		(tv_blank),
		.vga_blank		(vga_blank),
		.vga_cnt_in		(vga_cnt_in),
		.vga_cnt_out	(vga_cnt_out),
		.lcount			(lcount),
        .cnt_col        (cnt_col),
        .cnt_row        (cnt_row),
        .cptr	        (cptr),
		.scnt			(scnt),
		.flash			(flash),
		.pix_stb	    (pix_stb),
		.pix_start		(pix_start),
		.tspix_start	(tspix_start),
		.cstart			(x_offs_mode[9:2]),
		.rstart			(y_offs),
		.vga_line		(vga_line),
		.frame_start	(frame_start),
		.line_start		(line_start),
		.int_start		(int_start),
		.hpix			(hpix),
		.vpix			(vpix),
		.hvpix			(hvpix),
		.nogfx			(nogfx),
		.video_go		(video_go),
		.video_next		(video_next)
);


	video_fetch video_fetch (
		.clk			(clk),
		.f_sel			(fetch_sel),
		.b_sel			(fetch_bsl),
		.fetch_stb		(fetch_stb),
		.fetch_data		(fetch_data),
		.fetch_temp		(fetch_temp),
		.video_strobe	(video_strobe),
		.video_data		(dram_rddata)
);

	
	video_ts video_ts (
		.clk		    (clk),
		.zclk		    (zclk),
		.c3			    (c3),
		.line_start		(line_start),
		.num_tiles		(x_tiles),
		.lcount			(lcount),
		.tsconf			(tsconf),
		.tgpage			(tgpage),
		.vpage			(vpage_d),
		.sfys_addr_in	(a[8:1]),
		.sfys_data_in	(sfys_data_in),
		.sfys_we		(sfys_we),
		.tsbuf_wr_addr	(tsbuf_wr_addr),
		.tsbuf_wr_data	(tsbuf_wr_data),
		.tsbuf_we		(tsbuf_we),
		.ts_req			(ts_req),
		.ts_addr		(ts_addr),
		.ts_data		(dram_rddata),
		.ts_next		(ts_next),
		.ts_strobe		(ts_strobe)
);
	

	video_ts_render video_ts_render (
		// .clk		    (clk),
		.clk		    (0),
		.c0				(c0),
		.c2				(c2),
		.tspix_start	(tspix_start),
		.line_start		(line_start),
		.frame_start	(frame_start),
		.lsel			(lcount[0]),
		.tsbuf_wr_addr	(tsbuf_wr_addr),
		.tsbuf_wr_data	(tsbuf_wr_data),
		.tsbuf_we		(tsbuf_we),
		// .tsbuf_we		(0),
		.tsdata		    (tsdata)
);
	
	
	video_render video_render (
		.clk		    (clk),
		.c1			    (c1),
		.hvpix 	        (hvpix),
		.nogfx			(nogfx),
		.flash			(flash),
		.hires			(tv_hires),
		.psel			(scnt),
		.palsel			(palsel_d),
		.render_mode	(render_mode),
		.data	 	    (fetch_data),
		.border_in 	    (border),
		// .aaa 	    (vga_cnt_in),
		.tsdata_in 	    (tsdata),
		.vplex_out 	    (vplex)
);

	
	video_out video_out (
		.clk			(clk),
		.zclk			(zclk),
		.f0				(f0),
		.c3				(c3),
		.vga_on			(vga_on),
		.tv_blank 		(tv_blank),
		.vga_blank		(vga_blank),
		.vga_line		(vga_line),
		.palsel			(palsel_d),
	    .plex_sel_in	({h1, f1}),
		.tv_hires		(tv_hires),
		.vga_hires		(vga_hires),
		// .t0			(t0),	//debug
		.cram_addr_in	(a[8:1]),
		.cram_data_in	(cram_data_in),
		.cram_we		(cram_we),
	    .vplex_in		(vplex),
	    .vgaplex		(vgaplex),
	    // .vgaplex		(8'h0f),
		.vred			(vred),
	    .vgrn			(vgrn),
	    .vblu			(vblu)
);
	
	
	video_vmem video_vmem(
		.clock		(clk),
		.wraddress	(vga_cnt_in),
		// .data		(vplex | (t0 << 1)),	//debug!!!
		.data		(vplex),
		// .data		(8'h0f),
		.wren		(c3),
	    .rdaddress	(vga_cnt_out),
	    // .rden		(f0),
	    .q			(vgaplex)
);
	
endmodule
