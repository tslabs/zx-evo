`include "../include/tune.v"

// This is the Tile Sprite Processing Unit

// Tiles map address:
//  bits    desc
//  20:13   tmpage
//  12:

// Graphics address:
//  bits    desc
//  20:13   Xgpage
//  15:7    line (bits 15:13 are added) - 512 lines
//  6:0     word within line - 128 words = 512 pixels


module video_ts (

// clocks
	input wire clk,

// video controls
	input wire start,
	input wire [8:0] line,      // 	= vcount - vpix_beg + 9'b1;
	input wire pre_vpix,

// video config
	input wire [7:0] tsconf,
    input wire [7:0] t0gpage,
    input wire [7:0] t1gpage,
    input wire [7:0] sgpage,
	input wire [5:0] num_tiles,
	input wire [8:0] t0x_offs,
	input wire [8:0] t1x_offs,
	input wire [2:0] t0y_offs,
	input wire [2:0] t1y_offs,
	input wire [1:0] t0_palsel,
	input wire [1:0] t1_palsel,

// SFYS interface
	input  wire  [7:0] sfile_addr_in,
	input  wire [15:0] sfile_data_in,
	input  wire sfile_we,
	input  wire [6:0] tys_data_x,
	input  wire tys_data_s,
	output wire [5:0] tys_data_f,

// TMbuf interface
	output wire [8:0] tmb_raddr,
	input wire [15:0] tmb_rdata,

// renderer interface
	output wire tsr_go,
	output wire [5:0] tsr_addr,     // graphics address within the line
    output wire [8:0] tsr_line,     // bitmap line
    output wire [7:0] tsr_page,     // bitmap 1st page
    output wire [8:0] tsr_x,        // addr in buffer (0-359 visibles)
    output wire [2:0] tsr_xs,       // size (8-64 pix)
    output wire tsr_xf,             // X flip
    output wire [3:0] tsr_pal,		// palette
	input wire tsr_rdy,              // renderer is done and ready to receive a new task

	output wire [2:0] tst
);

// sprite descriptor fields
  // R0
	wire [8:0]  s_ycrd	= sfile_data_out[8:0];
	wire [2:0]  s_ysz	= sfile_data_out[11:9];
	wire        s_act	= sfile_data_out[13];
	wire        s_leap	= sfile_data_out[14];
	wire        s_yflp	= sfile_data_out[15];
  // R1
	wire [8:0]  s_xcrd	= sfile_data_out[8:0];
	wire [2:0]  s_xsz	= sfile_data_out[11:9];
	wire        s_xflp	= sfile_data_out[15];
  // R2
	wire [11:0] s_tnum	= sfile_data_out[11:0];
	wire [3:0]	s_pal	= sfile_data_out[15:12];


// tile descriptor fields
	wire [11:0] t_tnum	= tmb_rdata[11:0];
	wire [1:0]	t_pal	= tmb_rdata[13:12];
	wire        t_xflp	= tmb_rdata[14];
	wire        t_yflp	= tmb_rdata[15];


// config
	wire s_en = tsconf[7];
	wire t1_en = tsconf[6];
	wire t0_en = tsconf[5];
	wire t1z_en = tsconf[3];
	wire t0z_en = tsconf[2];
	wire t1ys_en = tsconf[1];
	wire t0ys_en = tsconf[0];


// TS renderer interface
    assign tsr_x = sprites ? sprites_x : tile_x;
    assign tsr_xs = sprites ? sprites_xs : tile_xs;
    assign tsr_xf = sprites ? sprites_xf : tile_xf;
    assign tsr_go = sprites ? sprite_go : tile_go;
    assign tsr_page = sprites ? sprites_page : tile_page;
    assign tsr_line = sprites ? sprites_line : tile_line;
    assign tsr_addr = sprites ? sprites_addr : tile_addr;
    assign tsr_pal = sprites ? sprites_pal : tile_pal;


// Layer selectors control

	// DEBUG !!!
	assign tst = lyr;
	reg [2:0] lyr;
	always@*
		if (layer[0])
			lyr = 4;
		else if (layer[1])
			lyr = 1;
		else if (layer[2])
			lyr = 6;
		else if (layer[3])
			lyr = 1;
		else if (layer[4])
			lyr = 2;
		else lyr = 0;

	wire [4:0] layer = start ? 5'b00001 : layer_r;

	reg [4:0] layer_r;
	always @(posedge clk)
		layer_r <= layer_skip_active ? {layer[3:0], 1'b0} : layer;

	wire sprites = layer_active[0] || layer_active[2] || layer_active[4];
	wire tiles = layer_active[1] || layer_active[3];

	wire [4:0] layer_active = layer & ~layer_skip;
	wire layer_skip_active = |(layer & layer_skip) || !pre_vpix;
	wire [4:0] layer_skip = layer_skip_end | layers_dis;
	wire [4:0] layer_skip_end = start ? 5'b00000 : (layer_skip_r | layer_end);
	wire [4:0] layer_end = {spr_end[2], tile_end[1], spr_end[1], tile_end[0], spr_end[0]};
	wire [4:0] layers_dis = {!s_en, !t1_en, !s_en, !t0_en, !s_en};

	reg [4:0] layer_skip_r;
	always @(posedge clk)
		layer_skip_r <= layer_skip;


// --- Tiles ---

    // TSR strobe for tiles
	wire tile_go = tmb_valid && tiles && t_act && tsr_rdy;

    // TSR values for tiles
    wire [7:0] tile_page = t_sel ? t0gpage : t1gpage;
    wire [8:0] tile_line = {t_tnum[11:6], (t_line[2:0] ^ {3{t_yflp}})};
    wire [5:0] tile_addr = t_tnum[5:0];
    wire tile_xf = t_xflp;
    wire [2:0] tile_xs = 3'd0;
    wire [3:0] tile_pal = {t_sel ? t0_palsel : t1_palsel, t_pal};

	// tiles internal layers control
	wire [1:0] tile_end = {2{t_layer_end}} & t_layer;
	wire t_layer_end = tx == num_tiles;
	wire tiles_done = t_layer_end && t_layer;

	wire t_layer = start ? 1'b0 : t_layer_r;
	wire t_sel = ~t_layer;

	reg t_layer_r;
	always @(posedge clk)
		t_layer_r <= t_layer_end ? 1'b1 : t_layer;

	// wire t_layer_start = start || t_layer_end;
	wire t_layer_start = start;

    // TMB control
	wire tmb_valid = !(t_layer_start || tiles_done);
    assign tmb_raddr = {t_line[4:3], tpos_x, t_layer};

	// tile X processing
	wire t_next = (!t_act && tmb_valid) || tile_go;
    wire t_act = |t_tnum || (t_sel ? t0z_en : t1z_en);

	wire [5:0] tpos_x = t_layer_start ? tx_offs[8:3] : (tpos_x_r + t_next);		// X position in tilemap
    wire [8:0] tx_offs = t_sel ? t0x_offs : t1x_offs;

	reg [5:0] tpos_x_r;
	always @(posedge clk)
        tpos_x_r <= tpos_x;

    // tile X coordinate
    wire [8:0] tile_x = {tx_r, 3'd0} - tx_offs[2:0];
	wire [5:0] tx = t_layer_start ? 6'd0 : (tx_r + t_next);		// X coordinate at the screen, higher bits (lower are offset finetune)

	reg [5:0] tx_r;
	always @(posedge clk)
        tx_r <= tx;

	// tile Y geometry
	wire [8:0] t_line = line + ty_offset;

	// tile Y scrollers
	// wire [2:0] ty_offset = tys_en ? tys_data[2:0] : ty_offs;
	wire [2:0] ty_offset = ty_offs;
	wire [2:0] ty_offs = t_sel ? t0y_offs : t1y_offs;

	// YSCRL addressing
	// wire [2:0] tys_data = yscrl_rd_r ? sfile_data_out[2:0] : tys_data_r;
	// wire yscrl_rd = tys_en && (tiles_start || t_next);
	// wire tys_en = tiles0 ? t0ys_en : t1ys_en;

	// reg [2:0] tys_data_r;
	// always @(posedge clk)
		// if (yscrl_rd_r)
			// tys_data_r <= sfile_data_out[2:0];

	// reg yscrl_rd_r;
	// always @(posedge clk)
		// yscrl_rd_r <= yscrl_rd || tys_data_s;


// --- Sprites ---

    // TSR strobe for sprites
	wire sprite_go = sf2_valid && sprites && tsr_rdy;		// kick to renderer

	// TSR values for sprites
    wire [8:0] sprites_x = sprites_x_r;
    wire [2:0] sprites_xs = sprites_xs_r;
    wire sprites_xf = sprites_xf_r;
    wire [7:0] sprites_page = sgpage;
    wire [8:0] sprites_line = s_bmline;
    wire [5:0] sprites_addr = s_tnum[5:0];
    wire [3:0] sprites_pal = s_pal;

	// sprites internal layers control
	wire [2:0] spr_end = ({3{s_layer_end}} & s_layer[2:0]) | {3{sprites_last}};
	wire s_layer_end = (spr_skip && s_leap) || (sprite_go && s_leap_r);
	wire sprites_last = sreg == 8'd254;
	wire sprites_done = s_layer_r[3];

	wire [3:0] s_layer = start ? 4'b1 : s_layer_r;

	reg [3:0] s_layer_r;
	always @(posedge clk)
		s_layer_r <= s_layer_end ? {(s_layer[2] || sprites_last), s_layer[1:0], 1'b0} : s_layer;

	// SFile registers control
	wire sreg_next = (sf0_valid && s_visible && s_act) || sf1_valid || sprite_go;
	wire spr_skip = sf0_valid && (!s_visible || !s_act);

	wire [7:0] sreg = start ? 8'd0 : ((sreg_next || spr_skip) ? (sreg_r + (sreg_next ? 8'd1 : 8'd3)) : sreg_r);

	reg [7:0] sreg_r;
	always @(posedge clk)
		sreg_r <= sreg;

	wire [2:0] sf_valid = (start || sprites_done) ? 3'd0 : sf_valid_r;
	wire sf0_valid = sf_valid[0];
	wire sf1_valid = sf_valid[1];
	wire sf2_valid = sf_valid[2];

	reg [2:0] sf_valid_r;
	always @(posedge clk)
		if (start)
			sf_valid_r <= 3'b001;
		else if (sreg_next)
			sf_valid_r <= {sf_valid_r[1:0], sf_valid_r[2]};

	// SFile regs latching
    reg [8:0] sprites_x_r;
    reg [5:0] s_bmline_offset_r;
    reg [2:0] sprites_xs_r;
    reg sprites_xf_r;
	reg s_leap_r;

	always @(posedge clk)
	begin
		if (sf0_valid)
		begin
			s_leap_r <= s_leap;
			s_bmline_offset_r <= s_bmline_offset;
		end

		if (sf1_valid)
		begin
			sprites_x_r <= s_xcrd;
			sprites_xs_r <= s_xsz;
			sprites_xf_r <= s_xflp;
		end
	end

	// sprite Y geometry
    wire [8:0] s_line = line - s_ycrd;			// visible line of sprite in current video line
    wire s_visible = (s_line <= s_ymax);		// check if sprite line is within Y size
	wire [5:0] s_ymax = {s_ysz, 3'b111};

	wire [8:0] s_bmline = {s_tnum[11:6], 3'b0} + s_bmline_offset_r;
	wire [5:0] s_bmline_offset = s_yflp ? (s_ymax - s_line[5:0]) : s_line[5:0];


// SFile
	// wire [7:0] sfile_addr_out = {(yscrl_rd || tys_data_s), tys_data_s ? tys_data_x : (yscrl_rd ? {t_layer, tpos_x} : sreg)};
	wire [7:0] sfile_addr_out = sreg;
	wire [15:0] sfile_data_out;
	assign tys_data_f = sfile_data_out[8:3];

video_sfile video_sfile (
	.clock	    (clk),
	.wraddress	(sfile_addr_in),
	.data		(sfile_data_in),
	.wren		(sfile_we),
	.rdaddress	(sfile_addr_out),
	.q			(sfile_data_out)
);


endmodule
