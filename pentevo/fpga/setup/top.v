`include "tune.v"

module top
(
  // clocks and external reset
  input fclk,
  output clkz_out,
  input clkz_in,

  // z80
  input iorq_n,
  input mreq_n,
  input rd_n,
  input wr_n,
  input m1_n,
  input rfsh_n,
  output int_n,
  output nmi_n,
  output wait_n,
  output res,

  inout [7:0] d,
  inout [15:0] a,

  // zxbus and related
  output csrom,
  output romoe_n,
  output romwe_n,

  output rompg0_n,
  output dos_n,
  output rompg2,
  output rompg3,
  output rompg4,

  input iorqge1,
  input iorqge2,
  output iorq1_n,
  output iorq2_n,

  // DRAM
  inout [15:0] rd,
  output [9:0] ra,
  output rwe_n,
  output rucas_n,
  output rlcas_n,
  output rras0_n,
  output rras1_n,

  // video
  output [1:0] vred,
  output [1:0] vgrn,
  output [1:0] vblu,

  output vhsync,
  output vvsync,
  output vcsync,

  // AY control and audio/tape
  output ay_clk,
  output ay_bdir,
  output ay_bc1,

  output beep,

  // IDE / VDAC / VDAC2 pins
`ifdef IDE_HDD
  inout [15:0] ide_d,
  output ide_rs_n,
`elsif IDE_VDAC
  output [15:0] ide_d,
  input ide_rs_n,
`elsif IDE_VDAC2
  output [15:0] ide_d,
  output ide_rs_n,
`endif

  output [2:0] ide_a,
  output ide_dir,
  output ide_cs0_n,
  output ide_cs1_n,
  output ide_rd_n,
  output ide_wr_n,
  input ide_rdy,

  // VG93 and FDD
  output vg_clk,

  output vg_cs_n,
  output vg_res_n,

  output vg_hrdy,
  output vg_rclk,
  output vg_rawr,
  output [1:0] vg_a,
  output vg_wrd,
  output vg_side,

  input step,
  input vg_sl,
  input vg_sr,
  input vg_tr43,
  input rdat_b_n,
  input vg_drq,
  input vg_irq,
  input vg_wd,

  // SPI SD-Card
  output sdcs_n,
`ifdef SD_CARD2
  output sd2cs_n,
`else
  input sd2cs_n,
`endif
  output sdclk,
  output sddo,
  input  sddi,

  // SPI Atmega-FPGA
  input spics_n,
  input spick,
  input spido,
  output spidi,
  output spiint_n
);

  wire f0, f1, h0, h1, c0, c1, c2, c3;
  wire genrst = 1'b0;
  wire rst_n;
  wire rst = !rst_n;

  wire [4:0] vred_raw;
  wire [4:0] vgrn_raw;
  wire [4:0] vblu_raw;
  wire vdac_mode;
`ifdef IDE_VDAC2
  wire vdac2_msel;
`endif

  wire [7:0] vconf;
  wire [7:0] vpage;
  wire [8:0] gx_offs;
  wire [8:0] gy_offs;
  wire [7:0] palsel;
  wire [7:0] border;
  wire [7:0] sysconf;

  wire [7:0] cram_addr;
  wire [15:0] cram_data;
  wire cram_we;

  wire [3:0] spi_cs_n;
  wire spi_mode;
`ifdef ESP32_SPI
  wire esp_ft_spi_dis;
  wire espcs_int;
`endif

  wire [20:0] daddr;
  wire dreq;
  wire drnw;
  wire [15:0] dram_wrdata;
  wire [15:0] dram_wd;
  wire [1:0] dbsel;

  wire [20:0] video_addr;
  wire [4:0] video_bw;
  wire video_go;
  wire video_strobe;
  wire video_pre_next;
  wire video_next;
  wire [8:0] ray_x_unused;
  wire [8:0] ray_y_unused;

  wire avr_dram_req;
  wire avr_dram_rnw;
  wire [20:0] avr_dram_addr;
  wire [15:0] avr_dram_wrdata;
  wire [1:0] avr_dram_bsel;
  wire avr_dram_ack;

  wire periph_wr;
  wire periph_rd;
  wire [21:0] periph_addr;
  wire [7:0] periph_wdata;
  wire [7:0] periph_rdata;

  wire rom_req;
  wire rom_rnw;
  wire [20:0] rom_addr;
  wire [7:0] rom_wdata;
  wire [7:0] rom_rdata;
  wire rom_ack;

  wire proxy_active;
  wire target_sd0 = !spi_cs_n[0];
