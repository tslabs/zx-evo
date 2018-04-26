// (c) NedoPC 2010
//
// wait generator for Z80

`include "tune.v"

module zwait(
  input  wire fclk,
  input  wire rst_n,
  input  wire wait_start_gluclock,
  input  wire wait_start_comport,
  input  wire wait_end,
  input  wire dma_wtp_req,
  output wire dma_wtp_stb,
  input  wire [1:0] dmawpdev,
  input  wire wr_n,
  output wire [1:0] wait_status,
  output wire wait_status_wrn,
  output wire wait_n,
  output wire spiint_n
);

  assign spiint_n = !(wait_status_glu || wait_status_com || wait_status_dma);
  assign wait_n = wait_cpu ? 1'b0 : 1'bZ;
  assign wait_status = wait_status_dma ? (1'b1 << dmawpdev) : {wait_status_com, wait_status_glu};
  assign wait_status_wrn = wait_status_dma ? 1'b1 : wr_n;
  assign dma_wtp_stb = !wtp_stb && wait_off;

  wire wait_off = wait_end || !rst_n;
  wire wait_off_dma = dma_wtp_stb || !rst_n;
  wire wait_cpu = wait_status_glu || wait_status_com;

  reg wait_status_glu;
  reg wait_status_com;
  reg wait_status_dma;

  always @(posedge wait_start_gluclock, posedge wait_off)
  if (wait_off)
    wait_status_glu <= 1'b0;
  else if (wait_start_gluclock)
    wait_status_glu <= 1'b1;

  always @(posedge wait_start_comport, posedge wait_off)
  if (wait_off)
    wait_status_com <= 1'b0;
  else if (wait_start_comport)
    wait_status_com <= 1'b1;

  always @(posedge fclk)
  if (wait_off_dma)
    wait_status_dma <= 1'b0;
  else if (dma_wtp_req)
    wait_status_dma <= 1'b1;

  reg wtp_stb;
  always @(posedge fclk)
    wtp_stb <= wait_off;

endmodule
