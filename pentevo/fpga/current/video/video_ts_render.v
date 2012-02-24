`include "../include/tune.v"

// This module renders pixels into TS-line for tiles/sprites
//
// It receives as inputs:
// - DRAM address to read from (latched by 'go')
// - number of cycles to render (one cycle is 2 words = 8 pixels) (latched by 'go', used for coerce of X if X flip active)
// - X coordinate (latched by 'go', re-latched by 'dram_pre_next' at the 1st rendering cycle)
// - X flip bit (used for X coerce by 'go', re-latched by 'dram_pre_next' at the 1st rendering cycle)
// - palette selector (latched by 'go', re-latched by 'dram_pre_next' at the 1st rendering cycle)
//
// It calculates address in TS-line from X coord respecting the X flip
// and automatically sets its inc/dec direction.
// At the end of last DRAM cycle the strobe 'done' issued.
// It should be used for next cycle 'go' strobe
//
// Reload of address is performed at the same clock period as 'done' appears with 'go' asserted
//
// Re-latch of parameters (X, pal) is used for pipe processing.
// While rendering is being performed, the new values could appear at the input bus.
// They could be changed next clock after 'go' issued.


module video_ts_render (

// clocks
	input wire clk, c1,

// controls
	input  wire        reset,      // line start strobe, inits the machine
	input  wire        go,         // 1 clk 28MHz strobe for render start. must be issued at 'done' clk period for continuous process
	input  wire        reload,     // should be issued together with 'go', makes latch of X, pal at the next rendering cycle
	input  wire [ 8:0] x_coord,    // address in TS-line where render to, coerced according to 'x_size' and 'x_flip'
	input  wire [ 2:0] x_size,     // number of rendering cycles (each cycle = 8 pixels, 0 = 1 cycle)
	input  wire        x_flip,
	input  wire [ 7:0] page,       // start page for bitmap (TxGpage or SGpage)
	input  wire [ 8:0] line,       // line within the bitmap
	input  wire [ 6:0] addr,       // address of word within the line
	input  wire [ 3:0] pal,        // palette selector, bits[7:4] for CRAM address
	output wire        done,       // issued at the last dram_next

// TS-Line interface
	output wire [ 8:0] ts_waddr,
	output wire [ 7:0] ts_wdata,
	output wire        ts_we,

// DRAM interface
	output wire [20:0] dram_addr,
	output wire        dram_req,
	input  wire [15:0] dram_rdata,
	input  wire        dram_pre_next,
	input  wire        dram_next

);


// DRAM request
    assign dram_req = go | !done;


// DRAM addressing
    assign dram_addr = go ? {page_line, addr} : addr_reg;
    wire [13:0] page_line = {page + line[8:6], line[5:0]};
    wire [20:0] addr_next = {dram_addr[20:7], dram_next ? dram_addr[6:0] + 1 : dram_addr[6:0]};
    // as renderer can't move outside the single bitmap line, only 7 bits are processed

    reg [20:0] addr_reg;
    always @(posedge clk)
        addr_reg <= addr_next;


// DRAM data fetching
    reg [11:0] data;
    always @(posedge clk)
        if (dram_next)
            data <= {dram_rdata[3:0], dram_rdata[15:8]};


// interim regs processing
    wire [8:0] x_coord_c = x_flip ? (x_coord + flip_adder) : x_coord;
    wire [5:0] flip_adder = (x_size + 1) * 8 - 1;

    reg [8:0] x_coord_r;
    reg [3:0] pal_r;
    reg x_flip_r;
    always @(posedge clk)
        if (go)
        begin
            x_coord_r <= x_coord_c;
            pal_r <= pal;
            x_flip_r <= x_flip;
        end


    wire rld = dram_pre_next & reload_r;

    reg reload_r;
    reg [3:0] pal_rr;
    reg x_flip_rr;
    always @(posedge clk)
        if (go & reload)
            reload_r <= 1'b1;
        else if (rld)
        begin
            reload_r <= 1'b0;
            pal_rr <= pal_r;
            x_flip_rr <= x_flip_r;
        end


// TS-line address
    assign ts_waddr = rld ? x_coord_r : x_reg;
    wire [8:0] x_next = x_flip_rr ? ts_waddr - 1 : ts_waddr + 1;

    reg [8:0] x_reg;
    always @(posedge clk)
        if (ts_wr)
            x_reg <= x_next;
        else if (rld)
            x_reg <= x_coord_r;


// cycles counter
    assign done = cyc[4];

    reg [4:0] cyc;
    always @(posedge clk)
        if (reset)
            cyc <= 5'b10000;
        else if (go)
            cyc <= {1'b0, x_size, 1'b1};

        else if (dram_next)
            cyc <= cyc - 1;


// pixel render counter
    wire ts_wr = dram_next | !cnt[2];   // write signal for TS-line (for internal use)
    assign ts_we = ts_wr & |pix;        // write signal for TS-line masked by transparency

    reg [2:0] cnt;
    always @(posedge clk)
        if (reset)
            cnt <= 3'b100;

        else if (dram_pre_next)
            cnt <= 3'b000;

        else if (!cnt[2])
            cnt <= cnt + 1;


// rendering
    assign ts_wdata = {pal_rr, pix};
    wire [3:0] pix = pix_m[cnt[1:0]];

    wire [3:0] pix_m[0:3];
    assign pix_m[0] = dram_rdata[7:4];
    assign pix_m[1] = data[11:8];
    assign pix_m[2] = data[7:4];
    assign pix_m[3] = data[3:0];


endmodule
