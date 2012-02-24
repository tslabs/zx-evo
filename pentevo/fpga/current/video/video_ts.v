// This is the heart of TS-graphics - Tile Sprite Processing Unit

`include "../include/tune.v"


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

// Z80 controls / SFYS interface
	input  wire [7:0] sfys_addr_in,
	input  wire [15:0] sfys_data_in,
	input  wire sfys_we,

// TMbuf interface
	output wire [8:0] tmb_raddr,
	output wire [15:0] tmb_rdata,

// renderer interface
	output wire tsr_go,
    output wire tsr_rld,
    output wire [8:0] tsr_x,
    output wire [2:0] tsr_xs,
    output wire tsr_xf,
    output wire [7:0] tsr_page,
    output wire [8:0] tsr_line,
	output wire [6:0] tsr_addr,
    output wire [3:0] tsr_pal,
    input wire tsr_done

);


// sprite descriptor fields
	// R0
	wire [8:0]  s_xcrd	= sfys_data_out[8:0];
	wire [2:0]  s_xsz	= sfys_data_out[11:9];
	wire        s_act	= sfys_data_out[13];
	wire        s_leap	= sfys_data_out[14];
	wire        s_xflp	= sfys_data_out[15];
	// R1
	wire [8:0]  s_ycrd	= sfys_data_out[8:0];
	wire [2:0]  s_ysz	= sfys_data_out[11:9];
	wire        s_yflp	= sfys_data_out[15];
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

// sprites
	wire sprite_layer_start = (start | layer_switch) & sprites;
	wire sprites = (layer == S0) | (layer == S1) | (layer == S2);
    wire sprites_end = s_reg == 7'd126;
	wire sprite_switch = sprite_skip | sprite_end;
	wire [6:0] s_reg = start ? 7'b0 : sprite_reg_switch ? s_reg_r + 1 : s_reg_r;
    wire [2:0] sr_sel = start ? 3'b001 : sprite_reg_switch ? {sr_sel_r[1:0], sr_sel_r[2]} : sr_sel_r;
	wire sprite_end = 0;

	wire sprite_skip = r0_stb & (!s_act | !((line >= s_ycrd) & (line < (s_ycrd + s_ysiz))));

// sprite geometry
    wire [8:0] line_of_sprite = line - s_ycrd;
    wire sprite_visible = line_of_sprite < 
	wire [6:0] s_ysiz = (s_ysz + 1) * 8;		// Y size in lines

	// number of tiles to process (sprite: 2-8, or tile: up to 46)
// tiles
    wire tile_layer_start = (start | layer_switch) & tiles;
    wire tiles0 = layer == T0;
    wire tiles1 = layer == T1;
    wire tiles = tiles0 | tiles1;
	wire tile_end = 0;

// layers
	wire [2:0] layer = start ? lnxt[BEG] : layer_switch ? lnxt[layer_r] : layer_r;
    wire layer_switch = (s_leap & sprite_skip) | (leap_r & sprite_end);
	wire s_reg_stb = sprite_layer_start ? 1'b1 : ~s_reg_stb_r;
	wire finish = layer == END;

	wire [2:0] s_reg = sprite_layer_start | sprite_switch ? R0 : s_reg_stb & (s_reg_r != R3) ? rnxt[s_reg_r] : s_reg_r;

	wire [5:0] tiles = tns ? n_t : n_s;
	wire [5:0] n_t = tile_layer_start ? num_tiles : tiles_next;
	wire [5:0] n_s = r0_stb ? 0 << 1 : tiles_next;
	wire [5:0] tiles_next = tile_end ? tiles_r - 1 : tiles_r;

	// X coordinate of tile being draw
	wire [8:0] xcrd = tns ? x_t : x_s;
	wire [8:0] x_t = tile_layer_start ? 0 : x_next;
	wire [8:0] x_s = r0_stb ? s_xflp ? (0 << 4) - s_xcrd - 1 : s_xcrd : x_next;
	wire [8:0] x_next = xflp_r & !tns ? xcrd_r - 1 : xcrd_r + 1;

	wire [7:0] sfys_addr = {tns, tns ? ys_addr : sf_addr};
	wire [6:0] sf_addr = s_reg * 3 + s_reg;		// 16 bit memory addressing
	wire [6:0] ys_addr = 0;
	wire tns = layer[0];						// tiles NOT sprites

	assign ts_req = 0;
	assign ts_addr = {addr};


// layer selection
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


// TS renderer
    assign tsr_go = (sprites & r2_stb) | (tiles & t_stb);
    assign tsr_rld = (sprites & r2_stb) | (tiles & tile_layer_start);
    assign tsr_x = sprites ? tsr_x_r : (3'd7 - (tiles0 ? t0x_offs[2:0] : t1x_offs[2:0]));
    assign tsr_xs = sprites ? tsr_xs_r : 0;
    assign tsr_xf = sprites ? tsr_xf_r : t_xflp;
    assign tsr_page = sprites ? sgpage : tiles0 ? t0gpage : t1gpage;
    assign tsr_line =
    assign tsr_addr =
    assign tsr_pal =


// SFile reading strobes
	wire r0_stb = r0_sel[2];
	wire r1_stb = r1_sel[2];
	wire r2_stb = r2_sel[2];


// SFile regs latching
    reg [8:0] tsr_x_r;
    reg [2:0] tsr_xs_r;
    reg tsr_xf_r;
	always @(posedge clk)
	begin
		if (r0_stb) tsr_x_r <= s_xcrd;
		if (r0_stb) tsr_xs_r <= s_xsz;
		if (r0_stb) tsr_xf_r <= s_xflp;

		if (r1_stb) yflp_r <= s_yflp;
		if (r1_stb) leap_r <= s_leap;
	end


// rock around a clock
	reg [6:0] s_reg_r;
    reg [2:0] sr_sel_r;
    reg [2:0] r0_sel;
    reg [2:0] r1_sel;
    reg [2:0] r2_sel;
	reg [2:0] layer_r;

	reg s_reg_stb_r;
	reg [2:0] s_reg_r;
	reg [20:0] addr_r;
	reg [8:0] xcrd_r;
	reg [5:0] tiles_r;

	always @(posedge clk)
	begin
		s_reg_r <= s_reg;
		sr_sel_r <= sr_sel;
        r0_sel <= {(start ? 2'b00 : r0_sel[1:0]), sr_sel[1]};   // 1st: reg1
        r1_sel <= {(start ? 2'b00 : r1_sel[1:0]), sr_sel[0]};   // 2nd: reg0
        r2_sel <= {(start ? 2'b00 : r2_sel[1:0]), sr_sel[2]};   // 3rd: reg2
		layer_r <= layer;

		s_reg_stb_r <= s_reg_stb;
		addr_r <= addr;
		xcrd_r <= xcrd;
		tiles_r <= tiles;
	end


// SFile / YScrolls
	wire [7:0] sfys_addr_out;
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
