`include "tune.v"

module spi
(
  // SPI wires
  input  wire       clk,      // system clock
  output wire       sck,      // SCK
  output reg        sdo,      // MOSI
  input  wire       sdi,      // MISO

  // DMA interface
  input  wire       dma_req,
  input  wire [7:0] dma_din,

  // Z80 interface
  input  wire       cpu_req,
  input  wire [7:0] cpu_din,

  // output
  output wire       start,    // start strobe, 1 clock length
  output reg  [7:0] dout
);

  wire req = cpu_req || dma_req;
  wire [7:0] din = dma_req ? dma_din : cpu_din;

`ifdef SPI_14
  reg [4:0] counter = 5'b10000;
  reg [7:0] shift = 0;

  wire busy = !counter[4];

  assign sck = counter[0];
  assign start = req && !busy;

  always @(posedge clk)
  begin
    if (start)
    begin
      counter <= 5'b0;
      sdo <= din[7];
      shift[7:1] <= din[6:0];
    end
    else
    begin
      if (busy)
      begin
        counter <= counter + 5'd1;

        if (!sck)
        begin
          shift[0] <= sdi;

          if (&counter[3:1])
            dout <= {shift[7:1], sdi};
        end

        if (sck)
        begin
          sdo <= shift[7];
          shift[7:1] <= shift[6:0];
        end
      end
    end
  end
`else

`ifdef SPI_14_MISO_LATE
  reg [4:0] counter = 5'b10000;
  reg [7:0] tx_shift = 0;
  reg [7:0] rx_shift = 0;

  wire busy = !counter[4];

  assign sck = counter[0];
  assign start = req && !busy;

  always @(posedge clk)
  begin
    if (start)
    begin
      counter <= 5'b0;
      sdo <= din[7];
      tx_shift[7:1] <= din[6:0];
    end
    else
    begin
      if (busy)
      begin
        counter <= counter + 5'd1;

        if (sck)
        begin
          sdo <= tx_shift[7];
          tx_shift[7:1] <= tx_shift[6:0];
        end
      end
    end
  end

  always @(negedge clk)
  begin
    if (busy && counter[0])
    begin
      rx_shift <= {rx_shift[6:0], sdi};

      if (&counter[3:1])
        dout <= {rx_shift[6:0], sdi};
    end
  end
`else

`ifdef SPI_28
  reg [3:0] counter = 4'd8;
  reg [7:0] tx_shift = 0;
  reg [7:0] rx_shift = 0;

  wire busy = counter[3] == 1'b0;

  assign sck = busy ? ~clk : 1'b0;
  assign start = req && !busy;

  always @(posedge clk)
  begin
    if (start)
    begin
      counter <= 4'd0;
      sdo <= din[7];
      tx_shift[7:1] <= din[6:0];
    end
    else if (busy)
    begin
      rx_shift <= {rx_shift[6:0], sdi};

      if (counter == 4'd7)
      begin
        counter <= 4'd8;
        dout <= {rx_shift[6:0], sdi};
      end
      else
      begin
        counter <= counter + 4'd1;
        sdo <= tx_shift[7];
        tx_shift[7:1] <= tx_shift[6:0];
      end
    end
  end
`endif
`endif
`endif

endmodule
