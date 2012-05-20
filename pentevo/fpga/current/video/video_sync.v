`include "../include/tune.v"

// This module generates all video raster signals

// clk			|—__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__—
// c0-3			|<c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3>
// hcount		|<......86......><......87......><......88......><......89......>
// hpix			|________________________________————————————————————————————————
// ts_raddr		|<......xx......><......xx......><......00......><......01......>
// ts_pix_start	|————————————————————————————————________________________________
// ts_start		|____________________________————________________________________
// ts_line_sel	|________________________________————————————————————————————————


module video_sync (

// clocks
	input wire clk, f1, c3, pix_stb,

// video parameters
	input wire [8:0] hpix_beg,
	input wire [8:0] hpix_end,
	input wire [8:0] vpix_beg,
	input wire [8:0] vpix_end,
	input wire [4:0] go_offs,
	input wire [1:0] x_offs,
	input wire [7:0] hint_beg,
	input wire [8:0] vint_beg,
	input wire [7:0] cstart,
    input wire [8:0] rstart,

// video syncs
	output reg hsync,
	output reg vsync,
	output reg csync,

// video controls
	input wire nogfx,
	input wire tiles_en,
	output wire tm_pf,
	output wire hpix,
	output wire vpix,
	output wire hvpix,
	output wire tv_blank,
	output wire vga_blank,
	output wire vga_line,
	output wire frame_start,
	output wire line_start,
	output wire pix_start,
	output wire ts_start,
	output wire frame,
	output wire flash,

// video counters
	output wire [9:0] vga_cnt_in,
	output wire [9:0] vga_cnt_out,
	output wire [8:0] ts_raddr,
	output wire [8:0] lcount,
	output reg 	[7:0] cnt_col,
	output reg 	[8:0] cnt_row,
	output reg 	[8:0] cnt_tp_row,
	output reg  	  cptr,
	output reg  [3:0] scnt,
	output reg   	  ts_line_sel,

// DRAM
	input wire video_pre_next,
	output wire video_go,

// ZX controls
	input wire y_offs_wr,
	output wire int_start

);

	localparam HSYNC_BEG 	= 9'd11;
	localparam HSYNC_END 	= 9'd43;
	localparam HBLNK_BEG 	= 9'd00;
	localparam HBLNK_END 	= 9'd88;

	localparam HSYNCV_BEG 	= 9'd5;
	localparam HSYNCV_END 	= 9'd31;
	localparam HBLNKV_END 	= 9'd42;

	localparam HPERIOD   	= 9'd448;

	localparam VSYNC_BEG 	= 9'd08;
	localparam VSYNC_END 	= 9'd11;
	localparam VBLNK_BEG 	= 9'd00;
	localparam VBLNK_END 	= 9'd32;

	localparam VPERIOD   	= 9'd320;	// fucking pentagovn!!!


// counters
	reg [8:0] hcount = 0;
	reg [8:0] vcount = 0;
	reg [8:0] cnt_out = 0;

	// horizontal TV (7 MHz)
	always @(posedge clk) if (c3)
		hcount <= line_start ? 9'b0 : hcount + 9'b1;

	wire vga_line_start = vga_pix_start && c3;

	// horizontal VGA (14MHz)
	always @(posedge clk) if (f1)
		cnt_out <= vga_line_start ? 9'b0 : cnt_out + 9'b1;


	// vertical TV (15.625 kHz)
	always @(posedge clk) if (c3) if (line_start)
		vcount <= (vcount == (VPERIOD - 1)) ? 9'b0 : vcount + 9'b1;


	// column address for DRAM
	always @(posedge clk)
    begin
		if (line_start)         // for tiles prefetch
		begin
			cnt_col <= 0;
			cptr <= 1'b0;
		end

        else
        if (line_start2)        // for graphics fetch
		begin
			cnt_col <= cstart;
			cptr <= 1'b0;
		end

        else
		if (video_pre_next)
		begin
			cnt_col <= cnt_col + 8'b1;
			cptr <= ~cptr;
		end
    end

	// row address for DRAM
	always @(posedge clk) if (c3)
		if (vis_start | (line_start & y_offs_wr_r))
			cnt_row <=  rstart;
		else
		if (line_start & vpix)
			cnt_row <=  cnt_row + 9'b1;


	// row address for tile-planes
	always @(posedge clk) if (c3)
		if (frame_start)
			cnt_tp_row <= 0;
		else
		if (line_start & tm_vpf)
			cnt_tp_row <=  cnt_tp_row + 9'b1;


	// pixel counter
	always @(posedge clk) if (pix_stb)		// f1 or c3
		scnt <= pix_start ? 4'b0 : scnt + 4'b1;

	assign vga_cnt_in = {vcount[0], hcount - HBLNK_END};
	assign vga_cnt_out = {~vcount[0], cnt_out};
	assign lcount = vcount - vpix_beg + 9'b1;


    // TS-line counter
    assign ts_raddr = hcount - hpix_beg;

	always @(posedge clk)
		if (ts_start)
			ts_line_sel <= ~ts_line_sel;


// Y offset re-latch trigger
    reg y_offs_wr_r;
    always @(posedge clk)
        if (y_offs_wr)
            y_offs_wr_r <= 1'b1;
        else
        if (line_start & c3)
            y_offs_wr_r <= 1'b0;


// FLASH generator
	reg [4:0] flash_ctr;
	assign frame = flash_ctr[0];
	assign flash = flash_ctr[4];
	always @(posedge clk)
		if (frame_start & c3)
			flash_ctr <= flash_ctr + 5'b1;


// sync strobes
	wire hs = (hcount >= HSYNC_BEG) & (hcount < HSYNC_END);
	// reg hs;
    // always @(posedge clk)
		// if (hcount == (HSYNC_BEG - 1))
			// hs <= 1'b1;
		// else if (hcount == (HSYNC_END - 2))
			// hs <= 1'b0;

	wire vs = (vcount >= VSYNC_BEG) & (vcount < VSYNC_END);
	// reg vs;
    // always @(posedge clk)
		// if (vcount == (VSYNC_BEG - 1))
			// vs <= 1'b1;
		// else if (vcount == (VSYNC_END - 2))
			// vs <= 1'b0;

	assign tv_blank = tv_hblank | tv_vblank;

	wire tv_hblank = (hcount > HBLNK_BEG) & (hcount <= HBLNK_END);
	// reg tv_hblank;
    // always @(posedge clk)
		// if (hcount == HBLNK_BEG)
			// tv_hblank <= 1'b1;
		// else if (hcount == (HBLNK_END - 1))
			// tv_hblank <= 1'b0;

	wire tv_vblank = (vcount >= VBLNK_BEG) & (vcount < VBLNK_END);
	// reg tv_vblank;
    // always @(posedge clk)
		// if (vcount == (VBLNK_BEG - 1))
			// tv_vblank <= 1'b1;
		// else if (vcount == (VBLNK_END - 2))
			// tv_vblank <= 1'b0;

	assign vga_blank = vga_hblank | vga_vblank;


	wire vga_hblank1 = (cnt_out > 9'd359);
	reg vga_hblank;
	always @(posedge clk) if (f1)		// fix me - bydlocode !!!
		vga_hblank <= vga_hblank1;
	// reg vga_hblank;
    // always @(posedge clk)
		// if (cnt_out == 9'd359)
			// vga_hblank <= 1'b1;
		// else if (vga_line_start)
			// vga_hblank <= 1'b0;

	wire hs_vga = ((hcount >= HSYNCV_BEG) & (hcount < HSYNCV_END)) |
			((hcount >= (HSYNCV_BEG + HPERIOD/2)) & (hcount < (HSYNCV_END + HPERIOD/2)));
    // reg hs_vga;
	// always @(posedge clk)
		// if ((hcount == (HSYNCV_BEG - 1)) || (hcount == (HSYNCV_BEG + HPERIOD/2 - 1)))
			// hs_vga <= 1'b1;
		// else if ((hcount == (HSYNCV_END - 2)) || (hcount == (HSYNCV_END + HPERIOD/2 - 2)))
			// hs_vga <= 1'b0;
		

	wire vga_pix_start = ((hcount == (HBLNKV_END)) | (hcount == (HBLNKV_END + HPERIOD/2)));

	assign vga_line = (hcount >= HPERIOD/2);

	assign hpix = (hcount >= hpix_beg) & (hcount < hpix_end);
	assign vpix = (vcount >= vpix_beg) & (vcount < vpix_end);
	assign hvpix = hpix & vpix;

	wire tm_hpf = (hcount >= 0) & (hcount < 32);        // 8+8 tiles per line, totally 64+64 per 8 lines
    wire tm_vpf = (vcount >= (vpix_beg - 17)) & (vcount < (vpix_end - 9));      // start prefetch 16 lines before visible area minus 1 line for ts renderer
    assign tm_pf = tm_hpf & tm_vpf & tiles_en;

	assign video_go = ((hcount >= (hpix_beg - go_offs - x_offs)) & (hcount < (hpix_end - go_offs - x_offs)) & vpix &!nogfx) || tm_pf;

	assign line_start = (hcount == (HPERIOD - 1));
	wire line_start2 = (hcount == (HSYNC_END - 1));
	assign frame_start = line_start & (vcount == (VPERIOD - 1));
	wire vis_start = line_start & (vcount == (VBLNK_END - 1));
	assign pix_start = (hcount == (hpix_beg - x_offs - 1));
	wire ts_pix_start = (hcount == (hpix_beg - 1));
	assign ts_start = c3 && ts_pix_start;
	assign int_start = (hcount == {hint_beg, 1'b0}) & (vcount == vint_beg);


	reg vga_vblank;
	always @(posedge clk) if (line_start & c3)		// fix me - bydlocode !!!
		vga_vblank <= tv_vblank;


	always @(posedge clk)
	begin
		hsync <= ~hs_vga;
		vsync <= ~vs;
		csync <= ~(vs ^ hs);
	end


endmodule