`ifdef SD_CARD2
  wire target_sd2 = !spi_cs_n[2];
`else
  wire target_sd2 = 1'b0;
`endif
  wire target_sd = target_sd0 || target_sd2;
`ifdef IDE_VDAC2
  wire target_ft = !spi_cs_n[1];
`ifdef ESP32_SPI
  wire target_esp = !spi_cs_n[3];
`else
  wire target_esp = 1'b0;
`endif
  wire target_vdac_spi = target_ft || target_esp;
`else
  wire target_ft = 1'b0;
  wire target_esp = 1'b0;
  wire target_vdac_spi = 1'b0;
`endif
  wire proxy_miso = target_vdac_spi ? ide_rdy : (target_sd ? sddi : 1'b1);

  assign clkz_out = f0;
  assign int_n = 1'bZ;
  assign nmi_n = 1'bZ;
  assign wait_n = 1'bZ;
  assign res = 1'b1;       // keep Z80 reset asserted
  assign iorq1_n = 1'b1;
  assign iorq2_n = 1'b1;
  assign spiint_n = 1'b1;

  assign ay_bdir = 1'b0;
  assign ay_bc1 = 1'b0;
  assign beep = 1'b0;

  assign vg_clk = 1'b0;
  assign vg_cs_n = 1'b1;
  assign vg_res_n = 1'b0;
  assign vg_hrdy = 1'b0;
  assign vg_rclk = 1'b0;
  assign vg_rawr = 1'b0;
  assign vg_a = 2'b00;
  assign vg_wrd = 1'b0;
  assign vg_side = 1'b0;

  assign sdcs_n = spi_cs_n[0];
`ifdef SD_CARD2
  assign sd2cs_n = spi_cs_n[2];
`endif
  assign sdclk = (proxy_active && target_sd) ? spick : 1'b0;
  assign sddo = (proxy_active && target_sd) ? spido : 1'b1;

  assign rd = rwe_n ? 16'hZZZZ : dram_wd;

`ifdef IDE_HDD
  assign ide_d = 16'hZZZZ;
  assign ide_rs_n = 1'b1;
  assign ide_dir = 1'b1;
  assign ide_a = 3'b000;
  assign ide_cs0_n = 1'b1;
  assign ide_cs1_n = 1'b1;
  assign ide_rd_n = 1'b1;
  assign ide_wr_n = 1'b1;
