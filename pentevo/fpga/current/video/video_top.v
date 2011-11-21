// This module is a video top-level


module video_top (

// clocks
	input wire clk,
	input wire f0,
	input wire q2,
	input wire q0,
	input wire c0,
	input wire c2,
	input wire c4,
	input wire c6,
	input wire c7,

// video DAC
	output wire	[1:0] vred,
	output wire	[1:0] vgrn,
	output wire	[1:0] vblu,

// video syncs
	output wire	hsync,
	output wire	vsync,
	output wire	csync,

// video config
	input wire [3:0] border,
	input wire [7:0] vpage,
	input wire [7:0] vconfig,
	input wire [8:0] x_offs,
	input wire [8:0] y_offs,
	
// Z80 controls
	input wire [15:0] a,
	input wire [14:0] pal_data_in,
	input wire 		  pal_we,
	
// ZX controls
	output wire int_start,

// DRAM interface
	input  wire        video_strobe,
	input  wire        video_next,
	output wire [20:0] video_addr,
	input  wire [15:0] video_data,
	output wire [ 3:0] video_bw,
	output wire        video_go, 
	
// video controls
	input wire vga_on
	
);


	video_mode video_mode (
		.vconfig	    (vconfig	     ),
		.vpage	    	(vpage	     	 ),
		.fetch_sel		(fetch_sel		 ),
		.fetch_bsl		(fetch_bsl		 ),
		.txt_char	    (fetch_data[15:0]),
		.x_offs			(x_offs			 ),
		.x_offs_mode	(x_offs_mode	 ),
		.hpix_beg	    (hpix_beg	     ),
		.hpix_end	    (hpix_end	     ),
		.vpix_beg	    (vpix_beg	     ),
		.vpix_end	    (vpix_end	     ),
        .go_offs        (go_offs         ),
        .cnt_col        (cnt_col         ),
        .cnt_row        (cnt_row         ),
        .cptr	        (cptr	         ),
		.hires		    (hires		     ),
		.render_mode	(render_mode     ),
		.video_addr	    (video_addr	     ),
		.video_bw		(video_bw		 )
	);
	
    wire [9:0] x_offs_mode;
	wire [8:0] hpix_beg;
	wire [8:0] hpix_end;
	wire [8:0] vpix_beg;
	wire [8:0] vpix_end;
    wire [4:0] go_offs;
	wire [1:0] render_mode;
	wire hires;

	
	video_sync video_sync (
		.clk			(clk			),
		.c0				(c0				),
		.c6				(c6				),
		.hpix_beg		(hpix_beg		),
		.hpix_end		(hpix_end		),
		.vpix_beg		(vpix_beg		),
		.vpix_end		(vpix_end		),
        .go_offs        (go_offs        ),
        .x_offs         (x_offs_mode[1:0]),
		.hsync			(hsync			),
		.vsync			(vsync			),
		.csync			(csync			),
		.tv_pix_start	(tv_pix_start	),
		.vga_pix_start	(vga_pix_start	),
		.hb				(tv_hblank		),
		.vb				(tv_vblank		),
		.vga_line		(vga_line		),
		.frame_start	(frame_start	),
		.line_start		(line_start		),
		.pix_start		(pix_start		),
		.int_start		(int_start		),
		.hpix			(hpix			),
		.vpix			(vpix			),
		.hvpix			(hvpix			),
		.video_go		(video_go		)
	);

	wire tv_pix_start;
    wire vga_pix_start;
	wire tv_hblank;
	wire tv_vblank;
	wire vga_hblank;
	wire vga_line;
	wire frame_start;
	wire line_start;
	wire pix_start;
	wire hpix;
	wire vpix;
	wire hvpix;
	
	
	video_cntr video_cntr (
		.clk			(clk			),
		.c4				(c4				),
		.line_start		(line_start		),
		.frame_start	(frame_start	),
		.cstart			(x_offs_mode[9:2]),
		.rstart			(y_offs			),
		.vpix			(vpix			),
        .cnt_col        (cnt_col        ),
        .cnt_row        (cnt_row        ),
        .cptr	        (cptr	        ),
		.video_next		(video_next		)
	);

    wire [7:0] cnt_col;
    wire [8:0] cnt_row;
	wire cptr;
    

	video_fetch video_fetch (
		.clk			(clk			),
		.f_sel			(fetch_sel		),
		.b_sel			(fetch_bsl		),
		.video_strobe	(video_strobe	),
		.video_data		(video_data		),
		.dram_out		(fetch_data		)
	);

	wire [31:0] fetch_data;
	wire [3:0] fetch_sel;
	wire [1:0] fetch_bsl;

	
	video_render video_render (
		.clk		    (clk      	),
		.q2			    (q2	      	),
		.c0			    (c0	      	),
		.c4			    (c4		  	),
		.c6			    (c6		  	),
		.pix_start	    (pix_start	),
		.hvpix 	        (hvpix	  	),
		.hires		    (hires		),
		.render_mode	(render_mode),
		.dram_in 	    (fetch_data	),
		.border 	    (border		),
		.vdata_out 	    (tvdata		)
	);
		
	
	video_vga video_vga (
		.clk		(clk			),
		.c0			(c0				),
		.c4			(c4				),
		.q0			(q0				),
		.start_in	(tv_pix_start	),
		.start_out	(vga_pix_start	),
		.line_start	(line_start		),
		.hb			(vga_hblank		),
		.hires		(hires		    ),
		.vga_in		(tvdata			),
		.vga_out	(vgadata		)
	);

	wire [7:0] tvdata;
	wire [7:0] vgadata;
	
	// assign vred = {debug, 1'b0};

	video_out video_out (
		.clk		(clk		),
		.f0			(f0			),
		.vga_on		(vga_on		),
		.vga_line	(vga_line	),
		.tv_hblank 	(tv_hblank	),
		.tv_vblank 	(tv_vblank	),
		.vga_hblank	(vga_hblank	),
		.hires		(hires		),
		.start_out	(vga_pix_start	),
	    .tvdata		(tvdata		),
	    .vgadata	(vgadata	),
		.pal_addr_in(a[8:1]		),
		.pal_data_in(pal_data_in),
		.pal_we		(pal_we		),
		.vred		(vred		),
	    .vgrn		(vgrn		),
	    .vblu		(vblu		)
	);
	
	
	
endmodule
