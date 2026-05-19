`include "tune.v"

// Byte ROM programmer bus for ACE fork.
// Uses the former Z80 address/data pins while Z80 is held in reset.

module rom_bus
(
  input  wire        clk,
  input  wire        rst,

  input  wire        req,
  input  wire        rnw,
  input  wire [20:0] addr,
  input  wire [ 7:0] wdata,
  output reg  [ 7:0] rdata,
  output reg         ack,

  inout  wire [15:0] a,
  inout  wire [ 7:0] d,
  output wire        csrom,
  output wire        romoe_n,
  output wire        romwe_n,
  output wire        rompg0_n,
  output wire        dos_n,
  output wire        rompg2,
  output wire        rompg3,
  output wire        rompg4
);

  reg [2:0] state = 3'd0;
  reg [4:0] wait_cnt = 5'd0;
  reg [20:0] addr_r = 21'd0;
  reg [7:0] wdata_r = 8'd0;
  reg rnw_r = 1'b1;

  localparam IDLE         = 3'd0;
  localparam SETUP        = 3'd1;
  localparam WR_LOW       = 3'd2;
  localparam WR_HIGH      = 3'd3;
  localparam RD_WAIT      = 3'd4;
  localparam DONE         = 3'd5;
  localparam WAIT_REQ_LOW = 3'd6;

  localparam ROM_RD_WAIT_CYCLES = 5'd16;
  localparam ROM_WR_LOW_CYCLES  = 5'd16;
  localparam ROM_WR_HOLD_CYCLES = 5'd4;

  wire active = (state != IDLE) && (state != WAIT_REQ_LOW);
  wire write_drive = !rnw_r && (state == SETUP || state == WR_LOW || state == WR_HIGH || state == DONE);
  wire read_oe = rnw_r && (state == RD_WAIT);
  wire write_we = !rnw_r && (state == WR_LOW);

  assign a[13:0] = active ? addr_r[13:0] : 14'bzzzzzzzzzzzzzz;
  assign a[15:14] = active ? 2'b00 : 2'bZZ;
  assign d = write_drive ? wdata_r : 8'hZZ;

  assign csrom = active;
  assign romoe_n = !read_oe;
  assign romwe_n = !write_we;

  assign rompg0_n = ~addr_r[14];
  assign dos_n    =  addr_r[15];
  assign rompg2   =  addr_r[16];
  assign rompg3   =  addr_r[17];
  assign rompg4   =  addr_r[18];

  always @(posedge clk)
  begin
    ack <= 1'b0;

    if (rst)
    begin
      state <= IDLE;
      wait_cnt <= 5'd0;
      rdata <= 8'hFF;
    end
    else
    begin
      case (state)
        IDLE:
        begin
          if (req)
          begin
            addr_r <= addr;
            wdata_r <= wdata;
            rnw_r <= rnw;
            state <= SETUP;
          end
        end

        SETUP:
        begin
          if (rnw_r)
          begin
            wait_cnt <= ROM_RD_WAIT_CYCLES;
            state <= RD_WAIT;
          end
          else
          begin
            wait_cnt <= ROM_WR_LOW_CYCLES;
            state <= WR_LOW;
          end
        end

        WR_LOW:
        begin
          if (wait_cnt != 5'd0)
            wait_cnt <= wait_cnt - 5'd1;
          else
          begin
            wait_cnt <= ROM_WR_HOLD_CYCLES;
            state <= WR_HIGH;
          end
        end

        WR_HIGH:
        begin
          if (wait_cnt != 5'd0)
            wait_cnt <= wait_cnt - 5'd1;
          else
            state <= DONE;
        end

        RD_WAIT:
        begin
          if (wait_cnt != 5'd0)
            wait_cnt <= wait_cnt - 5'd1;
          else
          begin
            rdata <= d;
            state <= DONE;
          end
        end

        DONE:
        begin
          ack <= 1'b1;
          state <= WAIT_REQ_LOW;
        end

        WAIT_REQ_LOW:
        begin
          if (!req)
            state <= IDLE;
        end

        default: state <= IDLE;
      endcase
    end
  end

endmodule
