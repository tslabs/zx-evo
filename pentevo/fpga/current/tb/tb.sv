
`include "tune.v"

module tb();

  reg clk = 0;
  
  initial
    forever #1 
      clk <= !clk;

  clock clock
  (
    .clk    (clk),
    .ay_mod ('b00)
  );

endmodule
