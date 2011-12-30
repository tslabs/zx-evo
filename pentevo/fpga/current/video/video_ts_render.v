// This is TS-graphics renderer

module video_ts_render (

// clocks
	input wire clk,
	input wire c0,
	input wire c2,

// video controls
	input wire tspix_start,
	input wire line_start,
	input wire frame_start,
	input wire lsel,

// video data
	output wire [7:0] tsdata,
	
// buffer interface
	input wire [8:0] tsbuf_wr_addr,
	input wire [8:0] tsbuf_wr_data,
	input wire [8:0] tsbuf_we
	
);


// buffer readback counter
	reg [8:0] cnt;

	always @(posedge clk) if (c0)
		cnt <= tspix_start ? 0 : cnt + 1;


// TS line video buffer
	wire [7:0] tb_rd0;
	wire [7:0] tb_rd1;
	
	assign tsdata = lsel ? tb_rd1 : tb_rd0;
	// assign tsdata = 8'h2;
	
	wire [8:0] tb_wa0 = lsel ? tsbuf_wr_addr : cnt;
	wire [7:0] tb_wd0 = lsel ? tsbuf_wr_data : 8'b0;
	wire tb_we0 = lsel ? tsbuf_we : c2;

	wire [8:0] tb_wa1 = ~lsel ? tsbuf_wr_addr : cnt;
	wire [7:0] tb_wd1 = ~lsel ? tsbuf_wr_data : 8'b0;
	wire tb_we1 = ~lsel ? tsbuf_we : c2;

	
video_tsbuf0 video_tsbuf0 (
	// .clock (clk),
	.clock (0),
	.data (tb_wd0),
	.rdaddress (cnt),
	.wraddress (tb_wa0),
	.wren (tb_we0),
	.q (tb_rd0)
	);
	
	
video_tsbuf1 video_tsbuf1 (
	// .clock (clk),
	.clock (0),
	.data (tb_wd1),
	.rdaddress (cnt),
	.wraddress (tb_wa1),
	.wren (tb_we1),
	.q (tb_rd1)
	);
	

endmodule


