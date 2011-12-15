// This is the heart of TS-graphics - Tile Sprite Processing Unit

module video_ts (

// clocks
	input wire clk,
	input wire zclk,
	input wire c7,

// video config
    input wire [4:0] tgpage,
    input wire [7:0] vpage,
	input wire [7:0] tsconf,
	input wire [5:0] num_tiles,
	
// video controls
	input wire line_start,
	input wire [8:0] lcount,
	
// Z80 controls
	input  wire [15:0] sfys_data_in,
	input  wire [7:0] sfys_addr_in,
	input  wire sfys_we,
	
// buffer interface
	output wire [8:0] tsbuf_wr_addr,
	output wire [8:0] tsbuf_wr_data,
	output wire [8:0] tsbuf_we,

// DRAM interface
	output wire        ts_req,
	output wire [20:0] ts_addr,
	input  wire [15:0] ts_data,
	input  wire        ts_next,
	input  wire        ts_strobe
	
);


// sprite descriptor fields
	// R0
	wire [8:0]  s_xcrd	= sfys_data[8:0];
	wire [1:0]  s_xsz	= sfys_data[10:9];
	wire        s_xflp	= sfys_data[11];
	wire [3:0]	s_pal	= sfys_data[15:12];
	// R1
	wire [8:0]  s_ycrd	= sfys_data[8:0];
	wire [1:0]  s_ysz	= sfys_data[10:9];
	wire        s_yflp	= sfys_data[11];
	wire        s_act	= sfys_data[14];
	wire        s_leap	= sfys_data[15];
	// R2
	wire [14:0] s_addr	= sfys_data[15:1];

	
// config
	wire s_en = tsconf[7];
	wire t1_en = tsconf[6];
	wire t0_en = tsconf[5];
		
	
// combs
	wire start = line_start & c7;
	wire sprites_start = (start | layer_sw) & ((layer == S0) | (layer == S1) | (layer == S2));
	wire tiles_start = (start | layer_sw) & ((layer == T0) | (layer == T1));
	wire layer_sw = (s_leap & sprite_skip) | (leap_r & sprite_end);
	wire sprite_sw = sprite_skip | sprite_end;
	wire tile_end = 0;
	wire sprite_end = 0;
	wire sprites_end = s_num == 6'd42;
	wire sprite_skip = r1_stb & (!s_act | !((lcount >= s_ycrd) & (lcount < (s_ycrd + s_ysiz))));
	wire s_reg_stb = sprites_start ? 1'b1 : ~s_reg_stb_r;
	wire r0_stb = (s_reg_r == R0) & s_reg_stb;
	wire r1_stb = (s_reg_r == R1) & s_reg_stb;
	wire r2_stb = (s_reg_r == R2) & s_reg_stb;
	wire finish = layer == END;

	wire [2:0] layer = start ? lnxt[BEG] : layer_sw ? lnxt[layer_r] : layer_r;
	wire [5:0] s_num = start ? 6'b0 : sprite_sw ? s_num_r + 6'd1 : s_num_r;
	wire [2:0] s_reg = sprites_start | sprite_sw ? R1 : s_reg_stb & (s_reg_r != R3) ? rnxt[s_reg_r] : s_reg_r;
	wire [2:0] x_sz = s_xsz + 1;

	// buffer interface
	assign tsbuf_wr_addr = xcrd;
	assign tsbuf_wr_data = 0;
	assign tsbuf_we = 0;

	// DRAM address for TS gfx reading
	wire [20:0] addr = tns ? a_t : a_s;
	wire [20:0] a_t = tiles_start ? t_addr : addr_next;
	wire [20:0] a_s = r2_stb ? s_addr_c : addr_next;
	wire [20:0] t_addr = 0;
	wire [20:0] s_addr_c = {s_addr, 6'b0} + 0;
	wire [20:0] addr_next = ts_next ? addr_r + 1 : addr_r;

	// number of tiles to process (sprite: 2-8, or tile: up to 46)
	wire [5:0] tiles = tns ? n_t : n_s;
	wire [5:0] n_t = tiles_start ? num_tiles : tiles_next;
	wire [5:0] n_s = r0_stb ? x_sz << 1 : tiles_next;
	wire [5:0] tiles_next = tile_end ? tiles_r - 1 : tiles_r;

	// X coordinate of tile being draw
	wire [8:0] xcrd = tns ? x_t : x_s;
	wire [8:0] x_t = tiles_start ? 0 : x_next;
	wire [8:0] x_s = r0_stb ? s_xflp ? (x_sz << 4) - s_xcrd - 1 : s_xcrd : x_next;
	wire [8:0] x_next = xflp_r & !tns ? xcrd_r - 1 : xcrd_r + 1;
	
	wire [7:0] sfys_addr = {tns, tns ? ys_addr : sf_addr};
	wire [6:0] sf_addr = s_num * 3 + s_reg;		// 16 bit memory addressing
	wire [6:0] ys_addr = 0;
	wire [6:0] s_ysiz = (s_ysz + 1) * 16;		// Y size in lines
	wire tns = layer[0];						// tiles NOT sprites

	assign ts_req = 0;
	assign ts_addr = {addr};

	
	
// layers
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


// sprite registers
	localparam R0 = 2'd0;
	localparam R1 = 2'd1;
	localparam R2 = 2'd2;
	localparam R3 = 2'd3;

	wire [1:0] rnxt[0:3];
	assign rnxt[R0] = R3;
	assign rnxt[R1] = R2;
	assign rnxt[R2] = R0;
	assign rnxt[R3] = R3;
	
	
// rock around a clock
	reg s_reg_stb_r;
	reg [2:0] layer_r = END;
	reg [5:0] s_num_r = 6'd0;
	reg [2:0] s_reg_r;
	reg [20:0] addr_r;
	reg [8:0] xcrd_r;
	reg [5:0] tiles_r;
	
	always @(posedge clk)
	begin
		s_reg_stb_r <= s_reg_stb;
		layer_r <= layer;
		s_num_r <= s_num;
		s_reg_r <= s_reg;
		addr_r <= addr;
		xcrd_r <= xcrd;
		tiles_r <= tiles;
	end


// sprite regs
	reg [3:0]  pal_r ;
	reg        xflp_r;
	reg        yflp_r;
	reg        leap_r;
	
	always @(posedge clk)
	begin
		if (r0_stb) pal_r <= s_pal;
		if (r0_stb) xflp_r <= s_xflp;
		if (r1_stb) yflp_r <= s_yflp;
		if (r1_stb) leap_r <= s_leap;
	end
	
	
// RAM modules

	wire [15:0] sfys_data;
	wire [15:0] tmbuf_data;
	
	
video_sf_ys video_sf_ys (
	.wrclock	(~zclk),
	// .wrclock	(clk),	// this should be zclk
	.wraddress	(sfys_addr_in),
	.data		(sfys_data_in),
	.wren		(sfys_we),
	
	// .rdclock	(clk),
	.rdclock	(0),
	// .rdaddress	(sfys_addr),
	.rdaddress	(0),
	// .rden		(1),
	.q			(sfys_data)
);

	
video_tmbuf video_tmbuf (
	.clock		(clk),
	.data		(0),
	.rdaddress	(0),
	.wraddress	(0),
	.wren		(0),
	.q			(tmbuf_data)
);
	
	
endmodule


