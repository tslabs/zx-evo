
// Decoding and strobing of z80 signals

`include "../include/tune.v"

module zsignals(

// clocks
	input wire clk,
	input wire zclk,

// z80 interface input
	input wire rst_n,
	input wire iorq_n,
	input wire mreq_n,
	input wire m1_n,
	input wire rfsh_n,
	input wire rd_n,
	input wire wr_n,

// signals output
	output wire rst,
	output wire m1,
	output wire rfsh,
	output wire rd,
	output wire wr,

	output wire iorq,
	output reg iorq_s,
	output reg iorq_s2,
	output wire mreq,
	output reg mreq_s,

	output wire rdwr,

	output wire iord,
	output wire iowr,
	output wire iorw,

	output wire iord_s,
	output wire iowr_s,
	output wire iorw_s,

	output wire memrd,
	output wire memwr,
	output wire memrw,

	output wire memrd_s,
	output wire memwr_s,
	output wire memrw_s,

	output wire opfetch,
	output wire opfetch_s,

	output wire intack,
	output wire intack_s

);

// invertors
    assign rst = !rst_n;
    assign m1 = !m1_n;
    assign rfsh = !rfsh_n;
    assign rd = !rd_n;
    assign wr = !wr_n;

// requests
    assign iorq = !iorq_n & m1_n;       // this is masked by ~M1 to avoid port decoding on INT ack
    assign mreq = !mreq_n & rfsh_n;     // this is masked by ~RFSH to ignore refresh cycles as memory requests

// combined
    assign rdwr = rd | wr;

    assign iord = iorq & rd;
    assign iowr = iorq & wr;
    assign iorw = iorq & rdwr;

    assign iord_s = iorq_s & rd;
    assign iowr_s = iorq_s & wr;
    assign iorw_s = iorq_s & rdwr;

    assign memrd = mreq & rd;
    assign memwr = mreq & !rd;
    assign memrw = mreq & rdwr;

    assign memrd_s = mreq_s & rd;
    assign memwr_s = mreq_s & !rd;
    assign memrw_s = mreq_s & rdwr;

    assign opfetch = memrd & m1;
    assign opfetch_s = memrd_s & m1;

    assign intack = !iorq_n & m1;
    assign intack_s = iorq_s & m1;


// latch inputs on Z80 clock
    reg iorq_zr;
    reg mreq_zr;
	always @(posedge zclk)
	begin
           iorq_zr <= iorq;
           mreq_zr <= mreq;
	end

// latch inputs on FPGA clock
    reg [1:0] iorq_r;
    reg [1:0] mreq_r;
	always @(posedge clk)
    begin
        iorq_r <= {iorq_r[0], iorq_zr};
        mreq_r <= {mreq_r[0], mreq_zr};
    end

// generate 1 clock strobes
	always @(posedge clk)
	begin
		if (iorq_r[1])
			iorq_s <= 1'b0;
		else
			iorq_s <= iorq_zr;

		if (mreq_r[1])
			mreq_s <= 1'b0;
		else
			mreq_s <= mreq_zr;

        iorq_s2 <= iorq_s;
    end

endmodule
