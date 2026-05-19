`include "tune.v"

// Minimal DRAM arbiter for ACE fork: video + AVR only.

module avr_arbiter
(
  input  wire        clk,
  input  wire        c1,
  input  wire        c2,
  input  wire        c3,

  output wire [20:0] dram_addr,
  output wire        dram_req,
  output wire        dram_rnw,
  output wire [ 1:0] dram_bsel,
  output wire [15:0] dram_wrdata,

  input  wire [20:0] video_addr,
  input  wire        video_go,
  input  wire [ 4:0] video_bw,
  output wire        video_pre_next,
  output wire        video_next,
  output wire        video_strobe,

  input  wire [20:0] avr_addr,
  input  wire [15:0] avr_wrdata,
  input  wire [ 1:0] avr_bsel,
  input  wire        avr_req,
  input  wire        avr_rnw,
  output wire        avr_ack
);

  localparam CYC_FREE = 2'b00;
  localparam CYC_AVR  = 2'b01;
  localparam CYC_VID  = 2'b10;

  reg [1:0] curr_cycle = CYC_FREE;
  reg [1:0] next_cycle;
  reg [2:0] blk_rem = 3'd0;
  reg [2:0] vid_rem = 3'd0;
  reg stall = 1'b0;

  wire video_start = ~|blk_rem;
  wire [2:0] blk_nrem = (video_start && video_go) ? {video_bw[4:3], 1'b1} : (video_start ? 3'd0 : (blk_rem - 3'd1));
  wire bw_full = ~|{video_bw[4] & video_bw[2], video_bw[3] & video_bw[1], video_bw[0]};
  wire video_only = stall || (vid_rem == blk_rem);
  wire video_idle = ~|vid_rem;
  wire next_vid = next_cycle == CYC_VID;
  wire next_avr = next_cycle == CYC_AVR;
  wire curr_vid = curr_cycle == CYC_VID;
  wire curr_avr = curr_cycle == CYC_AVR;

  always @(posedge clk) if (c3)
  begin
    blk_rem <= blk_nrem;
    if (video_start)
      stall <= bw_full & video_go;
  end

  wire [2:0] vidmax = video_bw[2:0];
  wire [2:0] vid_nrem_next = video_idle ? 3'd0 : (vid_rem - 3'd1);
  wire [2:0] vid_nrem_start = avr_req ? vidmax : (vidmax - 3'd1);
  wire [2:0] vid_nrem = (video_go && video_start) ? vid_nrem_start : (next_vid ? vid_nrem_next : vid_rem);

  always @(posedge clk) if (c3)
    vid_rem <= vid_nrem;

  always @*
    if (video_start)
      if (video_go)
        next_cycle = bw_full ? CYC_VID : (avr_req ? CYC_AVR : CYC_VID);
      else
        next_cycle = avr_req ? CYC_AVR : CYC_FREE;
    else
      next_cycle = video_only ? CYC_VID : (avr_req ? CYC_AVR : (!video_idle ? CYC_VID : CYC_FREE));

  always @(posedge clk) if (c3)
    curr_cycle <= next_cycle;

  assign dram_wrdata = avr_wrdata;
  assign dram_bsel = avr_bsel;
  assign dram_req = next_cycle != CYC_FREE;
  assign dram_rnw = next_avr ? avr_rnw : 1'b1;
  assign dram_addr = next_avr ? avr_addr : (next_vid ? video_addr : 21'd0);

  assign video_pre_next = curr_vid & c1;
  assign video_next = curr_vid & c2;
  assign video_strobe = curr_vid & c3;
  assign avr_ack = curr_avr & c3;

endmodule
