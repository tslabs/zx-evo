`include "tune.v"

// Minimal AVR-visible registers for SETUP fork.
// Keeps selected original TSConf/PentEvo register numbers used by video and SPI routing.

module avr_regs
(
  input  wire        clk,
  input  wire        rst,

  input  wire        periph_wr,
  input  wire        periph_rd,
  input  wire [21:0] periph_addr,
  input  wire [ 7:0] periph_wdata,
  output reg  [ 7:0] periph_rdata,

  output reg  [ 7:0] vconf,
  output reg  [ 7:0] vpage,
  output reg  [ 8:0] gx_offs,
  output reg  [ 8:0] gy_offs,
  output reg  [ 7:0] palsel,
  output reg  [ 7:0] border,
  output reg  [ 7:0] sysconf,

  output reg         cram_we,
  output reg  [ 7:0] cram_addr,
  output reg  [15:0] cram_data,

  output reg  [ 3:0] spi_cs_n,
  output reg         spi_mode
`ifdef ESP32_SPI
  , output reg       esp_ft_spi_dis
  , output reg       espcs_int
`endif
);

  localparam VCONF   = 8'h00;
  localparam VPAGE   = 8'h01;
  localparam GXOFFSL = 8'h02;
  localparam GXOFFSH = 8'h03;
  localparam GYOFFSL = 8'h04;
  localparam GYOFFSH = 8'h05;
  localparam PALSEL  = 8'h07;
  localparam XBORDER = 8'h0F;
  localparam SYSCONF = 8'h20;
  localparam SDDAT   = 8'h57;
  localparam SDCFG   = 8'h77;

  reg [7:0] cram_low;
  wire reg_window = periph_addr[21:8] == 14'd0;
  wire cram_window = periph_addr[21:9] == 13'd0 && periph_addr[8];

  initial
  begin
    periph_rdata = 8'hFF;
    vconf = 8'h83;
    vpage = 8'h00;
    gx_offs = 9'd0;
    gy_offs = 9'd0;
    palsel = 8'h0F;
    border = 8'h00;
    sysconf = 8'h01;
    cram_we = 1'b0;
    cram_addr = 8'h00;
    cram_data = 16'h0000;
    cram_low = 8'h00;
    spi_cs_n = 4'b1111;
    spi_mode = 1'b0;
`ifdef ESP32_SPI
    esp_ft_spi_dis = 1'b1;
    espcs_int = 1'b0;
`endif
  end

  always @(posedge clk)
  begin
    cram_we <= 1'b0;

    if (rst)
    begin
      vconf <= 8'h83;
      vpage <= 8'h00;
      gx_offs <= 9'd0;
      gy_offs <= 9'd0;
      palsel <= 8'h0F;
      border <= 8'h00;
      sysconf <= 8'h01;
      cram_addr <= 8'h00;
      cram_data <= 16'h0000;
      cram_low <= 8'h00;
      spi_cs_n <= 4'b1111;    // {ESP, SD2, FT, SD}
      spi_mode <= 1'b0;
`ifdef ESP32_SPI
      esp_ft_spi_dis <= 1'b1;
      espcs_int <= 1'b0;
`endif
    end
    else if (periph_wr)
    begin
      if (cram_window)
      begin
        cram_addr <= periph_addr[8:1];
        if (!periph_addr[0])
          cram_low <= periph_wdata;
        else
        begin
          cram_data <= {periph_wdata, cram_low};
          cram_we <= 1'b1;
        end
      end
      else if (reg_window)
      begin
        case (periph_addr[7:0])
          VCONF:   vconf <= periph_wdata;
          VPAGE:   vpage <= periph_wdata;
          GXOFFSL: gx_offs[7:0] <= periph_wdata;
          GXOFFSH: gx_offs[8] <= periph_wdata[0];
          GYOFFSL: gy_offs[7:0] <= periph_wdata;
          GYOFFSH: gy_offs[8] <= periph_wdata[0];
          PALSEL:  palsel <= periph_wdata;
          XBORDER: border <= periph_wdata;
          SYSCONF: sysconf <= periph_wdata;
          SDCFG:
          begin
            spi_cs_n <= {~periph_wdata[4:2], periph_wdata[1]};
            spi_mode <= 1'b0;
`ifdef SPI_MODE_EN
`ifdef ESP32_SPI
            if (periph_wdata[0]) spi_mode <= periph_wdata[7];
`else
            spi_mode <= periph_wdata[7];
`endif
`endif
`ifdef ESP32_SPI
            if (!periph_wdata[0])
            begin
              // esp_ft_spi_dis <= periph_wdata[7];
              esp_ft_spi_dis <= 1'b1;
              espcs_int <= 1'b0;
            end
`endif
          end
          default: ;
        endcase
      end
    end
  end

  always @*
  begin
    periph_rdata = 8'hFF;
    if (reg_window)
    begin
      case (periph_addr[7:0])
        VCONF:   periph_rdata = vconf;
        VPAGE:   periph_rdata = vpage;
        GXOFFSL: periph_rdata = gx_offs[7:0];
        GXOFFSH: periph_rdata = {7'b0, gx_offs[8]};
        GYOFFSL: periph_rdata = gy_offs[7:0];
        GYOFFSH: periph_rdata = {7'b0, gy_offs[8]};
        PALSEL:  periph_rdata = palsel;
        XBORDER: periph_rdata = border;
        SYSCONF: periph_rdata = sysconf;
        SDDAT:   periph_rdata = 8'hFF;
`ifdef ESP32_SPI
        SDCFG:   periph_rdata = {espcs_int, spi_mode, 2'b00, ~spi_cs_n};
`else
        SDCFG:   periph_rdata = {spi_mode, 3'b000, ~spi_cs_n};
`endif
        default: periph_rdata = 8'hFF;
      endcase
    end
  end

endmodule
