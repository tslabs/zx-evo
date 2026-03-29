
`include "tune.v"

module zint
(
  input  wire       clk,
  input  wire       zpos,
  input  wire       res,
  input  wire       wait_n,
  input  wire       int_start_frm,
  input  wire       int_start_lin,
  input  wire       int_start_dma,
  input  wire       int_start_wtp,
`ifdef COPPER
  input  wire       int_start_cpr,
`endif
  input  wire       vdos,           // pre_vdos
  input  wire       intack,
`ifdef PENT_312
  output wire       boost_start,
`endif

  input  wire [7:0] intmask,
  output wire [7:0] im2vect,

  output wire int_n
);

  // In VDOS INTs are focibly disabled.
  // For Frame, Line INT its generation is blocked, it will be lost.
  // For DMA INT only its output is blocked, so DMA ISR will will be processed as soon as returned from VDOS.

`ifdef COPPER
  reg [2:0] int_sel = 0;
`else 
  reg [1:0] int_sel = 0;
`endif
  reg int_frm;
  reg int_dma;
  reg int_lin;
  reg int_wtp;
`ifdef COPPER
  reg int_cpr;
`endif
  reg intack_r;
  wire intctr_fin;
  
`ifdef COPPER
  localparam INTFRM = 3'd0;
  localparam INTLIN = 3'd1;
  localparam INTDMA = 3'd2;
  localparam INTWTP = 3'd3;
  localparam INTCPR = 3'd4;
  localparam INT__1 = 3'd5;
  localparam INT__2 = 3'd6;
  localparam INT__3 = 3'd7;
`else 
  localparam INTFRM = 2'd0;
  localparam INTLIN = 2'd1;
  localparam INTDMA = 2'd2;
  localparam INTWTP = 2'd3;
`endif

`ifdef COPPER
  wire [7:0] vect [0:7];
`else
  wire [7:0] vect [0:3];
`endif
  
`ifdef COPPER
  assign vect[INTFRM] = 8'hFF;
  assign vect[INTLIN] = 8'hFD;
  assign vect[INTDMA] = 8'hFB;
  assign vect[INTWTP] = 8'hF9;
  assign vect[INTCPR] = 8'hF7;
  assign vect[INT__1] = 8'hFF;
  assign vect[INT__2] = 8'hFF;
  assign vect[INT__3] = 8'hFF;
`else
  assign vect[INTFRM] = 8'hFF;
  assign vect[INTLIN] = 8'hFD;
  assign vect[INTDMA] = 8'hFB;
  assign vect[INTWTP] = 8'hF9;
`endif

  assign im2vect = {vect[int_sel]};

`ifdef COPPER
  wire int_all = (int_frm || int_lin || int_dma || int_wtp || int_cpr) && !vdos;
`else
  wire int_all = (int_frm || int_lin || int_dma || int_wtp) && !vdos;
`endif
  assign int_n = int_all ? 1'b0 : 1'bZ;

  wire dis_int_frm = !intmask[0];
  wire dis_int_lin = !intmask[1];
  wire dis_int_dma = !intmask[2];
  wire dis_int_wtp = !intmask[3];
`ifdef COPPER
  wire dis_int_cpr = !intmask[4];
`endif

`ifdef PENT_312
  assign boost_start = intack_s || intctr_fin_s;

  wire intctr_fin_s = !intctr_fin_r && intctr_fin;
  reg intctr_fin_r;
  always @(posedge clk)
    intctr_fin_r <= intctr_fin;
`endif

  wire intack_s = intack && !intack_r;
  always @(posedge clk)
    intack_r <= intack;

// ~INT source latch
  always @(posedge clk)
    if (intack_s)
    begin
      if (int_frm)
        int_sel <= INTFRM;    // priority 0
      else if (int_lin)
        int_sel <= INTLIN;    // priority 1
      else if (int_dma)
        int_sel <= INTDMA;    // priority 2
`ifdef COPPER
      else if (int_cpr)
        int_sel <= INTCPR;    // priority 3
`endif
      else if (int_wtp)
        int_sel <= INTWTP;    // priority 4
    end

// ~INT generating
  always @(posedge clk)
    if (res || dis_int_frm)
      int_frm <= 1'b0;
    else if (int_start_frm)
      int_frm <= 1'b1;
    else if (intack_s || intctr_fin)    // priority 0
      int_frm <= 1'b0;

  always @(posedge clk)
    if (res || dis_int_lin)
      int_lin <= 1'b0;
    else if (int_start_lin)
      int_lin <= 1'b1;
    else if (intack_s && !int_frm)    // priority 1
      int_lin <= 1'b0;

  always @(posedge clk)
    if (res || dis_int_dma)
      int_dma <= 1'b0;
    else if (int_start_dma)
      int_dma <= 1'b1;
    else if (intack_s && !int_frm && !int_lin)    // priority 2
      int_dma <= 1'b0;

`ifdef COPPER
  always @(posedge clk)
    if (res || dis_int_cpr)
      int_cpr <= 1'b0;
    else if (int_start_cpr)
      int_cpr <= 1'b1;
    else if (intack_s && !int_frm && !int_lin && !int_dma)    // priority 3
      int_cpr <= 1'b0;
`endif

  always @(posedge clk)
    if (res || dis_int_wtp)
      int_wtp <= 1'b0;
    else if (int_start_wtp)
      int_wtp <= 1'b1;
`ifdef COPPER
    else if (intack_s && !int_frm && !int_lin && !int_dma && !int_cpr)    // priority 4
`else
    else if (intack_s && !int_frm && !int_lin && !int_dma)    // priority 3
`endif
      int_wtp <= 1'b0;

// ~WAIT resync
  reg wait_r;
    always @(posedge clk)
        wait_r <= !wait_n;

// ~INT counter
  reg [5:0] intctr;
  assign intctr_fin = intctr[5];   // 32 clks

  always @(posedge clk, posedge int_start_frm)
  begin
    if (int_start_frm)
      intctr <= 1'b0;
    else if (zpos && !intctr_fin && !wait_r && !vdos)
      intctr <= intctr + 1'b1;
  end

endmodule
