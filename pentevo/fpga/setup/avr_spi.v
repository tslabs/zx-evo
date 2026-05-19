`include "tune.v"

// AVR master -> FPGA slave protocol for SETUP fork.
// SPI mode 0, MSB first inside each byte.
// Header byte order: header[23:16], header[15:8], header[7:0], then stream bytes until CS high.

module avr_spi
(
  input  wire        clk,
  input  wire        rst_n,

  input  wire        spics_n,
  input  wire        spick,
  input  wire        spido,
  output wire        spidi,

  input  wire        proxy_miso,
  output reg         proxy_active,

  output reg         periph_wr,
  output reg         periph_rd,
  output reg  [21:0] periph_addr,
  output reg  [ 7:0] periph_wdata,
  input  wire [ 7:0] periph_rdata,

  output reg         dram_req,
  output reg         dram_rnw,
  output reg  [20:0] dram_addr,
  output reg  [15:0] dram_wrdata,
  output reg  [ 1:0] dram_bsel,
  input  wire        dram_ack,
  input  wire [15:0] dram_rdata,

  output reg         rom_req,
  output reg         rom_rnw,
  output reg  [20:0] rom_addr,
  output reg  [ 7:0] rom_wdata,
  input  wire        rom_ack,
  input  wire [ 7:0] rom_rdata
);

  localparam OP_DRAM_WR = 2'b00;
  localparam OP_DRAM_RD = 2'b01;
  localparam OP_PER_WR  = 2'b10;
  localparam OP_PER_RD  = 2'b11;

  localparam ST_HEADER  = 3'd0;
  localparam ST_DRAM_WR = 3'd1;
  localparam ST_DRAM_RD = 3'd2;
  localparam ST_PER_WR  = 3'd3;
  localparam ST_PER_RD  = 3'd4;
  localparam ST_ROM_WR  = 3'd5;
  localparam ST_ROM_RD  = 3'd6;
  localparam ST_PROXY   = 3'd7;

  localparam ADDR_PROXY = 22'h000057;

  reg [1:0] cs_sync = 2'b11;
  reg [1:0] sck_sync = 2'b00;
  reg [1:0] mosi_sync = 2'b00;

  always @(posedge clk)
  begin
    cs_sync <= {cs_sync[0], spics_n};
    sck_sync <= {sck_sync[0], spick};
    mosi_sync <= {mosi_sync[0], spido};
  end

  wire cs_n = cs_sync[0];
  wire cs_fall = cs_sync[1] && !cs_sync[0];
  wire cs_rise = !cs_sync[1] && cs_sync[0];
  wire sck_rise = !sck_sync[1] && sck_sync[0];
  wire sck_fall = sck_sync[1] && !sck_sync[0];
  wire mosi = mosi_sync[1];

  reg [2:0] state = ST_HEADER;
  reg proxy_wait_fall = 1'b0;
  reg [1:0] op = 2'b00;
  reg [21:0] addr = 22'd0;
  reg [23:0] header_shift = 24'd0;
  reg [1:0] header_bytes = 2'd0;
  reg [2:0] bit_cnt = 3'd0;
  reg [7:0] rx_shift = 8'd0;
  reg [7:0] tx_shift = 8'hFF;
  reg miso_reg = 1'b1;
  reg tx_load;
  reg [7:0] tx_load_data;
  reg periph_load_pending = 1'b0;
  reg rom_pending = 1'b0;
  reg rom_pending_rnw = 1'b1;
  reg [20:0] rom_pending_addr = 21'd0;
  reg [7:0] rom_pending_wdata = 8'hFF;
  reg rom_rd_dummy = 1'b0;
  reg rom_rd_wait_load = 1'b0;
  reg rom_rd_valid = 1'b0;
  reg [7:0] rom_rd_data = 8'hFF;

  wire [7:0] rx_byte = {rx_shift[6:0], mosi};
  wire [23:0] header_next = {header_shift[15:0], rx_byte};
  wire rst = !rst_n;

  assign spidi = proxy_active ? proxy_miso : miso_reg;

  task load_tx;
    input [7:0] data;
    begin
      tx_load <= 1'b1;
      tx_load_data <= data;
    end
  endtask

  task start_dram_read;
    input [21:0] raddr;
    begin
      dram_req <= 1'b1;
      dram_rnw <= 1'b1;
      dram_addr <= raddr[21:1];
      dram_wrdata <= 16'h0000;
      dram_bsel <= 2'b11;
    end
  endtask

  task start_dram_write;
    input [21:0] waddr;
    input [7:0] data;
    begin
      dram_req <= 1'b1;
      dram_rnw <= 1'b0;
      dram_addr <= waddr[21:1];
      dram_wrdata <= waddr[0] ? {data, 8'h00} : {8'h00, data};
      dram_bsel <= waddr[0] ? 2'b10 : 2'b01;
    end
  endtask

  task issue_rom_op;
    input rnw;
    input [20:0] op_addr;
    input [7:0] data;
    begin
      rom_req <= 1'b1;
      rom_rnw <= rnw;
      rom_addr <= op_addr;
      rom_wdata <= data;
    end
  endtask

  task queue_rom_read;
    input [20:0] raddr;
    begin
      if (!rom_req && !rom_pending)
        issue_rom_op(1'b1, raddr, 8'hFF);
      else
      begin
        rom_pending <= 1'b1;
        rom_pending_rnw <= 1'b1;
        rom_pending_addr <= raddr;
        rom_pending_wdata <= 8'hFF;
      end
    end
  endtask

  task queue_rom_write;
    input [20:0] waddr;
    input [7:0] data;
    begin
      if (!rom_req && !rom_pending)
        issue_rom_op(1'b0, waddr, data);
      else
      begin
        rom_pending <= 1'b1;
        rom_pending_rnw <= 1'b0;
        rom_pending_addr <= waddr;
        rom_pending_wdata <= data;
      end
    end
  endtask

  always @(posedge clk)
  begin
    periph_wr <= 1'b0;
    periph_rd <= 1'b0;
    tx_load <= 1'b0;

    if (rst)
    begin
      state <= ST_HEADER;
      proxy_active <= 1'b0;
      proxy_wait_fall <= 1'b0;
      dram_req <= 1'b0;
      rom_req <= 1'b0;
      periph_load_pending <= 1'b0;
      rom_pending <= 1'b0;
      rom_rd_dummy <= 1'b0;
      rom_rd_wait_load <= 1'b0;
      rom_rd_valid <= 1'b0;
      tx_load <= 1'b0;
      miso_reg <= 1'b1;
      tx_shift <= 8'hFF;
    end
    else
    begin
      if (cs_fall)
      begin
        state <= ST_HEADER;
        proxy_active <= 1'b0;
        proxy_wait_fall <= 1'b0;
        periph_load_pending <= 1'b0;
        rom_rd_dummy <= 1'b0;
        rom_rd_wait_load <= 1'b0;
        rom_rd_valid <= 1'b0;
        header_shift <= 24'd0;
        header_bytes <= 2'd0;
        bit_cnt <= 3'd0;
        rx_shift <= 8'd0;
        tx_shift <= 8'hFF;
        miso_reg <= 1'b1;
      end

      if (cs_rise)
      begin
        proxy_active <= 1'b0;
        proxy_wait_fall <= 1'b0;
        periph_load_pending <= 1'b0;
        rom_rd_dummy <= 1'b0;
        rom_rd_wait_load <= 1'b0;
        rom_rd_valid <= 1'b0;
        state <= ST_HEADER;
      end

      if (proxy_wait_fall && !cs_n && sck_fall)
      begin
        proxy_active <= 1'b1;
        proxy_wait_fall <= 1'b0;
      end

      if (periph_load_pending)
      begin
        load_tx(periph_rdata);
        periph_load_pending <= 1'b0;
      end

      if (dram_ack && dram_req)
      begin
        if (dram_rnw)
          load_tx(addr[0] ? dram_rdata[15:8] : dram_rdata[7:0]);
        dram_req <= 1'b0;
      end

      if (rom_ack && rom_req)
      begin
        if (rom_rnw)
        begin
          rom_rd_data <= rom_rdata;
          rom_rd_valid <= 1'b1;
          if (rom_rd_wait_load || !rom_rd_dummy)
          begin
            load_tx(rom_rdata);
            rom_rd_wait_load <= 1'b0;
          end
        end
        rom_req <= 1'b0;
      end

      if (!rom_req && rom_pending)
      begin
        issue_rom_op(rom_pending_rnw, rom_pending_addr, rom_pending_wdata);
        rom_pending <= 1'b0;
      end

      if (!cs_n && sck_rise && !proxy_active)
      begin
        rx_shift <= rx_byte;

        if (bit_cnt == 3'd7)
        begin
          bit_cnt <= 3'd0;

          case (state)
            ST_HEADER:
            begin
              header_shift <= {header_shift[15:0], rx_byte};
              header_bytes <= header_bytes + 2'd1;

              if (header_bytes == 2'd2)
              begin
                op <= header_next[23:22];
                addr <= header_next[21:0];
                periph_addr <= header_next[21:0];

                case (header_next[23:22])
                  OP_DRAM_WR:
                    state <= ST_DRAM_WR;

                  OP_DRAM_RD:
                  begin
                    state <= ST_DRAM_RD;
                    start_dram_read(header_next[21:0]);
                  end

                  OP_PER_WR:
                  begin
                    if (header_next[21:0] == ADDR_PROXY)
                    begin
                      state <= ST_PROXY;
                      proxy_wait_fall <= 1'b1;
                    end
                    else if (header_next[21])
                      state <= ST_ROM_WR;
                    else
                      state <= ST_PER_WR;
                  end

                  OP_PER_RD:
                  begin
                    if (header_next[21])
                    begin
                      state <= ST_ROM_RD;
                      rom_rd_dummy <= 1'b1;
                      rom_rd_wait_load <= 1'b0;
                      rom_rd_valid <= 1'b0;
                      queue_rom_read(header_next[20:0]);
                    end
                    else
                    begin
                      state <= ST_PER_RD;
                      periph_rd <= 1'b1;
                      periph_load_pending <= 1'b1;
                    end
                  end
                endcase
              end
            end

            ST_DRAM_WR:
            begin
              start_dram_write(addr, rx_byte);
              addr <= addr + 22'd1;
            end

            ST_DRAM_RD:
            begin
              addr <= addr + 22'd1;
              start_dram_read(addr + 22'd1);
            end

            ST_PER_WR:
            begin
              periph_addr <= addr;
              periph_wdata <= rx_byte;
              periph_wr <= 1'b1;
              addr <= addr + 22'd1;
            end

            ST_PER_RD:
            begin
              addr <= addr + 22'd1;
              periph_addr <= addr + 22'd1;
              periph_rd <= 1'b1;
              periph_load_pending <= 1'b1;
            end

            ST_ROM_WR:
            begin
              queue_rom_write(addr[20:0], rx_byte);
              addr <= addr + 22'd1;
            end

            ST_ROM_RD:
            begin
              if (rom_rd_dummy)
              begin
                rom_rd_dummy <= 1'b0;
                if (rom_rd_valid)
                  load_tx(rom_rd_data);
                else
                  rom_rd_wait_load <= 1'b1;
              end
              else
              begin
                addr <= addr + 22'd1;
                rom_rd_valid <= 1'b0;
                queue_rom_read(addr[20:0] + 21'd1);
              end
            end

            default: ;
          endcase
        end
        else
          bit_cnt <= bit_cnt + 3'd1;
      end

      if (tx_load)
      begin
        tx_shift <= tx_load_data;
        miso_reg <= tx_load_data[7];
      end
      else if (!cs_n && sck_rise && !proxy_active)
      begin
        tx_shift <= {tx_shift[6:0], 1'b1};
        miso_reg <= tx_shift[6];
      end
    end
  end

endmodule
