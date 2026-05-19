// Minimal ACE fork video mode decoder: 256c + text only.

`include "tune.v"

module video_mode
(
  input  wire        clk,
  input  wire        f1,
  input  wire        c3,

  input  wire [7:0]  vpage,
  input  wire [7:0]  vconf,
  input  wire        ts_rres_ext,
  input  wire        v60hz,

  input  wire [8:0]  gx_offs,
  output wire [9:0]  x_offs_mode,
  output wire [8:0]  hpix_beg,
  output wire [8:0]  hpix_end,
  output wire [8:0]  vpix_beg,
  output wire [8:0]  vpix_end,
  output wire [8:0]  hpix_beg_ts,
  output wire [8:0]  hpix_end_ts,
  output wire [8:0]  vpix_beg_ts,
  output wire [8:0]  vpix_end_ts,
  output wire [5:0]  x_tiles,
  output wire [4:0]  go_offs,
  output wire [3:0]  fetch_sel,
  output wire [1:0]  fetch_bsl,
  input  wire [3:0]  fetch_cnt,
  input  wire        pix_start,
  input  wire        line_start_s,
  output wire        tv_hires,
  output reg         vga_hires = 1'b0,
  output wire [1:0]  render_mode,
  output wire        pix_stb,
  output wire        fetch_stb,

  input  wire [15:0] txt_char,

  input  wire [7:0]  cnt_col,
  input  wire [8:0]  cnt_row,
  input  wire        cptr,

  output wire [20:0] video_addr,
  output wire [ 4:0] video_bw
);

  localparam M_XC = 2'h2;
  localparam M_TX = 2'h3;

  localparam R_XC = 2'h2;
  localparam R_TX = 2'h3;

  wire [1:0] rres = vconf[7:6];
  wire text_mode = vconf[1:0] == M_TX;
  wire [1:0] vmod = text_mode ? M_TX : M_XC;

  assign pix_stb = tv_hires ? f1 : c3;

  always @(posedge clk)
    if (line_start_s)
      vga_hires <= tv_hires;

  wire ftch_xc = fetch_cnt[0];
  wire ftch_tx = &fetch_cnt[3:0];
  assign fetch_stb = (pix_start | (text_mode ? ftch_tx : ftch_xc)) && c3;

  assign go_offs = text_mode ? 5'd10 : 5'd4;

  wire [3:0] f_txt_sel[0:3];
  wire [1:0] f_txt_bsl[0:3];

  assign f_txt_sel[1] = 4'b0011;
  assign f_txt_sel[2] = 4'b1100;
  assign f_txt_sel[3] = 4'b0001;
  assign f_txt_sel[0] = 4'b0010;

  assign f_txt_bsl[1] = 2'b10;
  assign f_txt_bsl[2] = 2'b10;
  assign f_txt_bsl[3] = {2{cnt_row[0]}};
  assign f_txt_bsl[0] = {2{cnt_row[0]}};

  assign fetch_sel = text_mode ? f_txt_sel[cnt_col[1:0]] : {~cptr, ~cptr, 2'b11};
  assign fetch_bsl = text_mode ? f_txt_bsl[cnt_col[1:0]] : 2'b10;

  assign x_offs_mode = text_mode ? {1'b0, gx_offs[8:1], gx_offs[0]} : {{gx_offs[8:1], 1'b0}, gx_offs[0]};

  localparam BW2 = 2'b00;
  localparam BW8 = 2'b11;
  localparam BU1 = 3'b001;
  localparam BU4 = 3'b100;

  assign video_bw = text_mode ? {BW8, BU4} : {BW2, BU1};
  assign tv_hires = text_mode;
  assign render_mode = text_mode ? R_TX : R_XC;

  wire [8:0] hp_beg[0:3];
  wire [8:0] hp_end[0:3];
  wire [8:0] vp_beg[0:3];
  wire [8:0] vp_end[0:3];
  wire [5:0] x_tile[0:3];

  assign hp_beg[0] = 9'd108;
  assign hp_beg[1] = 9'd108;
  assign hp_beg[2] = 9'd108;
  assign hp_beg[3] = 9'd88;

  assign hp_end[0] = 9'd428;
  assign hp_end[1] = 9'd428;
  assign hp_end[2] = 9'd428;
  assign hp_end[3] = 9'd448;

`ifdef PENT_312
  assign vp_beg[0] = v60hz ? 9'd022 : 9'd048;
  assign vp_beg[1] = v60hz ? 9'd022 : 9'd048;
  assign vp_beg[2] = v60hz ? 9'd022 : 9'd048;
  assign vp_beg[3] = v60hz ? 9'd022 : 9'd024;

  assign vp_end[0] = v60hz ? 9'd262 : 9'd288;
  assign vp_end[1] = v60hz ? 9'd262 : 9'd288;
  assign vp_end[2] = v60hz ? 9'd262 : 9'd288;
  assign vp_end[3] = v60hz ? 9'd262 : 9'd312;
`else
  assign vp_beg[0] = v60hz ? 9'd022 : 9'd056;
  assign vp_beg[1] = v60hz ? 9'd022 : 9'd056;
  assign vp_beg[2] = v60hz ? 9'd022 : 9'd056;
  assign vp_beg[3] = v60hz ? 9'd022 : 9'd032;

  assign vp_end[0] = v60hz ? 9'd262 : 9'd296;
  assign vp_end[1] = v60hz ? 9'd262 : 9'd296;
  assign vp_end[2] = v60hz ? 9'd262 : 9'd296;
  assign vp_end[3] = v60hz ? 9'd262 : 9'd320;
`endif

  assign x_tile[0] = 6'd42;
  assign x_tile[1] = 6'd42;
  assign x_tile[2] = 6'd42;
  assign x_tile[3] = 6'd47;

  assign hpix_beg = hp_beg[rres];
  assign hpix_end = hp_end[rres];
  assign vpix_beg = vp_beg[rres];
  assign vpix_end = vp_end[rres];

  assign hpix_beg_ts = hpix_beg;
  assign hpix_end_ts = hpix_end;
  assign vpix_beg_ts = vpix_beg;
  assign vpix_end_ts = vpix_end;
  assign x_tiles = x_tile[rres];

  wire [20:0] addr_256c = {vpage[7:4], cnt_row, cnt_col[7:0]};

  wire [13:0] addr_tx[0:3];
  wire [20:0] addr_text = {vpage[7:1], addr_tx[cnt_col[1:0]]};
  assign addr_tx[0] = {vpage[0], cnt_row[8:3], 1'b0, cnt_col[7:2]};
  assign addr_tx[1] = {vpage[0], cnt_row[8:3], 1'b1, cnt_col[7:2]};
  assign addr_tx[2] = {~vpage[0], 3'b000, txt_char[7:0], cnt_row[2:1]};
  assign addr_tx[3] = {~vpage[0], 3'b000, txt_char[15:8], cnt_row[2:1]};

  assign video_addr = text_mode ? addr_text : addr_256c;

endmodule
