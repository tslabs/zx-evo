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

sfile0 sfile0 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd0)), .ra(ra), .rd(rd[7:0]));
sfile1 sfile1 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd1)), .ra(ra), .rd(rd[15:8]));
sfile2 sfile2 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd2)), .ra(ra), .rd(rd[23:16]));
sfile3 sfile3 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd3)), .ra(ra), .rd(rd[31:24]));
sfile4 sfile4 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd4)), .ra(ra), .rd(rd[39:32]));
sfile5 sfile5 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd5)), .ra(ra), .rd(rd[47:40]));
sfile6 sfile6 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd6)), .ra(ra), .rd(rd[55:48]));
sfile7 sfile7 (.clk(clk),.wa(wa[4:0]), .wd(wd), .we(we && (wa[7:5] == 3'd7)), .ra(ra), .rd(rd[63:56]));
endmodule


module sfile0(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile1(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile2(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile3(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile4(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile5(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile6(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule


module sfile7(	input wire clk, we,
				input wire [7:0] wa, wd,
				input wire [4:0] ra,
				output reg [7:0] rd );

	reg [7:0] mem [0:31];
	always @(posedge clk)
	begin
		if(we) mem [wa] <= wd;
		rd <= mem[ra];
	end
endmodule
