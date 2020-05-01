
`include "tune.v"

module tb();

  reg clk = 1;
  reg rst = 0;

  initial
    forever #1
      clk <= !clk;

  initial
  begin
    #2600ps rst <= 1;
    #6 rst <= 0;
  end

  assign top.genrst = rst;

  wire [7:0] d;
  wire [15:0] a;

  top top
  (
    .fclk     (clk),
    .clkz_in  (top.clkz_out),
    .iorq_n   (z80.nIORQ),
    .mreq_n   (z80.nMREQ),
    .rd_n     (z80.nRD),
    .wr_n     (z80.nWR),
    .m1_n     (z80.nM1),
    .rfsh_n   (z80.nRFSH),

    .d        (d),
    .a        (a),
    .iorqge1  (),
    .iorqge2  (),
    .rd       (),
    .step     (),
    .vg_sl    (),
    .vg_sr    (),
    .vg_tr43  (),
    .rdat_b_n (),
    .vg_drq   (),
    .vg_irq   (),
    .vg_wd    (),
    .sddi     (),
    .spics_n  (),
    .spick    (),
    .spido    ()
  );

  reg [7:0] rom[512 * 1024];
  initial $readmemh("mem/cache_test.hex", rom);
  // initial $readmemh("mem/zxevo.rom.hex", rom);
  // initial $readmemh("mem/dram_test.hex", rom);
  wire [18:0] rom_addr = {top.rompg4, top.rompg3, top.rompg2, top.dos_n, !top.rompg0_n, a[13:0]};
  assign d = (!top.romoe_n && top.csrom) ? rom[rom_addr] : 8'hZZ;
  
  pullup(top.int_n);
  pullup(top.nmi_n);
  pullup(top.wait_n);
  
  pullup(z80.nIORQ);
  pullup(z80.nMREQ);
  pullup(z80.nRD);
  pullup(z80.nWR);
  
  z80_top_direct_n z80
  (
    .A       (a),
    .D       (d),
    .CLK     (top.clkz_in),
    .nRESET  (top.rst_n),
    // .nWAIT   (top.wait_n),
    .nWAIT   (1),
    .nINT    (top.int_n),
    .nNMI    (top.nmi_n),
    .nBUSRQ  (1'b1)
  );

  dram_simple dram0
  (
    .ras_n (top.dram.rras0_n),
    .lcas_n (top.dram.rlcas_n),
    .ucas_n (top.dram.rucas_n),
    .we_n  (top.dram.rwe_n),
    .a     (top.dram.ra),
    .d     (top.rd)
  );

  dram_simple dram1
  (
    .ras_n (top.dram.rras1_n),
    .lcas_n (top.dram.rlcas_n),
    .ucas_n (top.dram.rucas_n),
    .we_n  (top.dram.rwe_n),
    .a     (top.dram.ra),
    .d     (top.rd)
  );

endmodule

module dram_simple
(
  input wire ras_n,
  input wire lcas_n,
  input wire ucas_n,
  input wire we_n,
  input wire [9:0] a,
  inout wire [15:0] d
);

  reg [15:0] mem[1024 * 1024];
  reg [9:0] row;
  reg [9:0] col;
  reg act;
  
  assign d[7:0]  = (!lcas_n && we_n && act) ? mem[{col, row}][7:0] : 8'hZZ;
  assign d[15:8] = (!ucas_n && we_n && act) ? mem[{col, row}][15:8] : 8'hZZ;
  
  always @(negedge ras_n)
    row <= a;
    
  always @(negedge lcas_n, negedge ucas_n)
  begin
    col <= a;
    act <= !ras_n;
    
    if (!we_n && !ras_n)
    begin
      if (!lcas_n) mem[{a, row}][7:0] <= d[7:0];
      if (!ucas_n) mem[{a, row}][15:8] <= d[15:8];
    end
  end
  
endmodule
