// This module renders video data for output

`include "../include/tune.v"


module video_render (

// clocks
	input wire clk, c1,
	
// video controls
	input wire hvpix,
	input wire nogfx,
	input wire flash,
	input wire txmode,
	input wire [3:0] psel,
	input wire [3:0] palsel,

// video data
	input  wire [31:0] data,
	input  wire [ 7:0] border_in,
	input  wire [ 7:0] tsdata_in,
	output wire [ 7:0] vplex_out
	
);


// ZX graphics
	wire [15:0] zx_gfx = data[15: 0];
	wire [15:0] zx_atr = data[31:16];
	wire zx_dot = zx_gfx[{psel[3], ~psel[2:0]}];
	wire [7:0] zx_attr	= ~psel[3] ? zx_atr[7:0] : zx_atr[15:8];
	wire [7:0] zx_pix = {palsel, zx_attr[6], zx_dot ^ (flash & zx_attr[7]) ? zx_attr[2:0] : zx_attr[5:3]};

    
// text graphics
// (it uses common renderer with ZX, but different attributes)
	wire [7:0] tx_pix = {palsel, zx_dot ? zx_attr[3:0] : zx_attr[7:4]};

    
// 16c graphics
	// wire [3:0] hc_dot[0:3];
	// assign hc_dot[0] = data[ 7: 4];
	// assign hc_dot[1] = data[ 3: 0];
	// assign hc_dot[2] = data[15:12];
	// assign hc_dot[3] = data[11: 8];
	// wire [7:0] hc_pix = {palsel, hc_dot[psel[1:0]]};
	
    
// 256c graphics
	// wire [7:0] xc_dot[0:1];
	// assign xc_dot[0] = data[ 7: 0];
	// assign xc_dot[1] = data[15: 8];
	// wire [7:0] xc_pix = {xc_dot[psel[0]]};


// video plex muxer
	wire [7:0] video = !hvpix ? border_in : (|tsdata_in ? tsdata_in : (nogfx ? border_in : (txmode ? tx_pix : zx_pix)));
	assign vplex_out = txmode ? {temp, video[3:0]} : video;		// in hi-res plex contains two pixels 4 bits each
	
	reg [3:0] temp;
	always @(posedge clk) if (c1)
		temp <= video[3:0];
	
	
endmodule

