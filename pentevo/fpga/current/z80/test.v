module test(

	input  wire [ 7:0] din,
	output wire [ 7:0] dout,
	input  wire [15:0] a,
	input  wire portfe_wr_fclk,
	input  wire clk
	);
	
	reg [7:0] tst[0:15];
	
	// assign dout = tst[a[11:8]];
	// assign dout = tst[a[11:8]] ^ tst[a[15:12]];
	assign dout = a[15] ? 8'b1 : tst[a[11:8]];
	
	// always @(posedge clk)
	// if (portfe_wr_fclk)
		// tst[a[11:8]] <= din;

	always @(posedge portfe_wr_fclk)
		tst[a[11:8]] <= din;
	
endmodule
