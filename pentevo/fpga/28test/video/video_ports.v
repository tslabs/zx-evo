// This module latches all port parameters for video from Z80

`include "../include/tune.v"


module video_ports (

// clocks
	input wire clk,

// Z80 controls
	input wire [ 7:0] d,

// ZX controls
    input wire        res,

// port write strobes
    input wire zborder_wr,

// video parameters
	output reg [7:0] border

 );


    always @(posedge clk)
        if (zborder_wr)
			border <= d;


 endmodule