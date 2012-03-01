
// Decoding and strobing of z80 signals

`include "../include/tune.v"

module zsignals(

// clocks
	input wire clk,

// board controls
	input wire rst_n,

// z80 interface input
	input wire iorq_n,
	input wire mreq_n,
	input wire m1_n,
	input wire rfsh_n,
	input wire rd_n,
	input wire wr_n,

// signals output
	output wire rst,

	output wire rd,
	output wire wr,
	output wire rdwr,

	output wire iorq,
	output wire iord,
	output wire iowr,
	output wire iorw,
	output wire iorq_s,
	output wire iord_s,
	output wire iowr_s,
	output wire iorw_s,

	output wire mreq,
	output wire memrd,
	output wire memwr,
	output wire memwr_s,
	output wire memrw,

	output wire m1,
	output wire rfsh

);

    assign rst = !rst_n;

    assign rd = !rd_n;
    assign wr = !wr_n;
    assign rdwr = rd | wr;

    assign iorq = !iorq_n;
    assign iord = iorq & rd;
    assign iowr = iorq & wr;
    assign iorw = iorq & (rd | wr);
    
    assign iorq_s = iorq_r[1];
    assign iord_s = iorq_s & rd;
    assign iowr_s = iorq_s & wr;
    assign iorw_s = iorq_s & (rd | wr);

    assign mreq = !mreq_n;
    assign memrd = mreq & rd;
    assign memwr = mreq & !rfsh & !rd;
    assign memrw = mreq & (rd | wr);
    
    assign memwr_s = memwr_r[1];

    assign m1 = !m1_n;
    assign rfsh = !rfsh_n;


// z80 strobed signals
    reg [1:0] iorq_r;
    reg [1:0] memwr_r;
    always @(posedge clk)
    begin
        if (iorq)
            if (~|iorq_r)
                iorq_r <= 2'b11;
            else
                iorq_r[1] <= 1'b0;
        else
            iorq_r <= 2'b00;

        if (memwr)
            if (~|memwr_r)
                memwr_r <= 2'b11;
            else
                memwr_r[1] <= 1'b0;
        else
            memwr_r <= 2'b00;
    end


endmodule
