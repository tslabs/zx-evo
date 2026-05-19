`include "tune.v"

// Minimal video top for ACE fork: text + 256c, palette CRAM and VGA doubler kept.

module video_top
(
  input  wire        clk,
  input  wire        res,
  input  wire        f1,
  input  wire        h1,
  input  wire        c0,
  input  wire        c1,
  input  wire        c3,

  output wire [1:0]  vred,
  output wire [1:0]  vgrn,
  output wire [1:0]  vblu,
  output wire [4:0]  vred_raw,
  output wire [4:0]  vgrn_raw,
  output wire [4:0]  vblu_raw,
  output wire        vdac_mode,
`ifdef IDE_VDAC2
  output wire        vdac2_msel,
`endif

  output wire        hsync,
  output wire        vsync,
  output wire        csync,
  output wire [8:0]  ray_x,
  output wire [8:0]  ray_y,

  input  wire        cfg_60hz,
  input  wire        vga_on,

  input  wire [7:0]  border,
  input  wire [7:0]  vpage,
  input  wire [7:0]  vconf,
  input  wire [8:0]  gx_offs,
  input  wire [8:0]  gy_offs,
  input  wire [7:0]  palsel,

  input  wire [7:0]  cram_addr,
  input  wire [15:0] cram_data,
  input  wire        cram_we,

  output wire [20:0] video_addr,
  output wire [4:0]  video_bw,
  output wire        video_go,
  input  wire [15:0] dram_rdata,
  input  wire        video_strobe,
  input  wire        video_pre_next
);

  wire [8:0] hpix_beg;
  wire [8:0] hpix_end;
  wire [8:0] vpix_beg;
  wire [8:0] vpix_end;
  wire [8:0] hpix_beg_ts;
  wire [8:0] hpix_end_ts;
  wire [8:0] vpix_beg_ts;
  wire [8:0] vpix_end_ts;
  wire [5:0] x_tiles;
  wire [9:0] x_offs_mode;
  wire [4:0] go_offs;
  wire [1:0] render_mode;
  wire tv_hires;
  wire vga_hires;
  wire v60hz;
  wire nogfx = vconf[5];
`ifdef IDE_VDAC2
  assign vdac2_msel = vconf[2];
`endif

  wire [7:0] cnt_col;
  wire [8:0] cnt_row;
  wire cptr;
  wire [3:0] scnt;
  wire [8:0] lcount;

  wire pix_start;
  wire tv_blank;
  wire vga_blank;
  wire vga_line;
  wire v_ts;
  wire v_pf;
  wire hpix;
  wire vpix;
  wire hvpix;
  wire hvtspix;
  wire frame;
  wire flash;
  wire pix_stb;
  wire line_start_s;
  wire frame_start_s;
  wire int_start_s;

  wire [31:0] fetch_data;
  wire [31:0] fetch_temp;
  wire [3:0] fetch_sel;
  wire [1:0] fetch_bsl;
  wire fetch_stb;

  wire [7:0] vplex;
  wire [7:0] vgaplex;
  wire [9:0] vga_cnt_in;
  wire [9:0] vga_cnt_out;
  wire [8:0] ts_raddr;
  wire ts_start_unused;

`ifdef PENT_312
  wire [4:0] hcnt;
  wire upper8;
`endif

  video_mode video_mode
  (
    .clk          (clk),
    .f1           (f1),
    .c3           (c3),
    .vpage        (vpage),
    .vconf        (vconf),
    .v60hz        (v60hz),
    .fetch_sel    (fetch_sel),
    .fetch_bsl    (fetch_bsl),
    .fetch_cnt    (scnt),
    .fetch_stb    (fetch_stb),
    .txt_char     (fetch_temp[15:0]),
    .gx_offs      (gx_offs),
    .x_offs_mode  (x_offs_mode),
    .ts_rres_ext  (1'b0),
    .hpix_beg     (hpix_beg),
    .hpix_end     (hpix_end),
    .vpix_beg     (vpix_beg),
    .vpix_end     (vpix_end),
    .hpix_beg_ts  (hpix_beg_ts),
    .hpix_end_ts  (hpix_end_ts),
    .vpix_beg_ts  (vpix_beg_ts),
    .vpix_end_ts  (vpix_end_ts),
    .x_tiles      (x_tiles),
    .go_offs      (go_offs),
    .cnt_col      (cnt_col),
    .cnt_row      (cnt_row),
    .cptr         (cptr),
    .line_start_s (line_start_s),
    .pix_start    (pix_start),
    .tv_hires     (tv_hires),
    .vga_hires    (vga_hires),
    .pix_stb      (pix_stb),
    .render_mode  (render_mode),
    .video_addr   (video_addr),
    .video_bw     (video_bw)
  );

  video_sync video_sync
  (
    .clk            (clk),
    .f1             (f1),
    .c0             (c0),
    .c3             (c3),
    .hpix_beg       (hpix_beg),
    .hpix_end       (hpix_end),
    .vpix_beg       (vpix_beg),
    .vpix_end       (vpix_end),
    .hpix_beg_ts    (hpix_beg_ts),
    .hpix_end_ts    (hpix_end_ts),
    .vpix_beg_ts    (vpix_beg_ts),
    .vpix_end_ts    (vpix_end_ts),
    .go_offs        (go_offs),
    .x_offs         (x_offs_mode[1:0]),
    .y_offs_wr      (1'b0),
    .line_start_s   (line_start_s),
    .frame_start_s  (frame_start_s),
    .int_start_s    (int_start_s),
    .hint_beg       (8'd1),
    .vint_beg       (9'd0),
    .hsync          (hsync),
    .vsync          (vsync),
    .csync          (csync),
    .tv_blank       (tv_blank),
    .vga_blank      (vga_blank),
    .vga_cnt_in     (vga_cnt_in),
    .vga_cnt_out    (vga_cnt_out),
    .ts_raddr       (ts_raddr),
    .lcount         (lcount),
    .ray_x          (ray_x),
    .ray_y          (ray_y),
    .cnt_col        (cnt_col),
    .cnt_row        (cnt_row),
    .cptr           (cptr),
    .scnt           (scnt),
`ifdef PENT_312
    .hcnt           (hcnt),
    .upper8         (upper8),
