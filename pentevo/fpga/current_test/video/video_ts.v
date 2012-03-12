`include "../include/tune.v"

// This is the heart of TS-graphics - Tile Sprite Processing Unit
// (top-level)


module video_ts (

// clocks
	input wire clk,

// video controls
	input wire start,
	input wire [8:0] line,

// video config
	input wire [7:0] tsconf,
    input wire [7:0] tmpage,
    input wire [7:0] t0gpage,
    input wire [7:0] t1gpage,
    input wire [7:0] sgpage,
	input wire [5:0] num_tiles,
	input wire [8:0] t0x_offs,
	input wire [8:0] t0y_offs,
	input wire [8:0] t1x_offs,
	input wire [8:0] t1y_offs,
	input wire [1:0] t0_palsel,
	input wire [1:0] t1_palsel,

// SFYS interface
	input  wire [7:0] sfys_addr_in,
	input  wire [15:0] sfys_data_in,
	input  wire sfys_we,

// TMbuf interface
	output wire [8:0] tmb_raddr,
	input wire [15:0] tmb_rdata,

// renderer interface
	output wire tsr_go,
    output wire [8:0] tsr_x,
    output wire [2:0] tsr_xs,
    output wire tsr_xf,
    output wire [7:0] tsr_page,
    output wire [8:0] tsr_line,
	output wire [6:0] tsr_addr,
    output wire [3:0] tsr_pal,
    input wire tsr_ready

);


// sprite descriptor fields
  // R0
	wire [8:0]  s_ycrd	= sfys_data_out[8:0];
	wire [2:0]  s_ysz	= sfys_data_out[11:9];
	wire        s_act	= sfys_data_out[13];
	wire        s_leap	= sfys_data_out[14];
	wire        s_yflp	= sfys_data_out[15];
  // R1
	wire [8:0]  s_xcrd	= sfys_data_out[8:0];
	wire [2:0]  s_xsz	= sfys_data_out[11:9];
	wire        s_xflp	= sfys_data_out[15];
  // R2
	wire [11:0] s_tnum	= sfys_data_out[11:0];
	wire [3:0]	s_pal	= sfys_data_out[15:12];


// tile descriptor fields
	wire [11:0] t_tnum	= tmb_rdata[11:0];
	wire [1:0]	t_pal	= tmb_rdata[13:12];
	wire        t_xflp	= tmb_rdata[14];
	wire        t_yflp	= tmb_rdata[15];


// config
	wire s_en = tsconf[7];
	wire t1_en = tsconf[6];
	wire t0_en = tsconf[5];


// TS renderer interface
    assign tsr_go = sprites ? spr_go : til_go;
    assign tsr_x = sprites ? spr_x : til_x;
    assign tsr_xs = sprites ? spr_xs : til_xs;
    assign tsr_xf = sprites ? spr_xf : til_xf;
    assign tsr_page = sprites ? spr_page : til_page;
    assign tsr_line = sprites ? spr_line : til_line;
    assign tsr_addr = sprites ? spr_addr : til_addr;
    assign tsr_pal = sprites ? spr_pal : til_pal;


// --- Sprites ---
	wire [6:0] s_reg = start ? 7'b0 : s_reg_next;
    wire [6:0] s_reg_next = sprite_wait ? s_reg_r : s_reg_inc;
    wire [6:0] s_reg_inc = s_reg_r + (sprite_skip ? 7'd2 : 7'd1);

    wire [2:0] sr_sel = (start | sprite_skip) ? 3'b001 : sr_sel_next;
    wire [2:0] sr_sel_next = sprite_wait ? sr_sel_r : {sr_sel_r[1:0], sr_sel_r[2]};

    wire [1:0] r0_stb = start ? 2'b01 : r0_stb_next;
    wire [1:0] r0_stb_next = sprite_wait ? r0_stb_r : {r0_stb_r[0], sr_sel[0]};
	wire r0_rd_stb = r0_stb_r[1] & !start;
    wire [1:0] r1_stb = start ? 2'b00 : r1_stb_next;
    wire [1:0] r1_stb_next = sprite_wait ? r1_stb_r : {r1_stb_r[0], sr_sel[1]};
	wire r1_rd_stb = r1_stb_r[1] & !start;
    wire [1:0] r2_stb = start ? 2'b00 : r2_stb_next;
    wire [1:0] r2_stb_next = sprite_wait ? r2_stb_r : {r2_stb_r[0], sr_sel[2]};
	wire r2_rd_stb = r2_stb_r[1] & !start;

    wire sprite_skip = r0_rd_stb & !s_act & !sprite_visible;
    wire sprite_wait = r2_rd_stb & (!sprites | !tsr_ready);

    wire sprite_layer_end = (sprite_skip & s_leap) | (r2_rd_stb & sprites & tsr_ready & sprite_leap_r) | sprites_end;
	wire sprites = (layer == S0) | (layer == S1) | (layer == S2);
    wire sprites_end = s_reg == 7'd126;

	reg [6:0] s_reg_r;
    reg [2:0] sr_sel_r;
    reg [1:0] r0_stb_r;
    reg [1:0] r1_stb_r;
    reg [1:0] r2_stb_r;
	always @(posedge clk)
    begin
		s_reg_r <= s_reg;
        sr_sel_r <= sr_sel;
        r0_stb_r <= r0_stb;
        r1_stb_r <= r1_stb;
        r2_stb_r <= r2_stb;
    end


// SFile regs latching
    reg [8:0] tsr_x_r;
    reg [2:0] tsr_xs_r;
    reg tsr_xf_r;
    reg sprite_leap_r;
	always @(posedge clk)
	begin
		// if (r0_rd_stb) // here calculate and latch TSR addressing from Y
		if (r0_rd_stb) // here calculate and latch TSR addressing from Y
		if (r0_rd_stb) sprite_leap_r <= s_leap;

		if (r1_rd_stb) tsr_x_r <= s_xcrd;
		if (r1_rd_stb) tsr_xs_r <= s_xsz;
		if (r1_rd_stb) tsr_xf_r <= s_xflp;
	end

    
// sprite geometry
    // wire [8:0] line_of_sprite = line - s_ycrd;
    wire sprite_visible = 1;     // debug!!!
	// wire sprite_visible = ; // here calculate if sprite is visible in the current line
	// wire [6:0] s_ysiz = (s_ysz + 1) * 8;		// Y size in lines


// sprite TSR values
    wire spr_go = r2_rd_stb & tsr_ready;
    wire [8:0] spr_x = tsr_x_r;
    wire [2:0] spr_xs = tsr_xs_r;
    wire spr_xf = tsr_xf_r;
    wire [7:0] spr_page = sgpage;
    wire [8:0] spr_line = line;     // debug!!!
    wire [6:0] spr_addr = {s_tnum[5:0], 1'b0};
    wire [3:0] spr_pal = s_pal;


// --- Tiles ---
    wire [5:0] tpos_x = (start | tile_layer_end) ? tpos_x_init : tpos_x_next;
    wire [5:0] tpos_x_init = tx_offs[8:3];
    wire [5:0] tpos_x_next = tile_wait ? tpos_x_r : tpos_x_r + 6'd1;
    assign tmb_raddr = {line[4:3], tpos_x, tiles1};
    
    wire [5:0] tx = (start | tile_layer_end) ? 6'b0 : tx_next;
    wire [5:0] tx_next = (tr_rd_stb & (tsr_ready | !t_act)) ? tx_r + 6'd1 : tx_r;
    
    wire [1:0] tr_stb = (start | tile_layer_end) ? 2'b01 : tr_stb_next;
    wire [1:0] tr_stb_next = tile_wait ? tr_stb_r : {tr_stb_r[0], 1'b1};
	wire tr_rd_stb = tr_stb_r[1] & !(start | tile_layer_end);
	wire tr_rd_stb_act = tr_rd_stb & t_act;
    wire t_act = |t_tnum;
    
    wire tile_wait = tr_rd_stb_act & (!tiles | !tsr_ready);
    
    wire tiles0 = layer == T0;
    wire tiles1 = layer == T1;
    wire tiles = tiles0 | tiles1;
	wire tile_layer_end = tx_r == num_tiles;
    wire [8:0] tx_offs = tiles0 ? t0x_offs : t1x_offs;

	reg [5:0] tpos_x_r;
	reg [5:0] tx_r;
	reg [1:0] tr_stb_r;
	always @(posedge clk)
	begin
		tpos_x_r <= tpos_x;
		tx_r <= tx;
		tr_stb_r <= tr_stb;
	end


// tile TSR values
    wire til_go = tr_rd_stb_act & tsr_ready;
    wire [8:0] til_x = {tx, 3'd0} - tx_offs[2:0];
    wire [2:0] til_xs = 3'b0;
    wire til_xf = t_xflp;
    wire [7:0] til_page = tiles0 ? t0gpage : t1gpage;
    wire [8:0] til_line = line;     // debug!!!
    wire [6:0] til_addr = {t_tnum[5:0], 1'b0};
    wire [3:0] til_pal = {tiles0 ? t0_palsel : t1_palsel, t_pal};


// layers
	wire [2:0] layer = start ? lnxt[BEG] : layer_next;
	wire [2:0] layer_next = layer_switch ? lnxt[layer_r] : layer_r;
    wire layer_switch = sprite_layer_end | tile_layer_end;

	reg [2:0] layer_r;
	always @(posedge clk)
		layer_r <= layer;


	localparam S0 = 3'd0;
	localparam T0 = 3'd1;
	localparam S1 = 3'd2;
	localparam T1 = 3'd3;
	localparam S2 = 3'd4;
	localparam END = 3'd5;
	localparam END1 = 3'd6;
	localparam BEG = 3'd7;

	wire [2:0] lnxt[0:7];
	assign lnxt[BEG] = s_en ? S0 : t0_en ? T0 : t1_en ? T1 : END;
	assign lnxt[S0] = t0_en ? T0 : s_en ? S1 : t1_en ? T1 : END;
	assign lnxt[T0] = s_en ? S1 : t1_en ? T1 : END;
	assign lnxt[S1] = t1_en ? T1 : s_en ? S2 : END;
	assign lnxt[T1] = s_en ? S2 : END;
	assign lnxt[S2] = END;
	assign lnxt[END] = END;
	assign lnxt[END1] = END;


// SFile / YScrolls
	wire [7:0] sfys_addr_out = {1'b0, s_reg};
	wire [15:0] sfys_data_out;

video_sf_ys video_sf_ys (
	.clock	    (clk),
	.wraddress	(sfys_addr_in),
	.data		(sfys_data_in),
	.wren		(sfys_we),
	.rdaddress	(sfys_addr_out),
	.q			(sfys_data_out)
);


endmodule
