`include "../include/tune.v"

// Pentevo project (c) NedoPC 2010-2011
//
// mux out VGA and TV signals and make final v|h syncs, DAC data, etc.

module video_outmux(

	input  wire        clk,


	input  wire        vga_on,


	input  wire [ 5:0] tvcolor,
	input  wire [ 5:0] vgacolor,

	input  wire        vga_hsync,
	input  wire        hsync,
	input  wire        vsync,


	output reg  [ 1:0] vred,
	output reg  [ 1:0] vgrn,
	output reg  [ 1:0] vblu,

	output reg         vhsync,
	output reg         vvsync,
	output reg         vcsync
);


	always @(posedge clk)
	begin
		vgrn[1:0] <= vga_on ? vgacolor[5:4] : tvcolor[5:4];
		vred[1:0] <= vga_on ? vgacolor[3:2] : tvcolor[3:2];
		vblu[1:0] <= vga_on ? vgacolor[1:0] : tvcolor[1:0];

		vhsync <= vga_on ? vga_hsync : hsync;
		vvsync <= vsync;

		vcsync <= ~(hsync ^ vsync);
	end



endmodule

