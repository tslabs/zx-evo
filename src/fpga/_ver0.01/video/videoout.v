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

	output reg [1:0] vred, // to
	output reg [1:0] vgrn, //   the     DAC
	output reg [1:0] vblu, //      video

	output reg vhsync,
	output reg vvsync,

	output reg vcsync
);


	wire [5:0] color;


	assign color = (hblank | vblank) ? 6'd0 : (  (hpix & vpix) ? pixel : border  );

	always @(posedge clk)
	begin
		vred[1:0] <= color[5:4];
		vgrn[1:0] <= color[3:2];
		vblu[1:0] <= color[1:0];

		vhsync <= hsync;
		vvsync <= vsync;

		vcsync <= ~(hsync ^ vsync);
	end


endmodule