`elsif IDE_VDAC
  assign ide_d[ 4: 0] = vred_raw;
  assign ide_d[ 9: 5] = vgrn_raw;
  assign ide_d[14:10] = vblu_raw;
  assign ide_d[15] = vdac_mode;
  assign ide_dir = 1'b0;
  assign ide_a[0] = 1'bZ;
  assign ide_a[1] = !fclk;
  assign ide_a[2] = vhsync;
  assign ide_rd_n = 1'bZ;
  assign ide_wr_n = 1'bZ;
  assign ide_cs0_n = 1'bZ;
  assign ide_cs1_n = vvsync;
`elsif IDE_VDAC2
  wire vdac2_video_drive = !vdac2_msel;
  assign ide_d[ 0] = vdac2_video_drive ? vgrn_raw[2] : 1'bZ;
  assign ide_d[ 1] = vdac2_video_drive ? vred_raw[0] : 1'bZ;
  assign ide_d[ 2] = vdac2_video_drive ? vred_raw[1] : 1'bZ;
  assign ide_d[ 3] = vdac2_video_drive ? vred_raw[2] : 1'bZ;
  assign ide_d[ 4] = vdac2_video_drive ? vred_raw[3] : 1'bZ;
  assign ide_d[ 5] = vdac2_video_drive ? vred_raw[4] : 1'bZ;
  assign ide_d[ 6] = vdac2_video_drive ? vgrn_raw[0] : 1'bZ;
  assign ide_d[ 7] = vdac2_video_drive ? vgrn_raw[1] : 1'bZ;
  assign ide_d[ 8] = vdac2_video_drive ? vgrn_raw[3] : 1'bZ;
  assign ide_d[ 9] = vdac2_video_drive ? vgrn_raw[4] : 1'bZ;
  assign ide_d[10] = vdac2_video_drive ? vblu_raw[0] : 1'bZ;
  assign ide_d[11] = vdac2_video_drive ? vblu_raw[1] : 1'bZ;
  assign ide_d[12] = vdac2_video_drive ? vblu_raw[2] : 1'bZ;
  assign ide_d[13] = vdac2_video_drive ? vblu_raw[3] : 1'bZ;
  assign ide_d[14] = vdac2_video_drive ? vblu_raw[4] : 1'bZ;
  assign ide_d[15] = vdac2_video_drive ? vdac_mode : 1'bZ;

  assign ide_dir = !vdac2_video_drive;
  assign ide_wr_n = !vdac2_video_drive;
  assign ide_a[2] = !fclk;
  assign ide_cs0_n = vhsync;
  assign ide_cs1_n = vvsync;
`ifdef ESP32_SPI
  assign ide_a[0] = esp_ft_spi_dis ? 1'bZ : ((proxy_active && target_vdac_spi) ? spick : 1'b0);
  assign ide_a[1] = esp_ft_spi_dis ? 1'bZ : ((proxy_active && target_vdac_spi) ? spido : 1'b1);
  assign ide_rd_n = esp_ft_spi_dis ? 1'bZ : spi_cs_n[1];
  assign ide_rs_n = esp_ft_spi_dis ? (spi_cs_n[3] ? 1'bZ : 1'b0) : spi_cs_n[3];
`else
  assign ide_a[0] = (proxy_active && target_vdac_spi) ? spick : 1'b0;
  assign ide_a[1] = (proxy_active && target_vdac_spi) ? spido : 1'b1;
  assign ide_rd_n = spi_cs_n[1];
  assign ide_rs_n = vgrn_raw[2];
`endif
`else
  assign ide_dir = 1'b1;
  assign ide_a = 3'b000;
  assign ide_cs0_n = 1'b1;
  assign ide_cs1_n = 1'b1;
  assign ide_rd_n = 1'b1;
  assign ide_wr_n = 1'b1;
`endif

  clock clock
  (
    .clk(fclk),
    .f0(f0),
    .f1(f1),
    .h0(h0),
    .h1(h1),
    .c0(c0),
    .c1(c1),
    .c2(c2),
    .c3(c3),
    .ay_clk(ay_clk),
    .ay_mod(2'b00)
  );

  resetter myrst
  (
    .clk(fclk),
    .rst_in_n(~genrst),
    .rst_out_n(rst_n)
  );

  avr_regs avr_regs
  (
    .clk(fclk),
    .rst(rst),
    .periph_wr(periph_wr),
    .periph_rd(periph_rd),
    .periph_addr(periph_addr),
    .periph_wdata(periph_wdata),
    .periph_rdata(periph_rdata),
    .vconf(vconf),
    .vpage(vpage),
    .gx_offs(gx_offs),
    .gy_offs(gy_offs),
    .palsel(palsel),
    .border(border),
    .sysconf(sysconf),
    .cram_we(cram_we),
    .cram_addr(cram_addr),
    .cram_data(cram_data),
    .spi_cs_n(spi_cs_n),
    .spi_mode(spi_mode)
`ifdef ESP32_SPI
    , .esp_ft_spi_dis(esp_ft_spi_dis)
    , .espcs_int(espcs_int)
