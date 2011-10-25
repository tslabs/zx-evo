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
	input wire tv_blank,

// mode controls
	input wire [1:0] render_mode,
	input wire hires,

// video data
	input  wire [31:0] dram_in,
	input  wire [ 7:0] border,
	output wire [ 7:0] vdata_out
	
);

    localparam R_ZX = 2'h0;
    localparam R_HC = 2'h1;
    localparam R_02 = 2'h2;
    localparam R_03 = 2'h3;

    
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
			data <= dram_in;


// ZX graphics
	wire [15:0] w_zx_gfx = data[15: 0];
	wire [15:0] w_zx_atr = data[31:16];
	wire 		zx_bit 	 = w_zx_gfx [~cnt[3:0]];
	wire [ 7:0]	zx_attr	 = cnt[3] ? w_zx_atr[7:0] : w_zx_atr[15:8];
	wire [ 7:0]	zx_pix	 = {4'b1111, zx_attr[6], zx_bit ? zx_attr[2:0] : zx_attr[5:3]};

    
// mode mux
    wire [7:0] d_out[0:3];
    
    assign d_out[0] = zx_pix;
    assign d_out[1] = zx_pix;
    assign d_out[2] = zx_pix;
    assign d_out[3] = zx_pix;
    
	assign vdata_out = hvpix ? d_out[render_mode] : border;
	
endmodule