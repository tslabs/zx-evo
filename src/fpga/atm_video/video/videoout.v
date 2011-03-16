`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// resyncs and outs video data to the DAC plus syncs

module videoout(

	input clk,

	input [3:0] pixel,
	input [3:0] border,


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

	input  wire cfg_vga_on,

	input  wire rst_n,
	input  wire wr_pal64,
	input  wire [5:0] newrealcolor
);


	reg  [5:0] realcolor;
	wire [3:0] colorindex;
	wire [5:0] color, vga_color;

	assign colorindex = (hpix & vpix) ? pixel : border;
	assign color = (hblank | vblank) ? 6'd0 : realcolor;



	vga_double vga_double( .clk(clk),

	                       .hsync_start(hsync_start),
	                       .scanin_start(scanin_start),
	                       .scanout_start(scanout_start),

	                       .pix_in(realcolor),
	                       .pix_out(vga_color)
	                     );



	always @(posedge clk)
	begin
		vgrn[1:0] <= cfg_vga_on ? vga_color[5:4] : color[5:4];
		vred[1:0] <= cfg_vga_on ? vga_color[3:2] : color[3:2];
		vblu[1:0] <= cfg_vga_on ? vga_color[1:0] : color[1:0];

		vhsync <= cfg_vga_on ? vga_hsync : hsync;
		vvsync <= vsync;

		vcsync <= ~(hsync ^ vsync);
	end


// ----------------------------------------------------------------------------

 reg [5:0] palette [0:15];

 always @(posedge clk, negedge rst_n)
 begin
  if (!rst_n)
   begin
    palette[ 0]<=6'b000000;
    palette[ 1]<=6'b000010;
    palette[ 2]<=6'b001000;
    palette[ 3]<=6'b001010;
    palette[ 4]<=6'b100000;
    palette[ 5]<=6'b100010;
    palette[ 6]<=6'b101000;
    palette[ 7]<=6'b101010;
    palette[ 8]<=6'b000000;
    palette[ 9]<=6'b000011;
    palette[10]<=6'b001100;
    palette[11]<=6'b001111;
    palette[12]<=6'b110000;
    palette[13]<=6'b110011;
    palette[14]<=6'b111100;
    palette[15]<=6'b111111;
    realcolor<=6'b000000;
   end
  else if (wr_pal64)
   begin
    palette[colorindex]<=newrealcolor;
    realcolor<=newrealcolor;
   end
  else
   begin
    realcolor<=palette[colorindex];
   end
 end

// ----------------------------------------------------------------------------

endmodule

