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
	reg [7:0] mem [0:255];
	reg [15:0] rd1, rd2, rd3, rd4;
	
	always @(posedge clk)
	begin
		if(we) 
		mem [wa] <= wd;
		rd1 = {mem[{ra, 3'd7}], mem[{ra, 3'd6}]};
		rd2 = {mem[{ra, 3'd5}], mem[{ra, 3'd4}]};
		rd3 = {mem[{ra, 3'd3}], mem[{ra, 3'd2}]};
		// rd4 = {mem[{ra, 3'd1}], mem[{ra, 3'd0}]};
		rd = {rd1, rd2};
	end
endmodule
