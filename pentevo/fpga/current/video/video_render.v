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

// mode controls
	input wire [1:0] render_mode,
	input wire hires,

// video data
	input  wire [31:0] dram_in,
	input  wire [ 3:0] border,
	output wire [ 7:0] vdata_out
	
);

    localparam R_ZX = 2'h0;
    localparam R_HC = 2'h1;
    localparam R_XC = 2'h2;
    localparam R_TX = 2'h3;

    
	wire pix_stb = hires ? q2 : c6;	//fixme!!! take out to the mode decoder!

// pixel counter
	reg [3:0] cnt;
	
	always @(posedge clk) if (pix_stb)
		cnt <= pix_start ? 0 : cnt + 1;


// video data fetcher
	reg  [31:0] data;
	
	always @(posedge clk) if (pix_stb & fetch)
			data <= dram_in;


// ZX graphics
	localparam ZX_PAL = 4'hF;
	
	wire [15:0] zx_gfx = data[15: 0];
	wire [15:0] zx_atr = data[31:16];
	wire zx_dot = zx_gfx[{cnt[3], ~cnt[2:0]}];
	wire [7:0] zx_attr	= ~cnt[3] ? zx_atr[7:0] : zx_atr[15:8];
	wire [7:0] zx_pix = {ZX_PAL, zx_attr[6], zx_dot ? zx_attr[2:0] : zx_attr[5:3]};

    
// 16c graphics
	localparam HC_PAL = 4'hE;
	
	wire [3:0] hc_dot[0:7];
	assign hc_dot[0] = data[ 7: 4];
	assign hc_dot[1] = data[ 3: 0];
	assign hc_dot[2] = data[15:12];
	assign hc_dot[3] = data[11: 8];
	assign hc_dot[4] = data[23:20];
	assign hc_dot[5] = data[19:16];
	assign hc_dot[6] = data[31:28];
	assign hc_dot[7] = data[27:24];
	wire [7:0] hc_pix = {HC_PAL, hc_dot[cnt[2:0]]};
	
    
// 256c graphics
	wire [7:0] xc_dot[0:3];
	assign xc_dot[0] = data[ 7: 0];
	assign xc_dot[1] = data[15: 8];
	assign xc_dot[2] = data[23:16];
	assign xc_dot[3] = data[31:24];
	wire [7:0] xc_pix = {xc_dot[cnt[1:0]]};


// text graphics
// (it uses common renderer with ZX, but different attributes, it also shares palette with 16c)
	wire [7:0] tx_pix = {HC_PAL, zx_dot ? zx_attr[3:0] : zx_attr[7:4]};

    
// mode selects
    wire [7:0] d_out[0:3];
    assign d_out[R_ZX] = zx_pix;	// ZX
    assign d_out[R_HC] = hc_pix;	// 16c
    assign d_out[R_XC] = xc_pix;	// 256c
    assign d_out[R_TX] = tx_pix;	// text


	wire ftch[0:3];
	wire fetch = pix_start | ftch[render_mode];
	assign ftch[R_ZX] = cnt[3:0] == 4'b1111;
	assign ftch[R_HC] = cnt[2:0] == 3'b111;
	assign ftch[R_XC] = cnt[1:0] == 2'b11;
	assign ftch[R_TX] = cnt[3:0] == 4'b1111;

	
// mix video data and border
	assign vdata_out = hvpix ? d_out[render_mode] : {4'hF, border};
	
	
	
endmodule

