// This module renders video data for output


module video_render (

// clocks
	input wire clk,
	input wire q2,
	input wire c0,
	input wire c4,
	input wire c6,
	
// video controls
	input wire pix_start,
	input wire hvpix,
	input wire blank,

// mode controls
	input wire hires,

// video data
	input  wire [31:0] data_in,
	input  wire [ 7:0] border,
	output wire [ 7:0] data_out
	
);


	wire pix_stb = hires ? q2 : c6;

	wire get_zx = pix_start | (cnt == 4'b1111);

// pixel counter
	reg [3:0] cnt;
	
	always @(posedge clk) if (pix_stb)
		cnt <= pix_start ? 0 : cnt + 1;


// video data fetcher
	reg  [31:0] data;
	
	always @(posedge clk) if (pix_stb)
		if (get_zx)
			data <= data_in;


// ZX mode graphics render
	wire [15:0] w_zx_gfx = data[15: 0];
	wire [15:0] w_zx_atr = data[31:16];
	wire [ 7:0]	zx_attr		= cnt[3] ? w_zx_atr[7:0] : w_zx_atr[15:8];
	wire 		zx_gfx 		= w_zx_gfx [~cnt[3:0]];
	wire [ 2:0]	zx_pixc		= zx_gfx ? zx_attr[2:0] : zx_attr[5:3];
	wire [ 1:0]	zx_col		= zx_attr[6] ? 2'b11 : 2'b10;
	wire [ 1:0]	zx_pixr		= zx_pixc[1] ? zx_col : 2'b00;
	wire [ 1:0]	zx_pixg		= zx_pixc[2] ? zx_col : 2'b00;
	wire [ 1:0]	zx_pixb		= zx_pixc[0] ? zx_col : 2'b00;
	wire [ 7:0]	zx_color	= {zx_pixr, zx_pixg, zx_pixb, 2'b0};

	wire [ 7:0] pixel = zx_color;
	
// output video data render
	assign data_out = blank ? 0 : (hvpix ? pixel : border);
		
		

	
endmodule