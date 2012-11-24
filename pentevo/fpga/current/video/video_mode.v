// This module decodes video modes

`include "../include/tune.v"


module video_mode (

// clocks
	input wire clk, f1, c3, c2,

// video config
	input wire [7:0] vconf,
	output reg [7:0] vconf_d,
	input wire [7:0] vpage,
	output reg [7:0] vpage_d,
	input reg  [7:0] tmpage,
	input wire [7:0] palsel,
	output reg [7:0] palsel_d,
	input wire [5:0] t0y_offs,
	input wire [5:0] t1y_offs,

// video parameters & mode controls
	input  wire [8:0] gx_offs,
	output reg  [8:0] gx_offs_d,
	output wire [8:0] hpix_beg,
	output wire [8:0] hpix_end,
	output wire [8:0] vpix_beg,
	output wire [8:0] vpix_end,
	output wire [5:0] x_tiles,
	output wire [4:0] go_offs,
	output wire [3:0] fetch_sel,
	output wire	[1:0] fetch_bsl,
	input wire [3:0] fetch_cnt,
	input wire [1:0] tm_en,
	input wire [1:0] tys_en,
	input wire tm_pf,
	input wire pix_start,
	input wire line_start,
	output wire txmode,
	output reg  vga_hires,
	output wire pix_stb,
	output wire	fetch_stb,
	output wire nogfx,

// video data
	input wire [15:0] txt_char,

// video counters
    input wire [7:0] cnt_col,
    input wire [8:0] cnt_row,
    input wire [8:0] cnt_tp_row,
    input wire cptr,

// Z80 controls
    input wire zvpage_wr,

// TMBUF interface
    output wire [8:0] tmb_waddr,

// SFYS interface
	output wire [6:0] tys_data_x,
	output wire tys_data_s,
	input wire [5:0] tys_data_f,

// DRAM interface
    output wire [20:0] video_addr,
    output wire [ 4:0] video_bw,
    input wire next_video

);


    wire [1:0] vmod = vconf_d[1:0];
	wire [1:0] rres = vconf_d[7:6];
	assign nogfx = vconf_d[5] || (vmod == M_HC) || (vmod == M_XC);
	assign xtgfx = !vconf_d[5] && ((vmod == M_HC) || (vmod == M_XC));
	assign txmode = vmod == M_TX;


// re-latching regs at line start
    always @(posedge clk) if (line_start & c3)
    begin
        vconf_d <= vconf;
        gx_offs_d <= gx_offs;
        palsel_d <= palsel;

        vga_hires <= txmode;	// delaying hires for 1 line
    end

    reg zvpage_wr_r;
    always @(posedge clk)
        zvpage_wr_r <= zvpage_wr;

    always @(posedge clk) if ((line_start & c3) | zvpage_wr_r)
        vpage_d <= vpage;


// clocking strobe for pixels (TV)
	assign pix_stb = txmode ? f1 : c3;


// Modes
    localparam M_ZX = 2'h0;		// ZX
    localparam M_HC = 2'h1;		// 16c
    localparam M_XC = 2'h2;		// 256c
    localparam M_TX = 2'h3;		// Text


// Render modes (affects 'video_render.v')
    localparam R_ZX = 2'h0;
    localparam R_HC = 2'h1;
    localparam R_XC = 2'h2;
    localparam R_TX = 2'h3;


// fetch strobes
	assign fetch_stb = (pix_start || &fetch_cnt[3:0]) && c3;


// fetch window
    assign go_offs = txmode ? 5'd10 : 5'd18;


// fetch selectors
// Attention: counter is already incremented at the time of video data fetching!

	assign fetch_sel = txmode ? f_txt_sel[cnt_col[1:0]] : {~cptr, ~cptr, cptr, cptr};
	assign fetch_bsl = txmode ? f_txt_bsl[cnt_col[1:0]] : 2'b10;

	wire [3:0] f_txt_sel[0:3];
	assign f_txt_sel[1] = 4'b0011;			// char
	assign f_txt_sel[2] = 4'b1100;			// attr
	assign f_txt_sel[3] = 4'b0001;			// gfx0
	assign f_txt_sel[0] = 4'b0010;			// gfx1

	wire [1:0] f_txt_bsl[0:3];
	assign f_txt_bsl[1] = 2'b10;			// char
	assign f_txt_bsl[2] = 2'b10;			// attr
	assign f_txt_bsl[3] = {2{cnt_row[0]}};	// gfx0
	assign f_txt_bsl[0] = {2{cnt_row[0]}};	// gfx1


// raster resolution
	wire [8:0] hp_beg[0:3];
	wire [8:0] hp_end[0:3];
	wire [8:0] vp_beg[0:3];
	wire [8:0] vp_end[0:3];
	wire [5:0] x_tile[0:3];

	assign hp_beg[0] = 9'd140;	// 256 (88-52-256-52)
	assign hp_beg[1] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[2] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[3] = 9'd88;	// 360 (88-0-360-0)

	assign hp_end[0] = 9'd396;	// 256
	assign hp_end[1] = 9'd428;	// 320
	assign hp_end[2] = 9'd428;	// 320
	assign hp_end[3] = 9'd448;	// 360

	// decremented by 1
	assign vp_beg[0] = 9'd079;	// 192 (32-48-192-32)
	assign vp_beg[1] = 9'd075;	// 200 (32-44-200-44)
	assign vp_beg[2] = 9'd055;	// 240 (32-24-240-24)
	assign vp_beg[3] = 9'd031;	// 288 (32-0-288-0)

	// decremented by 1
	assign vp_end[0] = 9'd271;	// 192
	assign vp_end[1] = 9'd275;	// 200
	assign vp_end[2] = 9'd295;	// 240
	assign vp_end[3] = 9'd319;	// 288

	assign x_tile[0] = 6'd33;	// 256
	assign x_tile[1] = 6'd41;	// 320
	assign x_tile[2] = 6'd41;	// 320
	assign x_tile[3] = 6'd46;	// 360

	assign hpix_beg = hp_beg[rres];
	assign hpix_end = hp_end[rres];
	assign vpix_beg = vp_beg[rres];
	assign vpix_end = vp_end[rres];
	assign x_tiles = x_tile[rres];

// DRAM bandwidth usage
    localparam BW2 = 2'b00;
    localparam BW4 = 2'b01;
    localparam BW8 = 2'b11;

    localparam BU1 = 3'b001;
    localparam BU2 = 3'b010;
    localparam BU4 = 3'b100;

	// [4:3] - total cycles: 11 = 8 / 01 = 4 / 00 = 2
	// [2:0] - need cycles
    assign video_bw = tm_pf ? bw_tm : (txmode ? bw_tx : bw_zx);
	
    wire [4:0] bw_zx = {BW8, BU1};	// '1 of 8' (ZX)
    wire [4:0] bw_tx = {BW8, BU4};	// '4 of 8' (text)
// wire [4:0] bw_tm = {BW4, &tm_en ? BU2 : BU1};      // '1/2 of 4' (1 or 2 tile-planes used)
    wire [4:0] bw_tm = {BW8, &tm_en ? BU4 : BU2};      // '2/4 of 8' (1 or 2 tile-planes used)		- dirty fix!!! probably tiles will blink at strong DRAM usage - problem with fetcher signals


// addresses
	// Tilemap prefetch
    wire [20:0] tm_addr = {tmpage, tpos_y, tpn, tpos_x};        // 128 bytes for plane0 + 128 bytes for plane1
    assign tmb_waddr = {cnt_tp_row[4:0], cnt_tp_col, tpn};      // 1 word for plane0 + 1 word for plane1
    wire [5:0] tpos_y = cnt_tp_row[8:3] + (tys_en[tpn] ? tys_data_f : (tpn ? t1y_offs : t0y_offs));
    wire [5:0] tpos_x = {cnt_tp_row[2:0], cnt_tp_col};
    wire [2:0] cnt_tp_col = &tm_en ? cnt_col[3:1] : cnt_col[2:0];
    wire tpn = &tm_en ? cnt_col[0] : tm_en[1];
	assign tys_data_s = c2 && next_video && tm_pf && tys_en[tpn];
	assign tys_data_x = {tpn, tpos_x};


// videomode addresses
    assign video_addr = tm_pf ? tm_addr : (txmode ? addr_text : addr_zx);

	// ZX
	wire [20:0] addr_zx = {vpage_d, 1'b0, ~cnt_col[0] ? addr_zx_gfx : addr_zx_atr};
	wire [11:0] addr_zx_gfx = {cnt_row[7:6], cnt_row[2:0], cnt_row[5:3], cnt_col[4:1]};
	wire [11:0] addr_zx_atr = {3'b110, cnt_row[7:3], cnt_col[4:1]};

	// 16c
	// wire [20:0] addr_16c = {vpage_d[7:3], cnt_row, cnt_col[6:0]};

	// 256c
	// wire [20:0] addr_256c = {vpage_d[7:4], cnt_row, cnt_col[7:0]};

	// Textmode
    wire [20:0] addr_text = {vpage_d[7:1], addr_tx[cnt_col[1:0]]};
	wire [13:0] addr_tx[0:3];
    assign addr_tx[0] = {vpage_d[0], cnt_row[8:3], 1'b0, cnt_col[7:2]};			// char codes, data[15:0]
    assign addr_tx[1] = {vpage_d[0], cnt_row[8:3], 1'b1, cnt_col[7:2]};			// char attributes, data[31:16]
    assign addr_tx[2] = {~vpage_d[0], 3'b000, (txt_char[7:0]), cnt_row[2:1]};	// char0 graphics, data[7:0]
    assign addr_tx[3] = {~vpage_d[0], 3'b000, (txt_char[15:8]), cnt_row[2:1]};	// char1 graphics, data[15:8]


endmodule