`endif
    .frame          (frame),
    .flash          (flash),
    .pix_stb        (pix_stb),
    .pix_start      (pix_start),
    .ts_start       (ts_start_unused),
    .cstart         (x_offs_mode[9:2]),
    .rstart         (gy_offs),
    .vga_line       (vga_line),
    .v_pf           (v_pf),
    .hpix           (hpix),
    .v_ts           (v_ts),
    .vpix           (vpix),
    .hvpix          (hvpix),
    .hvtspix        (hvtspix),
    .nogfx          (nogfx),
    .vga_on         (vga_on),
    .cfg_60hz       (cfg_60hz),
    .v60hz          (v60hz),
    .video_go       (video_go),
    .video_pre_next (video_pre_next)
  );

  video_fetch video_fetch
  (
    .clk          (clk),
    .f_sel        (fetch_sel),
    .b_sel        (fetch_bsl),
    .fetch_stb    (fetch_stb),
    .fetch_data   (fetch_data),
    .fetch_temp   (fetch_temp),
    .video_strobe (video_strobe),
    .video_data   (dram_rdata)
  );

  video_render video_render
  (
    .clk        (clk),
    .c1         (c1),
    .hvpix      (hvpix),
    .hvtspix    (hvtspix),
    .nogfx      (nogfx),
    .notsu      (1'b1),
    .gfxovr      (1'b0),
    .flash       (flash),
    .hires       (tv_hires),
    .psel        (scnt),
    .palsel      (palsel[3:0]),
    .render_mode (render_mode),
    .data        (fetch_data),
    .border_in   (border),
    .tsdata_in   (8'h00),
    .vplex_out   (vplex)
  );

  video_out video_out
  (
    .clk          (clk),
    .c3           (c3),
    .vga_on       (vga_on),
    .tv_blank     (tv_blank),
    .vga_blank    (vga_blank),
    .vga_line     (vga_line),
    .palsel       (palsel[3:0]),
    .plex_sel_in  ({h1, f1}),
    .tv_hires     (tv_hires),
    .vga_hires    (vga_hires),
    .cram_addr_in (cram_addr),
    .cram_data_in (cram_data),
    .cram_we      (cram_we),
    .vplex_in     (vplex),
    .vgaplex      (vgaplex),
    .vred         (vred),
    .vgrn         (vgrn),
    .vblu         (vblu),
    .vred_raw     (vred_raw),
    .vgrn_raw     (vgrn_raw),
    .vblu_raw     (vblu_raw),
    .vdac_mode    (vdac_mode)
  );

  altdpram video_vmem
  (
    .inclock        (clk),
    .outclock       (clk),
    .wren           (c3),
    .data           (vplex),
    .rdaddress      (vga_cnt_out),
    .wraddress      (vga_cnt_in),
    .q              (vgaplex),
    .aclr           (1'b0),
    .byteena        (1'b1),
    .inclocken      (1'b1),
    .outclocken     (1'b1),
    .rdaddressstall (1'b0),
    .rden           (1'b1),
    .wraddressstall (1'b0)
  );

  defparam
    video_vmem.indata_aclr = "OFF",
    video_vmem.indata_reg = "INCLOCK",
    video_vmem.intended_device_family = "ACEX1K",
    video_vmem.lpm_type = "altdpram",
    video_vmem.outdata_aclr = "OFF",
    video_vmem.outdata_reg = "OUTCLOCK",
    video_vmem.rdaddress_aclr = "OFF",
    video_vmem.rdaddress_reg = "INCLOCK",
    video_vmem.rdcontrol_aclr = "OFF",
    video_vmem.rdcontrol_reg = "UNREGISTERED",
    video_vmem.width = 8,
    video_vmem.widthad = 10,
    video_vmem.wraddress_aclr = "OFF",
    video_vmem.wraddress_reg = "INCLOCK",
    video_vmem.wrcontrol_aclr = "OFF",
    video_vmem.wrcontrol_reg = "INCLOCK";

endmodule
