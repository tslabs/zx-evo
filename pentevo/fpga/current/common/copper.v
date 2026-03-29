module copper
(
  input wire clk,
  input wire res,

  input wire [7:0] cpu_data,    // CPU write to Copper control
  input wire cpu_wr,

  input wire cpu_xt_access,         // CPU is writing to a TS reg

  input wire [7:0] cl_wr_addr,  // CLRAM write
  input wire [15:0] cl_wr_data,
  input wire cl_wr,

  output wire [7:0] ts_reg_addr,  // TS reg write by Copper
  output wire [7:0] ts_reg_data,
  output wire ts_reg_wr,

  output wire sig_int,            // CPU INT by Copper
  output wire sig_rdy,            // READY by Copper

  input wire [7:0] ray_x,         // current screen counters
  input wire [8:0] ray_y,

  input wire dma_done,            // DMA transaction complete
  input wire line_start_s,        // Line start strobe
  input wire frame_start_s        // Frame start strobe
);

  localparam SIGNAL = 4'b0101;
  localparam SETA   = 4'b0110;
  localparam WAITX  = 4'b0111;
  localparam SETB   = 4'b100x;
  localparam WAITY  = 4'b101x;
  localparam DJNZA  = 4'b1100;
  localparam DJNZB  = 4'b1101;
  localparam CALL   = 4'b1110;
  localparam JUMP   = 4'b1111;

  reg en = 1'b0;
  reg [7:0] pc = 8'b0;
  reg [7:0] ret_pc = 8'b0;
  reg [7:0] a = 8'b0;
  reg [8:0] b = 9'b0;

  // CList RAM
  wire [15:0] cl_data;

  altdpram clist
  (
    .rdaddress      (pc),
    .q              (cl_data),
    .wraddress      (cl_wr_addr),
    .data           (cl_wr_data),
    .wren           (cl_wr),
    .inclock        (1'b1),
    .aclr           (1'b0),
    .byteena        (1'b1),
    .inclocken      (1'b1),
    .outclock       (1'b1),
    .outclocken     (1'b1),
    .rdaddressstall (1'b0),
    .rden           (1'b1),
    .wraddressstall (1'b0)
  );

  defparam
    clist.indata_aclr = "OFF",
    clist.indata_reg = "UNREGISTERED",
    clist.intended_device_family = "ACEX1K",
    clist.lpm_type = "altdpram",
    clist.outdata_aclr = "OFF",
    clist.outdata_reg = "UNREGISTERED",
    clist.rdaddress_aclr = "OFF",
    clist.rdaddress_reg = "UNREGISTERED",
    clist.rdcontrol_aclr = "OFF",
    clist.rdcontrol_reg = "UNREGISTERED",
    clist.width = 16,
    clist.widthad = 8,
    clist.wraddress_aclr = "OFF",
    clist.wraddress_reg = "UNREGISTERED",
    clist.wrcontrol_aclr = "OFF",
    clist.wrcontrol_reg = "UNREGISTERED";

  // TS reg writer
  assign ts_reg_addr = cl_data[15:8];
  assign ts_reg_data = cl_data[7:0];
  assign ts_reg_wr = en && (cl_data[15:12] != 4'b1111);

  // Signal generator
  assign sig_int = en && (cl_data[15:8] == {4'hF, SIGNAL}) && cl_data[0];
  assign sig_rdy = en && (cl_data[15:8] == {4'hF, SIGNAL}) && cl_data[1];

  // Event decoder
  logic evt;

  always_comb
    case (cl_data[3:0])
      4'd1:    evt = dma_done;      // DMA
      4'd2:    evt = frame_start_s; // Frame start
      4'd3:    evt = line_start_s;  // Line start
      default: evt = 1'b1;          // no event
    endcase

  wire [7:0] deca = a - 8'b1;
  wire [8:0] decb = b - 9'b1;
  wire [7:0] inc_pc = pc + 8'b1;

  // Instruction decoder
  always @(posedge clk)

    // Reset
    if (res)
      en <= 1'b0;

    // Start by CPU
    else if (cpu_wr)
      begin
          en <= cpu_data != 8'hFF;
          pc <= cpu_data;
      end

    // Normal work
    else if (en)
    begin
      if (cl_data[15:12] == 4'b1111)  // all instructions except WREG
        casex (cl_data[11:8])
          SETA:
          begin
            a <= cl_data[7:0];
            pc <= inc_pc;
          end

          SETB:
          begin
            b <= cl_data[8:0];
            pc <= inc_pc;
          end

          WAITX:
            if (cl_data[7:0] == ray_x) pc <= inc_pc;

          WAITY:
            if ((&cl_data[8:4] && evt) /* WAIT <evt> */ || (cl_data[8:0] == ray_y) /* WAITY */) pc <= inc_pc;
            // else if (cl_data[8:4] == 5'b11110) pc <= evt ? inc_pc : (inc_pc + 8'b1);   // SKIP <evt>

          DJNZA:
          begin
            a <= deca;
            pc <= (deca == 8'b0) ? inc_pc : cl_data[7:0];
          end

          DJNZB:
          begin
            b <= decb;
            pc <= (decb == 9'b0) ? inc_pc : cl_data[7:0];
          end

          CALL:
          begin
            ret_pc <= inc_pc;
            pc <= cl_data[7:0];
          end

          JUMP:
            pc <= (cl_data[7:0] == 8'hFF) ? ret_pc : cl_data[7:0];

          SIGNAL:
            pc <= inc_pc;    // do nothing, see above for signal logic

          default:
            pc <= inc_pc;    // reserved instructions
        endcase

        else // WREG
          if (!cpu_xt_access) pc <= inc_pc;    // give CPU write priority, see above for signal logic
    end

endmodule
