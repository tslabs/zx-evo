// (c) NedoPC 2010
//
// doubles video line by replicating it in 3x512b RAM buffer

module vga_double(

	input  wire        clk,

	input  wire        hsync_start,

	input  wire        scanin_start,
	input  wire [ 5:0] pix_in,

	input  wire        scanout_start,
	output reg  [ 5:0] pix_out
);

/*
addressing of non-overlapping pages:

pg0 pg1
0xx 1xx
2xx 3xx
4xx 5xx
*/

	reg [9:0] ptr_in;  // count up to 768 or $300
	reg [9:0] ptr_out; //

	reg pages; // swapping of pages

	reg wr_stb;

	wire [ 7:0] data_out;


	always @(posedge clk) if( hsync_start )
		pages <= ~pages;


	// write ptr and strobe
	always @(posedge clk)
	begin
		if( scanin_start )
		begin
			ptr_in[9:8] <= 2'b00;
		end
		else
		begin
			if( ptr_in[9:8]!=2'b11 )
			begin
				wr_stb <= ~wr_stb;
				if( wr_stb )
				begin
					ptr_in <= ptr_in + 10'd1;
				end
			end
		end
	end


	// read ptr
	always @(posedge clk)
	begin
		if( scanout_start )
		begin
			ptr_out[9:8] <= 2'b00;
		end
		else
		begin
			if( ptr_out[9:8]!=2'b11 )
			begin
				ptr_out <= ptr_out + 10'd1;
			end
		end
	end

	//read data
	always @(posedge clk)
	begin
		if( ptr_out[9:8]!=2'b11 )
			pix_out <= data_out[5:0];
		else
			pix_out <= 6'd0;
	end





	mem1536 line_buf( .clk(clk),

	                  .wraddr({ptr_in[9:8], pages, ptr_in[7:0]}),
	                  .wrdata({2'b00,pix_in}),
	                  .wr_stb(wr_stb),

	                  .rdaddr({ptr_out[9:8], (~pages), ptr_out[7:0]}),
	                  .rddata(data_out)
	                );


endmodule




// 3x512b memory
module mem1536(

	input  wire        clk,

	input  wire [10:0] wraddr,
	input  wire [ 7:0] wrdata,
	input  wire        wr_stb,

	input  wire [10:0] rdaddr,
	output reg  [ 7:0] rddata
);

	reg [7:0] mem [0:1535];

	always @(posedge clk)
	begin
		if( wr_stb )
		begin
			mem[wraddr] <= wrdata;
		end

		rddata <= mem[rdaddr];
	end


endmodule

