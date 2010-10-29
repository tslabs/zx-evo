// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on

// SFILE: Sprites description file
// 2048 bit = Write: 256x8, Read: 32x64

module sfile (
	input  wire clk,
	input  wire [7:0] wa,
	input  wire [7:0] wd,
	input  wire we,
	input  wire [4:0] ra,
	output reg [63:0] rd
);
	reg [63:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [(wa[7:3])][(wa[2:0]*8+7)-:(wa[2:0]*8)] <= wd;
		rd <= mem[ra];
	end
endmodule
