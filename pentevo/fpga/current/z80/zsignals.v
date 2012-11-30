
// Decoding and strobing of z80 signals

`include "../include/tune.v"

module zsignals(

// clocks
	input wire clk,
	input wire zclk,
	input wire zpos,

// z80 interface input
	input wire rst_n,
	input wire iorq_n,
	input wire mreq_n,
	input wire m1_n,
	input wire rfsh_n,
	input wire rd_n,
	input wire wr_n,

// Z80 signals
	output wire rst,
	output wire m1,
	output wire rfsh,
	output wire rd,
	output wire wr,
	output wire iorq,
	output wire mreq,
	output wire rdwr,
	output wire iord,
	output wire iowr,
	output wire iorw,
	output wire memrd,
	output wire memwr,
	output wire memrw,
	output wire opfetch,
	output wire intack,

// Z80 signals strobes, at fclk
	output reg iorq_s,
	// output reg iorq_s2,
	output reg mreq_s,
	output reg iord_s,
	output reg iowr_s,
	output reg iorw_s,
	output reg memrd_s,
	output reg memwr_s,
	output reg memrw_s,
	output reg opfetch_s,
	output reg intack_s
);

// invertors
    assign rst = !rst_n;
    assign m1 = !m1_n;
    assign rfsh = !rfsh_n;
    assign rd = !rd_n;
    assign wr = !wr_n;

// requests
    assign iorq = !iorq_n && m1_n;       // this is masked by ~M1 to avoid port decoding on INT ack
    assign mreq = !mreq_n && rfsh_n;     // this is masked by ~RFSH to ignore refresh cycles as memory requests

// combined
    assign rdwr = rd | wr;

    assign iord = iorq && rd;
    assign iowr = iorq && wr;
    assign iorw = iorq && rdwr;

    assign memrd = mreq && rd;
    assign memwr = mreq && !rd;
    assign memrw = mreq && rdwr;

    assign opfetch = memrd && m1;

    assign intack = !iorq_n && m1;


// latch inputs on FPGA clock
	always @(posedge clk)
	if (zpos)
		begin
			iorq_s 	  <= iorq;
			mreq_s 	  <= mreq;
			iord_s 	  <= iord;
			iowr_s 	  <= iowr;
			iorw_s 	  <= iorw;
			memrd_s   <= memrd;
			memwr_s   <= memwr;
			memrw_s   <= memrw;
			opfetch_s <= opfetch;
			intack_s  <= intack;
		end
	else
		begin
			iorq_s 	  <= 1'b0;
			mreq_s 	  <= 1'b0;
			iord_s 	  <= 1'b0;
			iowr_s 	  <= 1'b0;
			iorw_s 	  <= 1'b0;
			memrd_s   <= 1'b0;
			memwr_s   <= 1'b0;
			memrw_s   <= 1'b0;
			opfetch_s <= 1'b0;
			intack_s  <= 1'b0;
		end
		
	// always @(posedge clk)
		// iorq_s2 <= iorq_s;

endmodule
