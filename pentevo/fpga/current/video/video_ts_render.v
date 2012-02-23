`include "../include/tune.v"

// This module renders pixels for tiles/sprites in 16c mode
// It receives as input:
// - DRAM address to read from
// - number of cycles to render (one cycle is 2 words = 8 pixels)
// - palette selector
// - X flip bit
// - X coordinate
// It calculates address in TSline from X coord respecting X flip
// and automatically sets its inc/dec direction.
// When rendering dram_done, the strobe issued.


module video_ts_render (

// clocks
	input wire clk,

// controls
	input  wire        reset,      // line start strobe
	input  wire        go,         // 1 clk 28MHz strobe for render start
	input  wire        reload,
	input  wire [ 8:0] x_coord,
	input  wire [ 2:0] x_size,     // number of cycles: 0: 8 pix/2 words ... 7: 64 pix/16 words
	input  wire        x_flip,
	input  wire [ 7:0] page,       // starting page of bitmap (TxGpage or SGpage)
	input  wire [ 8:0] line,       // line of bitmap
	input  wire [ 6:0] addr,       // address of word within the line
	input  wire [ 3:0] pal,
	output wire        dram_done,  // issued in the last dram_next
	output wire        next,       // issued in the clock when the last pix is written, can be used for reload

// TS-Line interface
	output wire [ 8:0] ts_waddr,
	output wire [ 7:0] ts_wdata,
	output wire        ts_we,

// DRAM interface
	output wire [20:0] dram_addr,
	output wire        dram_req,
	input  wire [15:0] dram_rdata,
	input  wire        dram_next

);


assign next = dram_done & 

// DRAM request
    assign dram_req = go | !dram_done;


// DRAM addressing
    assign dram_addr = {page_int, line[5:0], addr_int};
    wire [7:0] page_int = page + line[8:6];
    wire [6:0] addr_int = (go & reload) ? addr : dram_next ? addr_reg + 1 : addr_reg;

    reg [6:0] addr_reg;
    always @(posedge clk)
        addr_reg <= addr_int;


// DRAM data fetching
    reg [11:0] data;
    always @(posedge clk)
        if (dram_next)
            data <= {dram_rdata[3:0], dram_rdata[15:8]};


// TSline address
    assign ts_waddr = (go & reload) ? x_coord_c : x_reg;
    wire [8:0] x_next = x_flip ? ts_waddr - 1 : ts_waddr + 1;
    wire [8:0] x_coord_c = !x_flip ? x_coord : x_coord + 7;     // fix it !!!

    reg [8:0] x_reg;
    always @(posedge clk)
        if (ts_wr)
            x_reg <= x_next;
        else if (go & reload)
            x_reg <= x_coord_c;


// cycles counter
    wire dram_done = cyc[4];

    reg [4:0] cyc;
    always @(posedge clk)
        if (reset)
            cyc <= 5'b10000;
        else if (go)
            cyc <= {1'b0, x_size, 1'b1};

        else if (dram_next)
            cyc <= cyc - 1;


// pixel counter
    assign ts_wr = dram_next | !cnt[2];
    assign ts_we = ts_wr & |pix;

    reg [2:0] cnt;
    always @(posedge clk)
        if (reset)
            cnt <= 3'b100;

        else if (dram_next)
            cnt <= 3'b001;

        else if (!cnt[2])
            cnt <= cnt + 1;


// rendering
    assign ts_wdata = {pal, pix};
    wire [3:0] pix = pix_m[cnt[1:0]];

    wire [3:0] pix_m[0:3];
    assign pix_m[0] = dram_rdata[7:4];
    assign pix_m[1] = data[11:8];
    assign pix_m[2] = data[7:4];
    assign pix_m[3] = data[3:0];


endmodule
