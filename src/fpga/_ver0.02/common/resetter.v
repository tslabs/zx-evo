module resetter(

	clk,

	rst_in_n,

	rst_out_n );

parameter RST_CNT_SIZE = 4;


	input clk;

	input rst_in_n; // input of external asynchronous reset

	output rst_out_n; // output of end-synchronized reset (beginning is asynchronous to clock)
	reg    rst_out_n;



	reg [RST_CNT_SIZE:0] rst_cnt; // one bit more for counter stopping

	reg rst1_n,rst2_n;


	always @(posedge clk, negedge rst_in_n)
	if( !rst_in_n ) // external asynchronous reset
	begin
		rst_cnt <= 0;
		rst1_n <= 1'b0;
		rst2_n <= 1'b0;
		rst_out_n <= 1'b0; // this zeroing also happens after FPGA configuration, so also power-up reset happens
	end
	else // clocking
	begin
		rst1_n <= 1'b1;
		rst2_n <= rst1_n;

		if( rst2_n && !rst_cnt[RST_CNT_SIZE] )
		begin
			rst_cnt <= rst_cnt + 1;
		end

		if( rst_cnt[RST_CNT_SIZE] )
		begin
			rst_out_n <= 1'b1;
		end
	end


endmodule

