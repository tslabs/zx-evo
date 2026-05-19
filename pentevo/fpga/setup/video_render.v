// Minimal ACE fork renderer: 256c + text only, no TS overlay.

`include "tune.v"

module video_render
(
  input  wire        clk,
  input  wire        c1,

  input  wire        hvpix,
  input  wire        hvtspix,
  input  wire        nogfx,
  input  wire        notsu,
  input  wire        gfxovr,
  input  wire        flash,
  input  wire        hires,
  input  wire [3:0]  psel,
  input  wire [3:0]  palsel,

  input  wire [1:0]  render_mode,

  input  wire [31:0] data,
  input  wire [ 7:0] border_in,
  input  wire [ 7:0] tsdata_in,
  output wire [ 7:0] vplex_out
);

  localparam R_XC = 2'h2;
  localparam R_TX = 2'h3;

  reg [3:0] temp;

  wire [15:0] tx_gfx = data[15:0];
  wire [15:0] tx_atr = data[31:16];
  wire tx_dot = tx_gfx[{psel[3], ~psel[2:0]}];
  wire [7:0] tx_attr = ~psel[3] ? tx_atr[7:0] : tx_atr[15:8];
  wire [7:0] tx_pix = {palsel, tx_dot ? tx_attr[3:0] : tx_attr[7:4]};

  wire [7:0] xc_dot[0:1];
  assign xc_dot[0] = data[7:0];
  assign xc_dot[1] = data[15:8];
  wire [7:0] xc_pix = xc_dot[psel[0]];

  wire text_mode = render_mode == R_TX;
  wire [7:0] pix = text_mode ? tx_pix : xc_pix;
  wire pixv = text_mode ? tx_dot : |xc_dot[psel[0]];
  wire [7:0] video = hvpix ? ((pixv && !nogfx) ? pix : (nogfx ? border_in : pix)) : border_in;

  assign vplex_out = hires ? {temp, video[3:0]} : video;

  always @(posedge clk) if (c1)
    temp <= video[3:0];

endmodule
