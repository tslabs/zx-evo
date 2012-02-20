
// Decoding and strobing of z80 signals

`include "../include/tune.v"

module zsignals(

// clocks
	input wire clk,
	input wire zclk,

// board controls
	input wire rst_n,
    
// z80 interface input
	input wire iorq_n,
	input wire mreq_n,
	input wire m1_n,
	input wire rd_n,
	input wire wr_n,

// signals output
	output wire rst,
    
	output wire rd,
	output wire wr,
	output wire rdwr,
    
	output wire iorq,
	output wire iorq_s,
	output wire iord,
	output wire iowr,
	output wire iorw,
    
	output wire mreq,
	output wire memrd,
	output wire memwr,
	output wire memrw,
    
	output wire m1

);
    
    assign rst = !rst_n;
    
    assign rd = !rd_n;
    assign wr = !wr_n;
    assign rdwr = rd | wr;
    
    assign iorq = !iorq_n;
    assign iorq_s = !iorq_r & iorq;
    assign iord = iorq & rd;
    assign iowr = iorq & wr;
    assign iorw = iorq & (rd | wr);
    
    assign mreq = !mreq_n;
    assign memrd = mreq & rd;
    assign memwr = mreq & wr;
    assign memrw = mreq & (rd | wr);
    
    assign m1 = !m1_n;
    
    
// z80 strobed signals
    reg iorq_r;
    always @(posedge clk)
        iorq_r <= iorq;
    

endmodule
