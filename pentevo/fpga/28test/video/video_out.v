// This module generates video for DAC

`include "../include/tune.v"


module video_out (

// video controls
    input wire vga_on,
    input wire tv_blank,
    input wire vga_blank,

// video data
	input wire [7:0] vplex_in,
	input wire [7:0] vgaplex,
	output reg [1:0] vred,
    output reg [1:0] vgrn,
    output reg [1:0] vblu
);


	wire [7:0] vdata = vga_on ? vgaplex : vplex_in;
    wire blank = vga_on ? vga_blank : tv_blank;
	assign vred = blank ? 0 : vdata[5:4];
	assign vgrn = blank ? 0 : vdata[3:2];
	assign vblu = blank ? 0 : vdata[1:0];

    
endmodule
