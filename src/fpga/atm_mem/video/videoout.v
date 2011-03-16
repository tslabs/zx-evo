`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// resyncs and outs video data to the DAC plus syncs

module videoout(

	input clk,

	input [5:0] pixel,  // this data has format: { red[1:0], green[1:0], blue[1:0] }
	input [5:0] border, //


	input hblank,
	input vblank,

	input hpix,
	input vpix,


	input hsync,
	input vsync,

	input vga_hsync,


	input wire scanin_start,
	input wire scanout_start,

	input wire hsync_start,


	output reg [1:0] vred, // to
	output reg [1:0] vgrn, //   the     DAC
	output reg [1:0] vblu, //      video

	output reg vhsync,
	output reg vvsync,

	output reg vcsync,

	input  wire cfg_vga_on
);


	wire [5:0] color, vga_color;

	assign color = (hblank | vblank) ? 6'd0 : (  (hpix & vpix) ? pixel : border  );



	vga_double vga_double( .clk(clk),

	                       .hsync_start(hsync_start),
	                       .scanin_start(scanin_start),
	                       .scanout_start(scanout_start),

	                       .pix_in(color),
	                       .pix_out(vga_color)
	                     );



	always @(posedge clk)
	begin
		vred[1:0] <= cfg_vga_on ? vga_color[5:4] : color[5:4];
		vgrn[1:0] <= cfg_vga_on ? vga_color[3:2] : color[3:2];
		vblu[1:0] <= cfg_vga_on ? vga_color[1:0] : color[1:0];

		vhsync <= cfg_vga_on ? vga_hsync : hsync;
		vvsync <= vsync;

		vcsync <= ~(hsync ^ vsync);
	end


endmodule