`endif
  );

  avr_spi avr_spi
  (
    .clk(fclk),
    .rst_n(rst_n),
    .spics_n(spics_n),
    .spick(spick),
    .spido(spido),
    .spidi(spidi),
    .proxy_miso(proxy_miso),
    .proxy_active(proxy_active),
    .periph_wr(periph_wr),
    .periph_rd(periph_rd),
    .periph_addr(periph_addr),
    .periph_wdata(periph_wdata),
    .periph_rdata(periph_rdata),
    .dram_req(avr_dram_req),
    .dram_rnw(avr_dram_rnw),
    .dram_addr(avr_dram_addr),
    .dram_wrdata(avr_dram_wrdata),
    .dram_bsel(avr_dram_bsel),
    .dram_ack(avr_dram_ack),
    .dram_rdata(rd),
    .rom_req(rom_req),
    .rom_rnw(rom_rnw),
    .rom_addr(rom_addr),
    .rom_wdata(rom_wdata),
    .rom_ack(rom_ack),
    .rom_rdata(rom_rdata)
  );

  rom_bus rom_bus
  (
    .clk(fclk),
    .rst(rst),
    .req(rom_req),
    .rnw(rom_rnw),
    .addr(rom_addr),
    .wdata(rom_wdata),
    .rdata(rom_rdata),
    .ack(rom_ack),
    .a(a),
    .d(d),
    .csrom(csrom),
    .romoe_n(romoe_n),
    .romwe_n(romwe_n),
    .rompg0_n(rompg0_n),
    .dos_n(dos_n),
    .rompg2(rompg2),
    .rompg3(rompg3),
    .rompg4(rompg4)
  );

  dram dram
  (
    .clk(fclk),
    .rst_n(rst_n),
    .addr(daddr),
    .req(dreq),
    .rnw(drnw),
    .c0(c0),
    .c1(c1),
    .c2(c2),
    .c3(c3),
    .wrdata(dram_wrdata),
    .bsel(dbsel),
    .ra(ra),
    .dram_wd(dram_wd),
    .rwe_n(rwe_n),
    .rucas_n(rucas_n),
    .rlcas_n(rlcas_n),
    .rras0_n(rras0_n),
    .rras1_n(rras1_n)
  );

  avr_arbiter avr_arbiter
  (
    .clk(fclk),
    .c1(c1),
    .c2(c2),
    .c3(c3),
    .dram_addr(daddr),
    .dram_req(dreq),
    .dram_rnw(drnw),
    .dram_bsel(dbsel),
    .dram_wrdata(dram_wrdata),
    .video_addr(video_addr),
    .video_go(video_go),
    .video_bw(video_bw),
    .video_pre_next(video_pre_next),
    .video_next(video_next),
    .video_strobe(video_strobe),
    .avr_addr(avr_dram_addr),
    .avr_wrdata(avr_dram_wrdata),
    .avr_bsel(avr_dram_bsel),
    .avr_req(avr_dram_req),
    .avr_rnw(avr_dram_rnw),
    .avr_ack(avr_dram_ack)
  );

  video_top video_top
  (
    .clk(fclk),
    .res(rst),
    .f1(f1),
    .h1(h1),
    .c0(c0),
    .c1(c1),
    .c3(c3),
    .vred(vred),
    .vgrn(vgrn),
    .vblu(vblu),
    .vred_raw(vred_raw),
    .vgrn_raw(vgrn_raw),
    .vblu_raw(vblu_raw),
    .vdac_mode(vdac_mode),
`ifdef IDE_VDAC2
    .vdac2_msel(vdac2_msel),
`endif
    .hsync(vhsync),
    .vsync(vvsync),
    .csync(vcsync),
    .ray_x(ray_x_unused),
    .ray_y(ray_y_unused),
    .cfg_60hz(sysconf[4]),
    .vga_on(sysconf[0]),
    // .vga_on(1'b1),
    .border(border),
    .vpage(vpage),
    .vconf(vconf),
    .gx_offs(gx_offs),
    .gy_offs(gy_offs),
    .palsel(palsel),
    .cram_addr(cram_addr),
    .cram_data(cram_data),
    .cram_we(cram_we),
    .video_addr(video_addr),
    .video_bw(video_bw),
    .video_go(video_go),
    .dram_rdata(rd),
    .video_strobe(video_strobe),
    .video_pre_next(video_pre_next)
  );

endmodule
