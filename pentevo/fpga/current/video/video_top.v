// This module is a video top-level


module video_top (

// clocks
	input wire clk,
	input wire f0,
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

// ZX ports
	input wire [7:0] border,
	input wire [7:0] scr_page,
	input wire [7:0] vconfig,
	
// ZX controls
	output wire int_start,

// DRAM interface
	input  wire        video_strobe,
	input  wire        video_next,
	output wire [20:0] video_addr,
	input  wire [15:0] video_data,
	output wire [ 1:0] video_bw,
	output wire        video_go, 
	
// controls
	input wire vga_on
	
);


// instantiations
	video_mode video_mode (
		.vconfig	(vconfig	),
		.hpix_beg	(hpix_beg	),
		.hpix_end	(hpix_end	),
		.vpix_beg	(vpix_beg	),
		.vpix_end	(vpix_end	)
	);
	
	wire [8:0] hpix_beg;
	wire [8:0] hpix_end;
	wire [8:0] vpix_beg;
	wire [8:0] vpix_end;
	
	
	video_sync video_sync (
		.clk			(clk			),
		.c0				(c0				),
		.c6				(c6				),
		.hpix_beg		(hpix_beg		),
		.hpix_end		(hpix_end		),
		.vpix_beg		(vpix_beg		),
		.vpix_end		(vpix_end		),
		.hsync			(hsync			),
		.vsync			(vsync			),
		.csync			(csync			),
		.tv_pix_start	(tv_pix_start	),
		.vga_pix_start	(vga_pix_start	),
		.blank			(blank			),
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
	wire blank;
	wire frame_start;
	wire line_start;
	wire pix_start;
	wire hpix;
	wire vpix;
	wire hvpix;
	
	
	
	video_adr video_adr (
		.clk			(clk			),
		.c4				(c4				),
		.line_start		(line_start		),
		.frame_start	(frame_start	),
		.mode_zx		(mode_zx		),
		.mode_256c		(mode_256c		),
		.vpix			(vpix			),
		.scr_page		(scr_page		),
		.addr_zx_gfx	(addr_zx_gfx	),
		.addr_zx_atr	(addr_zx_atr	),
		.addr_256c		(addr_256c  	),
		.video_next		(video_next		)
	);

	
	wire mode_zx = 1;
	wire mode_256c = 0;
	wire [21:0] addr_zx_gfx;
	wire [21:0] addr_zx_atr;
	wire [21:0] addr_256c;
	
	
	video_fetch video_fetch (
		.clk			(clk			),
		.c0				(c0				),
		.mode_zx		(mode_zx		),
		.mode_256c		(mode_256c		),
		.addr_zx_gfx	(addr_zx_gfx	),
		.addr_zx_atr	(addr_zx_atr	),
		.addr_256c		(addr_256c  	),
		.video_strobe	(video_strobe	),
		.video_addr		(video_addr		),
		.video_data		(video_data		),
		.video_bw		(video_bw		),
		.data_out		(fetch_data		)
	);

	wire [31:0] fetch_data;

	
	video_render video_render (
		.clk		(clk      	),
		.c0			(c0	      	),
		.c4			(c4		  	),
		.c6			(c6		  	),
		.pix_start	(pix_start	),
		.hvpix 	    (hvpix	  	),
		.blank 	    (blank	  	),
		.mode_zx 	(mode_zx	),
		.mode_256c	(mode_256c	),	 
		.data_in 	(fetch_data	),
		.border 	(border		),
		.data_out 	(tvdata		)
	);
		
	
	video_vga video_vga (
		.clk		(clk			),
		.c0			(c0				),
		.q0			(q0				),
		.start_in	(tv_pix_start	),
		.start_out	(vga_pix_start	),
		.line_start	(line_start		),
		.vga_in		(tvdata			),
		.vga_out	(vgadata		)
	);

	wire [7:0] tvdata;
	wire [7:0] vgadata;
	

	video_out video_out (
		.clk		(clk		),
		.vga_on		(vga_on		),
		.vga_line	(vga_line	),
		.vred		(vred		),
	    .vgrn		(vgrn		),
	    .vblu		(vblu		),
	    .tvdata		(tvdata		),
	    .vgadata	(vgadata	)
	);
	
	
	
endmodule
